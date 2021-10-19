/* Copyright (C)
* 2018 - John Melton, G0ORX/N6LYT
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/


#include <gtk/gtk.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <semaphore.h>

#include <soundio/soundio.h>
#ifndef __APPLE__
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <pulse/simple.h>
#include <alsa/asoundlib.h>
#endif

#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#endif

#include "adc.h"
#include "dac.h"
#include "discovered.h"
#include "wideband.h"
#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "radio.h"
#include "main.h"
#include "protocol1.h"
#include "protocol2.h"
#ifdef SOAPYSDR
#include "soapy_protocol.h"
#endif
#include "audio.h"

int n_input_devices;
AUDIO_DEVICE input_devices[MAX_AUDIO_DEVICES];
int n_output_devices;
AUDIO_DEVICE output_devices[MAX_AUDIO_DEVICES];



static int running=FALSE;

#ifndef __APPLE__
static snd_pcm_format_t record_audio_format;

#define FORMATS 3
static snd_pcm_format_t formats[3]={
  SND_PCM_FORMAT_FLOAT_LE,
  SND_PCM_FORMAT_S32_LE,
  SND_PCM_FORMAT_S16_LE};
#endif

static GThread *mic_read_thread_id;
static void *mic_read_thread(void *arg);


struct SoundIo *soundio;

#ifndef __APPLE__
//static pa_buffer_attr bufattr;
static pa_glib_mainloop *main_loop;
static pa_mainloop_api *main_loop_api;
static pa_operation *op;
static pa_context *pa_ctx;
#endif

static int ready=0;
static int sample_rate=48000;


static int underflow_count=0;

static void underflow_callback(struct SoundIoOutStream *outstream) {
  underflow_count++;
  //g_print("audio_write: underflow %d\n", underflow_count);
}

static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
  RECEIVER *rx=(RECEIVER *)outstream->userdata;
  struct SoundIoChannelArea *areas;
  int frames_left;
  int frame_count;
  int err;

  char *read_ptr = soundio_ring_buffer_read_ptr(rx->ring_buffer);
  int fill_bytes = soundio_ring_buffer_fill_count(rx->ring_buffer);
  int fill_count = fill_bytes / outstream->bytes_per_frame;

  if (frame_count_min > fill_count) {
    //g_print("write_callback: not enough data: frame_count_min=%d fill_count=%d bytes_per_frame=%d\n",frame_count_min,fill_count,outstream->bytes_per_frame);
    // Ring buffer does not have enough data, fill with zeroes.
    frames_left = frame_count_min;
    for (;;) {
      frame_count = frames_left;
      if (frame_count <= 0)
        return;
      if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
        //g_print("write_callback: begin write error: %s\n", soundio_strerror(err));
        return;
      }
      if (frame_count <= 0)
        return;
      for (int frame = 0; frame < frame_count; frame += 1) {
        for (int ch = 0; ch < outstream->layout.channel_count; ch += 1) {
          memset(areas[ch].ptr, 0, outstream->bytes_per_sample);
          areas[ch].ptr += areas[ch].step;
        }
      }
      if ((err = soundio_outstream_end_write(outstream)))
        //g_print("write_callback: end write error: %s\n", soundio_strerror(err));
        frames_left -= frame_count;
      }
    }

    int read_count;
    if(frame_count_max<fill_count) read_count=frame_count_max; else read_count=fill_count;
    frames_left = read_count;

    while (frames_left > 0) {
      int frame_count = frames_left;

      if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
        g_print("begin write error: %s", soundio_strerror(err));
        return;
      }

      if (frame_count <= 0)
        break;

      for (int frame = 0; frame < frame_count; frame += 1) {
        for (int ch = 0; ch < outstream->layout.channel_count; ch += 1) {
          memcpy(areas[ch].ptr, read_ptr, outstream->bytes_per_sample);
          areas[ch].ptr += areas[ch].step;
          read_ptr += outstream->bytes_per_sample;
        }
      }

      if ((err = soundio_outstream_end_write(outstream))) {
        //g_print("end write error: %s\n", soundio_strerror(err));
        return;
      }

      frames_left -= frame_count;
  }

  soundio_ring_buffer_advance_read_ptr(rx->ring_buffer, read_count * outstream->bytes_per_frame);
}

static void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max) {
    RADIO *r=(RADIO *)instream->userdata;
    struct SoundIoChannelArea *areas;
    int err;

    //if(frame_count_min!=frame_count_max) {
    if(frame_count_min<=0) {
      //g_print("read_callback: frame_counts differ min=%d max=%d\n",frame_count_min,frame_count_max);
      return;
    }

    if(r->local_microphone_buffer!=NULL) {
      if(r->local_microphone_buffer_size!=frame_count_min) {
        free(r->local_microphone_buffer);
        r->local_microphone_buffer=NULL;
      }
    }

    if(r->local_microphone_buffer==NULL) {
      r->local_microphone_buffer_size=frame_count_min;
      r->local_microphone_buffer=g_new0(float,r->local_microphone_buffer_size);
g_print("read_callback: create microphone buffer: %p length=%d (%d bytes)\n",r->local_microphone_buffer,r->local_microphone_buffer_size,instream->bytes_per_sample*r->local_microphone_buffer_size);
    }

    int frame_count=frame_count_min;

    if((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
      g_print("read_callback: begin read error: %s\n", soundio_strerror(err));
      return;
    }

    g_mutex_lock(&r->ring_buffer_mutex);

    char *write_ptr = soundio_ring_buffer_write_ptr(r->ring_buffer);
    int free_bytes = soundio_ring_buffer_free_count(r->ring_buffer);
    int free_count = free_bytes / instream->bytes_per_frame;
    if(frame_count!=0 && free_count>=frame_count) {
      if(areas==NULL) {
        g_print("read_callback: areas is NULL\n");
        memset(write_ptr,0,frame_count*instream->bytes_per_sample);
      } else {
        memcpy(write_ptr,areas[0].ptr,frame_count*instream->bytes_per_sample);
      }

      if((err = soundio_instream_end_read(instream))) {
        g_print("read_callback: end read error: %s", soundio_strerror(err));
      }

      soundio_ring_buffer_advance_write_ptr(r->ring_buffer, frame_count*instream->bytes_per_frame);
      g_cond_signal (&r->ring_buffer_cond);
    } else {
      g_print("read_callback: frame_count is 0 or free_count too small\n");
    }
    g_mutex_unlock (&r->ring_buffer_mutex);

}

int audio_open_output(RECEIVER *rx) {
  int result=0;
  int err;
#ifndef __APPLE__
  pa_sample_spec sample_spec;
#endif
  switch(radio->which_audio) {
    case USE_SOUNDIO: {
g_print("audio_open_output: SOUNDIO: %s\n",rx->audio_name);
      g_mutex_lock(&rx->local_audio_mutex);

      // find the device
      rx->output_index=-1;
      for(int i=0;i<n_output_devices;i++) {
        if(strcmp(rx->audio_name,output_devices[i].name)==0) {
          rx->output_index=output_devices[i].index;
          break;
        }
      }

      if(rx->output_index==-1) {
        g_mutex_unlock(&rx->local_audio_mutex);
        return -1;
      }
  
      rx->output_device = soundio_get_output_device(soundio, rx->output_index);
      if(!rx->output_device) {
        g_print("audio_open_output: could not get output device: out of memory");
        g_mutex_unlock(&rx->local_audio_mutex);
        return -1;
      }

      if(!soundio_device_supports_sample_rate(rx->output_device, sample_rate)) {
        g_print("audio_open_output: device does not support sample rate of %d",sample_rate);
        g_mutex_unlock(&rx->local_audio_mutex);
        return -1;
      }

      if(!soundio_device_supports_format(rx->output_device, SoundIoFormatFloat32NE)) {
        g_print("audio_open_output: device does not support SoundIoFormatFloat32NE");
        g_mutex_unlock(&rx->local_audio_mutex);
        return -1;
      }

      // guess that 8 output buffers should be enough
      int size=8*rx->output_samples*sizeof(float)*2;
      if(size<32768) size=32768;
      rx->ring_buffer = soundio_ring_buffer_create(soundio, size);
      if(!rx->ring_buffer) {
        g_print("audio_open_output: soundio_ring_buffer_create failed");
        g_mutex_unlock(&rx->local_audio_mutex);
        return -1;
      }

      rx->output_stream = soundio_outstream_create(rx->output_device);
      if(!rx->output_stream) {
        g_print("audio_open_output: could not open output device: out of memory");
        g_mutex_unlock(&rx->local_audio_mutex);
        return -1;
      }
      rx->output_stream->format = SoundIoFormatFloat32NE;
      rx->output_stream->sample_rate = sample_rate;
      rx->output_stream->write_callback = write_callback;
      rx->output_stream->underflow_callback = underflow_callback;
      rx->output_stream->software_latency = 0.01;
      rx->output_stream->userdata=(void *)rx;

      if((err = soundio_outstream_open(rx->output_stream))) {
        g_print("audio_open_output: unable to open output stream: %s", soundio_strerror(err));
        g_mutex_unlock(&rx->local_audio_mutex);
        return -1;
      }

      g_mutex_unlock(&rx->local_audio_mutex);
      break;
    }
#ifndef __APPLE__
    case USE_PULSEAUDIO: {
g_print("audio_open_output: PULSEAUDIO: %s\n",rx->audio_name);

      if(rx->audio_name==NULL) {
        result=-1;
      } else {
        g_mutex_lock(&rx->local_audio_mutex);
        sample_spec.rate=48000;
        sample_spec.channels=2;
        sample_spec.format=PA_SAMPLE_FLOAT32NE;

        char stream_id[16];
        sprintf(stream_id,"RX-%d",rx->channel);
    
        rx->playstream=pa_simple_new(NULL,               // Use the default server.
                        "linHPSDR",           // Our application's name.
                        PA_STREAM_PLAYBACK,
                        rx->audio_name,
                        stream_id,            // Description of our stream.
                        &sample_spec,                // Our sample format.
                        NULL,               // Use default channel map
                        NULL,               // Use default buffering attributes.
                        &err               // error code if returns NULL
                        );
    
        if(rx->playstream!=NULL) {
          rx->local_audio_buffer_offset=0;
          rx->local_audio_buffer=g_new0(float,2*rx->local_audio_buffer_size);
          fprintf(stderr,"audio_open_output: allocated local_audio_buffer %p size %ld bytes\n",rx->local_audio_buffer,2*rx->local_audio_buffer_size*sizeof(float));
        } else {
          result=-1;
          fprintf(stderr,"pa-simple_new failed: err=%d\n",err);
        }
        g_mutex_unlock(&rx->local_audio_mutex);
      }
      break;
    }
    case USE_ALSA: {
g_print("audio_open_output: ALSA: %s\n",rx->audio_name);
      int err;
      unsigned int rate = 48000;
      unsigned int channels=2;
      int soft_resample=1;
      unsigned int latency=125000;      
      
      if(rx->audio_name==NULL) {
        rx->local_audio=0;
        return -1;
      }

      int i;
      char hw[128];

      i=0;
      while(i<127 && rx->audio_name[i]!=' ') {
        hw[i]=rx->audio_name[i];
        i++;
      }
      hw[i]='\0';
      
    g_print("audio_open_output: hw=%s\n",hw);

      for(i=0;i<FORMATS;i++) {
        g_mutex_lock(&rx->local_audio_mutex);
        if ((err = snd_pcm_open (&rx->playback_handle, hw, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
          g_print("audio_open_output: cannot open audio device %s (%s)\n", 
                  hw,
                  snd_strerror (err));
          g_mutex_unlock(&rx->local_audio_mutex);
          return err;
        }
    g_print("audio_open_output: handle=%p\n",rx->playback_handle);

    g_print("audio_open_output: trying format %s (%s)\n",snd_pcm_format_name(formats[i]),snd_pcm_format_description(formats[i]));
        if ((err = snd_pcm_set_params (rx->playback_handle,formats[i],SND_PCM_ACCESS_RW_INTERLEAVED,channels,rate,soft_resample,latency)) < 0) {
          g_print("audio_open_output: snd_pcm_set_params failed: %s\n",snd_strerror(err));
          g_mutex_unlock(&rx->local_audio_mutex);
          audio_close_output(rx);
          continue;
        } else {
    g_print("audio_open_output: using format %s (%s)\n",snd_pcm_format_name(formats[i]),snd_pcm_format_description(formats[i]));
          rx->local_audio_format=formats[i];
          break;
        }
      }

      if(i>=FORMATS) {
        g_print("audio_open_output: cannot find usable format\n");
        return err;
      }

      rx->local_audio_buffer_offset=0;
      switch(rx->local_audio_format) {
        case SND_PCM_FORMAT_S16_LE:
    g_print("audio_open_output: local_audio_buffer: size=%d sample=%ld\n",rx->local_audio_buffer_size,sizeof(gint16));
          rx->local_audio_buffer=g_new(gint16,2*rx->local_audio_buffer_size);
            break;
        case SND_PCM_FORMAT_S32_LE:
    g_print("audio_open_output: local_audio_buffer: size=%d sample=%ld\n",rx->local_audio_buffer_size,sizeof(gint32));
          rx->local_audio_buffer=g_new(gint32,2*rx->local_audio_buffer_size);
          break;
        case SND_PCM_FORMAT_FLOAT_LE:
    g_print("audio_open_output: local_audio_buffer: size=%d sample=%ld\n",rx->local_audio_buffer_size,sizeof(gfloat));
          rx->local_audio_buffer=g_new(gfloat,2*rx->local_audio_buffer_size);
          break;

        default: return -1;
      }
      
      g_print("audio_open_output: rx=%d handle=%p buffer=%p size=%d\n",rx->channel,rx->playback_handle,rx->local_audio_buffer,rx->local_audio_buffer_size);      

      g_mutex_unlock(&rx->local_audio_mutex);          
      break;
    }
#endif
  }
  return result;
}
	
int audio_open_input(RADIO *r) {
  int result=0;
  int err;
#ifndef __APPLE__
  pa_sample_spec sample_spec;
#endif

  g_print("%s\n",__FUNCTION__);
  switch(radio->which_audio) {
    case USE_SOUNDIO: {
      if(r->microphone_name==NULL) {
        g_print("audio_open_input: microphone name is NULL\n");
        return -1;
      }

      g_print("audio_open_input: %s\n",r->microphone_name);
      // find the device
      int input_index=-1;
      for(int i=0;i<n_input_devices;i++) {
        if(strcmp(r->microphone_name,input_devices[i].name)==0) {
          input_index=input_devices[i].index;
          break;
        }
      }

      if(input_index==-1) {
        g_print("audio_open_input: did not find %s\n",r->microphone_name);
        return -1;
      }


      r->input_device = soundio_get_input_device(soundio, input_index);
      if(!r->input_device) {
        g_print("audio_open_input: could not get input device: out of memory");
        return -1;
      }

      if(!soundio_device_supports_sample_rate(r->input_device, sample_rate)) {
        g_print("audio_open_input: device does not support sample rate of %d",sample_rate);
        return -1;
      }

      if(!soundio_device_supports_format(r->input_device, SoundIoFormatFloat32NE)) {
        g_print("audio_open_input: device does not support SoundIoFormatFloat32NE");
        return -1;
      }

      r->input_stream = soundio_instream_create(r->input_device);
      if(!r->input_stream) {
        g_print("audio_open_input: could not open input device: out of memory");
        return -1;
      }
      r->input_stream->format = SoundIoFormatFloat32NE;
      r->input_stream->sample_rate = sample_rate;
      r->input_stream->read_callback = read_callback;
      r->input_stream->userdata=(void *)r;

      if((err = soundio_instream_open(r->input_stream))) {
        g_print("audio_open_input: unable to open input stream: %s", soundio_strerror(err));
        return -1;
      }

      // guess that 8 input buffers should be enough
      int size=8*512*sizeof(float);
      r->ring_buffer = soundio_ring_buffer_create(soundio, size);
      if(!r->ring_buffer) {
        g_print("audio_open_input: soundio_ring_buffer_create failed");
        return -1;
      }

      if((err = soundio_instream_start(r->input_stream))) {
        g_print("unable to start input device: %s", soundio_strerror(err));
        return -1;
      }
      r->input_started=TRUE;
      running=TRUE;
      g_print("%s: SOUNDIO mic_read_thread\n",__FUNCTION__);
      mic_read_thread_id = g_thread_new( "mic_thread", mic_read_thread, r);
      if(!mic_read_thread_id ) {
        fprintf(stderr,"g_thread_new failed on mic_read_thread\n");
        soundio_instream_destroy(r->input_stream);
        soundio_device_unref(r->input_device);
        soundio_ring_buffer_destroy(r->ring_buffer);
        if(r->local_microphone_buffer!=NULL) {
          g_free(r->local_microphone_buffer);
          r->local_microphone_buffer=NULL;
        }
        running=FALSE;
        result=-1;
      }
      break;
    }
#ifndef __APPLE__
    case USE_PULSEAUDIO: {
      if(r->microphone_name==NULL) {
        return -1;
      }

      g_mutex_lock(&r->local_microphone_mutex);
      
      
      pa_buffer_attr attr;
      attr.maxlength = (uint32_t) -1;
      attr.tlength = (uint32_t) -1;
      attr.prebuf = (uint32_t) -1;
      attr.minreq = (uint32_t) -1;
      attr.fragsize = 512;    
      
      
      sample_spec.rate=48000;
      sample_spec.channels=1;
      sample_spec.format=PA_SAMPLE_FLOAT32NE;

      r->microphone_stream=pa_simple_new(NULL,               // Use the default server.
                      "linHPSDR",           // Our application's name.
                      PA_STREAM_RECORD,
                      r->microphone_name,
                      "TX",            // Description of our stream.
                      &sample_spec,                // Our sample format.
                      NULL,               // Use default channel map
                      //NULL,
                      &attr,               // Use default buffering attributes.
                      NULL               // Ignore error code.
                      );

      if(r->microphone_stream!=NULL) {
        r->local_microphone_buffer_offset=0;
        r->local_microphone_buffer=g_new0(float,r->local_microphone_buffer_size);
        running=TRUE;
        g_print("%s: PULSEAUDIO mic_read_thread\n",__FUNCTION__);
        mic_read_thread_id = g_thread_new( "mic_thread", mic_read_thread, r);
        if(!mic_read_thread_id ) {
          fprintf(stderr,"g_thread_new failed on mic_read_thread\n");
          g_free(r->local_microphone_buffer);
          r->local_microphone_buffer=NULL;
          running=FALSE;
          result=-1;
        }
      } else {
        result=-1;
      }
      g_mutex_unlock(&r->local_microphone_mutex);
      break;
    }
    case USE_ALSA: {
      int err;
      unsigned int rate=48000;
      unsigned int channels=1;
      int soft_resample=1;
      unsigned int latency=125000;
      
      char hw[64];
      int i = 0;
      
      if(r->microphone_name==NULL) {
        r->local_microphone=0;
        return -1;
      }
        
      while(r->microphone_name[i]!=' ') {
        hw[i]=r->microphone_name[i];
        i++;
      }
      hw[i]='\0';      
      
      g_print("audio_open_input: hw=%s\n",hw);

      for(i=0;i<FORMATS;i++) {
        if ((err = snd_pcm_open (&r->record_handle, hw, SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC)) < 0) {
          g_print("audio_open_input: cannot open audio device %s (%s)\n",
                  hw,
                  snd_strerror (err));
          return err;
        }
g_print("audio_open_input: handle=%p\n",r->record_handle);

g_print("audio_open_input: trying format %s (%s)\n",snd_pcm_format_name(formats[i]),snd_pcm_format_description(formats[i]));
        if ((err = snd_pcm_set_params (r->record_handle,formats[i],SND_PCM_ACCESS_RW_INTERLEAVED,channels,rate,soft_resample,latency)) < 0) {
          g_print("audio_open_input: snd_pcm_set_params failed: %s\n",snd_strerror(err));
          audio_close_input(r);
          continue;
        } else {
g_print("audio_open_input: using format %s (%s)\n",snd_pcm_format_name(formats[i]),snd_pcm_format_description(formats[i]));
          record_audio_format=formats[i];
          break;
        }
      }

      if(i>=FORMATS) {
g_print("audio_open_input: cannot find usable format\n");
        return err;
      }

g_print("audio_open_input: format=%d\n",record_audio_format);

      switch(record_audio_format) {
        case SND_PCM_FORMAT_S16_LE:
g_print("audio_open_input: mic_buffer: size=%d channels=%d sample=%ld bytes\n",r->local_microphone_buffer_size,channels,sizeof(gint16));
          r->local_microphone_buffer=g_new(gint16, r->local_microphone_buffer_size);
          break;
        case SND_PCM_FORMAT_S32_LE:
g_print("audio_open_input: mic_buffer: size=%d channels=%d sample=%ld bytes\n",r->local_microphone_buffer_size,channels,sizeof(gint32));
          r->local_microphone_buffer=g_new(gint32, r->local_microphone_buffer_size);
          break;
        case SND_PCM_FORMAT_FLOAT_LE:
g_print("audio_open_input: mic_buffer: size=%d channels=%d sample=%ld bytes\n",r->local_microphone_buffer_size,channels,sizeof(gfloat));
          r->local_microphone_buffer=g_new(gfloat, r->local_microphone_buffer_size);
          break;
          
        default: return -1;          
      }

      //r->local_microphone_buffer_offset=0;
      //r->local_microphone_buffer=g_new0(float,r->local_microphone_buffer_size);
      running=TRUE;

      g_print("%s: ALSA mic_read_thread\n",__FUNCTION__);
      GError *error;
      mic_read_thread_id = g_thread_try_new("microphone",mic_read_thread,r,&error);
      if(!mic_read_thread_id ) {
        g_print("g_thread_new failed on mic_read_thread: %s\n",error->message);
        g_free(r->local_microphone_buffer);
        r->local_microphone_buffer=NULL;
        running=FALSE;
        result=-1;        
      }
      break;
  }
#endif  
  }
  return result;
}

void audio_close_output(RECEIVER *rx) {
 g_print("audio_close_output\n");
  switch(radio->which_audio) {
    case USE_SOUNDIO: {
      g_mutex_lock(&rx->local_audio_mutex);
      if(rx->output_stream!=NULL) {
        soundio_outstream_destroy(rx->output_stream);
        rx->output_stream=NULL;
      }
      if(rx->output_device!=NULL) {
        soundio_device_unref(rx->output_device);
        rx->output_device=NULL;
      } 
      if(rx->ring_buffer!=NULL) {
        soundio_ring_buffer_destroy(rx->ring_buffer);
        rx->ring_buffer=NULL;
      }
      rx->output_started=FALSE;
      g_mutex_unlock(&rx->local_audio_mutex);
      break;
    }
#ifndef __APPLE__
    case USE_PULSEAUDIO: {
      g_mutex_lock(&rx->local_audio_mutex);
      if(rx->playstream!=NULL) {
        pa_simple_free(rx->playstream);
        rx->playstream=NULL;
      }
      if(rx->local_audio_buffer!=NULL) {
        g_free(rx->local_audio_buffer);
        rx->local_audio_buffer=NULL;
      }
      rx->output_started=FALSE;
      g_mutex_unlock(&rx->local_audio_mutex);
      break;
    }
    case USE_ALSA: {    
      g_mutex_lock(&rx->local_audio_mutex);
      if(rx->playback_handle!=NULL) {
        snd_pcm_close (rx->playback_handle);
        rx->playback_handle=NULL;
      }
      if(rx->local_audio_buffer!=NULL) {
        g_free(rx->local_audio_buffer);
        rx->local_audio_buffer=NULL;
      }
      rx->output_started=FALSE;
      g_mutex_unlock(&rx->local_audio_mutex);
      break;
    }
#endif
  }
}

void audio_close_input(RADIO *r) {
  g_print("Close audio input\n");
  switch(radio->which_audio) {
    case USE_SOUNDIO: {
      g_mutex_lock(&r->local_microphone_mutex);
      soundio_instream_destroy(r->input_stream);
      soundio_device_unref(r->input_device);
      soundio_ring_buffer_destroy(r->ring_buffer);
      g_mutex_unlock(&r->local_microphone_mutex);
      break;
    }
#ifndef __APPLE__
    case USE_PULSEAUDIO: {
      g_mutex_lock(&r->local_microphone_mutex);
      if(r->microphone_stream!=NULL) {
        pa_simple_free(r->microphone_stream);
        r->microphone_stream=NULL;
        g_free(r->local_microphone_buffer);
        r->local_microphone_buffer=NULL;
      }
      g_mutex_unlock(&r->local_microphone_mutex);
      break;
    }
    case USE_ALSA: {
      g_mutex_lock(&r->local_microphone_mutex);
      running=FALSE;
      if(mic_read_thread_id!=NULL) {
g_print("audio_close_input: wait for thread to complete\n");
        g_thread_join(mic_read_thread_id);
        mic_read_thread_id=NULL;
      }      
      if(r->record_handle!=NULL) {
g_print("audio_close_input: snd_pcm_close\n");        
        snd_pcm_close (r->record_handle);
        r->record_handle=NULL;
      }
      if(r->local_microphone_buffer!=NULL) {
g_print("audio_close_input: free mic buffer\n");
        g_free(r->local_microphone_buffer);
        r->local_microphone_buffer=NULL;
      }      
      
      g_free(r->local_microphone_buffer);
      r->local_microphone_buffer=NULL;
      g_mutex_unlock(&r->local_microphone_mutex);
      break;
    }
#endif
  }
}

void audio_start_output(RECEIVER *rx) {
  int err;
  switch(radio->which_audio) {
    case USE_SOUNDIO:
      if(rx->output_stream!=NULL) {
        if(!rx->output_started) {
          underflow_count=0;
          if((err = soundio_outstream_start(rx->output_stream))) {
              g_print("audio_start_output: unable to start output device: %s", soundio_strerror(err));
          } else {
            rx->output_started=TRUE;
          }
        }
      }
      break;
#ifndef __APPLE__
    case USE_PULSEAUDIO:
      rx->output_started=TRUE;
      break;
    case USE_ALSA:
      rx->output_started=TRUE;
      break;
#endif
  }
}

int audio_write(RECEIVER *rx,float left_sample,float right_sample) {
  int result=0;
  int rc;
  int err;
  float *float_buffer;  
  
  switch(radio->which_audio) {
    case USE_SOUNDIO: {
      g_mutex_lock(&rx->local_audio_mutex);
      float samples[2];
      samples[0]=left_sample;
      samples[1]=right_sample;
      if(rx->ring_buffer!=NULL) {
        char *buf = soundio_ring_buffer_write_ptr(rx->ring_buffer);
        int fill_count = sizeof(float)*2;
        int free=soundio_ring_buffer_free_count(rx->ring_buffer);
        if(free<fill_count) {
          //g_print("audio_write: ring buffer full: need %d free %d\n",fill_count,free);
        } else {
          memcpy(buf, &samples[0], fill_count);
          soundio_ring_buffer_advance_write_ptr(rx->ring_buffer, fill_count);
        }
      }
      g_mutex_unlock(&rx->local_audio_mutex);
      break;
    }
#ifndef __APPLE__
    case USE_PULSEAUDIO: {
      g_mutex_lock(&rx->local_audio_mutex);
      if(rx->local_audio_buffer==NULL) {
        rx->local_audio_buffer_offset=0;
        rx->local_audio_buffer=g_new0(float,2*rx->local_audio_buffer_size);
      }
      
      float_buffer=(float *)rx->local_audio_buffer;
      float_buffer[rx->local_audio_buffer_offset*2]=left_sample;
      float_buffer[(rx->local_audio_buffer_offset*2)+1]=right_sample;
      //rx->local_audio_buffer[rx->local_audio_buffer_offset*2]=left_sample;
      //rx->local_audio_buffer[(rx->local_audio_buffer_offset*2)+1]=right_sample;
      
      rx->local_audio_buffer_offset++;
      if(rx->local_audio_buffer_offset>=rx->local_audio_buffer_size) {
        rc=pa_simple_write(rx->playstream,
                           rx->local_audio_buffer,
                           rx->local_audio_buffer_size*sizeof(float)*2,
                           &err); 
        if(rc!=0) {
          fprintf(stderr,"audio_write failed err=%d\n",err);
        }
        rx->local_audio_buffer_offset=0;
      }
      g_mutex_unlock(&rx->local_audio_mutex);
      break;
    }
    case USE_ALSA: {
      snd_pcm_sframes_t delay;
      long trim;
      

      gint32 *long_buffer;
      gint16 *short_buffer;      

      g_mutex_lock(&rx->local_audio_mutex);

      if(rx->playback_handle!=NULL && rx->local_audio_buffer!=NULL) {
        switch(rx->local_audio_format) {
          case SND_PCM_FORMAT_S16_LE:
            short_buffer=(gint16 *)rx->local_audio_buffer;
            short_buffer[rx->local_audio_buffer_offset*2]=(gint16)(left_sample*32767.0F);
            short_buffer[(rx->local_audio_buffer_offset*2)+1]=(gint16)(right_sample*32767.0F);
            break;
          case SND_PCM_FORMAT_S32_LE:
            long_buffer=(gint32 *)rx->local_audio_buffer;
            long_buffer[rx->local_audio_buffer_offset*2]=(gint32)(left_sample*4294967295.0F);
            long_buffer[(rx->local_audio_buffer_offset*2)+1]=(gint32)(right_sample*4294967295.0F);
            break;
          case SND_PCM_FORMAT_FLOAT_LE:
            float_buffer=(float *)rx->local_audio_buffer;
            float_buffer[rx->local_audio_buffer_offset*2]=left_sample;
            float_buffer[(rx->local_audio_buffer_offset*2)+1]=right_sample;
            break;
            
          default: return -1;            
        }
        rx->local_audio_buffer_offset++;

        if(rx->local_audio_buffer_offset>=rx->local_audio_buffer_size) {

          trim=0;

/*
          int max_delay=rx->local_audio_buffer_size*6;
          if(snd_pcm_delay(rx->playback_handle,&delay)==0) {
            if(delay>max_delay) {
              trim=delay-max_delay;
g_print("audio delay=%ld trim=%ld audio_buffer_size=%d\n",delay,trim,rx->local_audio_buffer_size);
            }
          }
*/
          if(trim<rx->local_audio_buffer_size) {
            if ((rc = snd_pcm_writei (rx->playback_handle, rx->local_audio_buffer, rx->local_audio_buffer_size-trim)) != rx->local_audio_buffer_size-trim) {
              if(rc<0) {
                if(rc==-EPIPE) {
                  if ((rc = snd_pcm_prepare (rx->playback_handle)) < 0) {
                    g_print("audio_write: cannot prepare audio interface for use %d (%s)\n", rc, snd_strerror (rc));
                    rx->local_audio_buffer_offset=0;
                    g_mutex_unlock(&rx->local_audio_mutex);
                    return rc;
                  }
                } else {
                  // ignore short write
                }
              }
            }
          }
          rx->local_audio_buffer_offset=0;
        }
      }

      g_mutex_unlock(&rx->local_audio_mutex);
      break;
    }      
#endif
  }
  return result;
}

static void *mic_read_thread(gpointer arg) {
  RADIO *r=(RADIO *)arg;
  int rc;
  int err;
  g_print("mic_read_thread: ENTRY\n");
  switch(radio->which_audio) {
    case USE_SOUNDIO:
      while(running) {
        g_mutex_lock (&r->ring_buffer_mutex);
        while(soundio_ring_buffer_fill_count(r->ring_buffer)==0)
          g_cond_wait (&r->ring_buffer_cond, &r->ring_buffer_mutex);
        char *read_ptr = soundio_ring_buffer_read_ptr(r->ring_buffer);
        int fill_bytes = soundio_ring_buffer_fill_count(r->ring_buffer);
        if(fill_bytes>(r->local_microphone_buffer_size*sizeof(float))) {
          fill_bytes=r->local_microphone_buffer_size*sizeof(float);
        }
        memcpy(r->local_microphone_buffer,read_ptr,fill_bytes);
        soundio_ring_buffer_advance_read_ptr(r->ring_buffer, fill_bytes);
        g_mutex_unlock (&r->ring_buffer_mutex);
        switch(r->discovered->protocol) {
          case PROTOCOL_1:
            protocol1_process_local_mic(r);
            break;
          case PROTOCOL_2:
            protocol2_process_local_mic(r);
            break;
#ifdef SOAPYSDR
          case PROTOCOL_SOAPYSDR:
            soapy_protocol_process_local_mic(r);
            break;
#endif
        }
      }
      break;
#ifndef __APPLE__
    case USE_PULSEAUDIO:
      while(running) {
        g_mutex_lock(&r->local_microphone_mutex);
        if(r->local_microphone_buffer==NULL) {
          running=0;
        } else {
          rc=pa_simple_read(r->microphone_stream,
      		r->local_microphone_buffer,
      		r->local_microphone_buffer_size*sizeof(float),
      		&err); 
          if(rc<0) {
            running=0;
            g_print("mic_read_thread: returned %d error=%d (%s)\n",rc,err,pa_strerror(err));
          } else {
            switch(r->discovered->protocol) {
              case PROTOCOL_1:
                protocol1_process_local_mic(r);
                break;
              case PROTOCOL_2:
                protocol2_process_local_mic(r);
                break;
#ifdef SOAPYSDR
              case PROTOCOL_SOAPYSDR:
                soapy_protocol_process_local_mic(r);
                break;
#endif
            }
          }
        }
        g_mutex_unlock(&r->local_microphone_mutex);
      }
      break;

    case USE_ALSA:
      {
      int rc;
      if ((rc = snd_pcm_start (r->record_handle)) < 0) {
    g_print("mic_read_thread: ALSA: cannot start audio interface for use (%s)\n",
            snd_strerror (rc));

        return NULL;
      }
fprintf(stderr,"mic_read_thread: ALSA: mic_buffer_size=%d\n",radio->local_microphone_buffer_size);
      running=TRUE;
      while(running) {
        if ((rc = snd_pcm_readi (r->record_handle, r->local_microphone_buffer, r->local_microphone_buffer_size)) != r->local_microphone_buffer_size) {
          if(running) {
            if(rc<0) {
              if(rc==-EPIPE) {
                //g_print("mic_read_thread: -EPIPE: snd_pcm_prepare\n");
                if ((rc = snd_pcm_prepare (r->record_handle)) < 0) {
                    g_print("mic_read_thread: ALSA: cannot prepare audio interface for use %d (%s)\n", rc, snd_strerror (rc));
                    //return rc;
                }
              } else {
                fprintf (stderr, "mic_read_thread: ALSA: read from audio interface failed (%s)\n",
                      snd_strerror (rc));
                running=FALSE;
              }
            } else {
              fprintf(stderr,"mic_read_thread: ALSA: read %d\n",rc);
            }
          }
        } else {
          // process the mic input
          switch(r->discovered->protocol) {
            case PROTOCOL_1:        
              protocol1_process_local_mic(r);
              break;
            case PROTOCOL_2:
              protocol2_process_local_mic(r);
              break;
#ifdef SOAPYSDR
            case PROTOCOL_SOAPYSDR:
              soapy_protocol_process_local_mic(r);
              break;
#endif
          }
        }
      }

      }
      break;
#endif
  }
g_print("mic_read_thread: EXIT\n");
  return NULL;
}

void audio_get_cards() {
}

#ifndef __APPLE__
static void source_list_cb(pa_context *context,const pa_source_info *s,int eol,void *data) {
  int i;
  if(eol>0) {
    for(i=0;i<n_input_devices;i++) {
      g_print("Input: %d: %s (%s)\n",input_devices[i].index,input_devices[i].name,input_devices[i].description);
    }
  } else if(n_input_devices<MAX_AUDIO_DEVICES) {
    input_devices[n_input_devices].name=g_new0(char,strlen(s->name)+1);
    memcpy(input_devices[n_input_devices].name, s->name, strlen(s->name));
    input_devices[n_input_devices].description=g_new0(char,strlen(s->description)+1);
    memcpy(input_devices[n_input_devices].description,s->description, strlen(s->description));
    input_devices[n_input_devices].index=s->index;
    
    n_input_devices++;
  }
}

static void sink_list_cb(pa_context *context,const pa_sink_info *s,int eol,void *data) {
  int i;
  if(eol>0) {
    for(i=0;i<n_output_devices;i++) {
      g_print("Output: %d: %s (%s)\n",output_devices[i].index,output_devices[i].name,output_devices[i].description);
    }
    op=pa_context_get_source_info_list(pa_ctx,source_list_cb,NULL);
  } else if(n_output_devices<MAX_AUDIO_DEVICES) {
    output_devices[n_output_devices].name=g_new0(char,strlen(s->name)+1);
    memcpy(output_devices[n_output_devices].name,s->name,strlen(s->name));
    output_devices[n_output_devices].description=g_new0(char,strlen(s->description)+1);
    memcpy(output_devices[n_output_devices].description,s->description,strlen(s->description));
    output_devices[n_output_devices].index=s->index;
    n_output_devices++;
  }
}

static void state_cb(pa_context *c, void *userdata) {
        pa_context_state_t state;
        int *ready = userdata;

        state = pa_context_get_state(c);

g_print("%s: %d\n",__FUNCTION__,state);
        switch  (state) {
                // There are just here for reference
                case PA_CONTEXT_UNCONNECTED:
g_print("audio: state_cb: PA_CONTEXT_UNCONNECTED\n");
                        break;
                case PA_CONTEXT_CONNECTING:
g_print("audio: state_cb: PA_CONTEXT_CONNECTING\n");
                        break;
                case PA_CONTEXT_AUTHORIZING:
g_print("audio: state_cb: PA_CONTEXT_AUTHORIZING\n");
                        break;
                case PA_CONTEXT_SETTING_NAME:
g_print("audio: state_cb: PA_CONTEXT_SETTING_NAME\n");
                        break;
                case PA_CONTEXT_FAILED:
g_print("audio: state_cb: PA_CONTEXT_FAILED\n");
                        *ready = 2;
                        break;
                case PA_CONTEXT_TERMINATED:
g_print("audio: state_cb: PA_CONTEXT_TERMINATED\n");
                        *ready = 2;
                        break;
                case PA_CONTEXT_READY:
g_print("audio: state_cb: PA_CONTEXT_READY\n");
                        *ready = 1;
// get a list of the output devices
                        n_input_devices=0;
                        n_output_devices=0;
                        op = pa_context_get_sink_info_list(pa_ctx,sink_list_cb,NULL);
                        break;
                default:
                        g_print("audio: state_cb: unknown state %d\n",state);
                        break;
        }
}
#endif

void create_audio(int backend_index,const char *backend) {
  int rc;

  n_output_devices=0;
  n_input_devices=0;

  switch(radio->which_audio) {
    case USE_SOUNDIO:
g_print("audio: create_audio: USE_SOUNDIO: %d %s\n",soundio_get_backend(soundio,backend_index),backend);
      soundio=soundio_create();
      if(!soundio) {
        g_print("create_audio: soundio_create failed\n");
        return;
      }
      rc=soundio_connect_backend(soundio,soundio_get_backend(soundio,backend_index));
      if(rc) {
        g_print("create_audio: soundio_connect_backend: %s\n",soundio_strerror(rc));
        return;
      }

      soundio_flush_events(soundio);

      int output_count=soundio_output_device_count(soundio);
      for(int i=0;i<output_count;i++) {
        if(n_output_devices<MAX_AUDIO_DEVICES) {
          struct SoundIoDevice *device=soundio_get_output_device(soundio,i);

          // ignore devices that do not support the sample rate or format
          if(!soundio_device_supports_sample_rate(device, sample_rate) ) {
            continue;
          }
          if(!soundio_device_supports_format(device, SoundIoFormatFloat32NE) ) {
            continue;
          }

          output_devices[n_output_devices].name=g_new0(char,strlen(device->name)+1);
          memcpy(output_devices[n_output_devices].name,device->name,strlen(device->name));
          output_devices[n_output_devices].description=g_new0(char,strlen(device->name)+1);
          memcpy(output_devices[n_output_devices].description,device->name,strlen(device->name));
          output_devices[n_output_devices].index=i;
          soundio_device_unref(device);
          n_output_devices++;
        }
      }
      int input_count=soundio_input_device_count(soundio);
      for(int i=0;i<input_count;i++) {
        if(n_input_devices<MAX_AUDIO_DEVICES) {
          struct SoundIoDevice *device=soundio_get_input_device(soundio,i);
          input_devices[n_input_devices].name=g_new0(char,strlen(device->name)+1);
          memcpy(input_devices[n_input_devices].name,device->name,strlen(device->name));
          input_devices[n_input_devices].description=g_new0(char,strlen(device->name)+1);
          memcpy(input_devices[n_input_devices].description,device->name,strlen(device->name));
          input_devices[n_input_devices].index=i;
          soundio_device_unref(device);
          n_input_devices++;
        }
      }
      break;
  
#ifndef __APPLE__
    case USE_PULSEAUDIO:
g_print("audio: create_audio: USE_PULSEAUDIO\n");
      main_loop=pa_glib_mainloop_new(NULL);
      main_loop_api=pa_glib_mainloop_get_api(main_loop);
      pa_ctx=pa_context_new(main_loop_api,"linhpsdr");
      pa_context_connect(pa_ctx,NULL,0,NULL);
      pa_context_set_state_callback(pa_ctx, state_cb, &ready);
      break;

    case USE_ALSA:
      {

        snd_ctl_card_info_t *info;
        snd_pcm_info_t *pcminfo;
        snd_ctl_card_info_alloca(&info);
        snd_pcm_info_alloca(&pcminfo);
        int i;
        char *device_id;
        int card = -1;

        n_input_devices=0;
        n_output_devices=0;

        snd_ctl_card_info_alloca(&info);
        snd_pcm_info_alloca(&pcminfo);
        while (snd_card_next(&card) >= 0 && card >= 0) {
          int err = 0;
          snd_ctl_t *handle;
          char name[20];
          snprintf(name, sizeof(name), "hw:%d", card);
          if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
            continue;
          }

          if ((err = snd_ctl_card_info(handle, info)) < 0) {
            snd_ctl_close(handle);
            continue;
          }

          int dev = -1;

          while (snd_ctl_pcm_next_device(handle, &dev) >= 0 && dev >= 0) {
            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);

            // input devices
            snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) == 0) {
              device_id=g_new(char,128);
              snprintf(device_id, 128, "plughw:%d,%d %s", card, dev, snd_ctl_card_info_get_name(info));
              if(n_input_devices<MAX_AUDIO_DEVICES) {
                input_devices[n_input_devices].name=g_new0(char,strlen(device_id)+1);
                strcpy(input_devices[n_input_devices].name,device_id);
                input_devices[n_input_devices].description=g_new0(char,strlen(device_id)+1);
                strcpy(input_devices[n_input_devices].description,device_id);
                input_devices[n_input_devices].index=0; // not used
                n_input_devices++;
g_print("input_device: %s\n",device_id);
              }
	      g_free(device_id);
            }

            // ouput devices
            snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) == 0) {
              device_id=g_new(char,128);
              snprintf(device_id, 128, "plughw:%d,%d %s", card, dev, snd_ctl_card_info_get_name(info));
              if(n_output_devices<MAX_AUDIO_DEVICES) {
                output_devices[n_output_devices].name=g_new0(char,strlen(device_id)+1);
                strcpy(output_devices[n_output_devices].name,device_id);
                output_devices[n_output_devices].description=g_new0(char,strlen(device_id)+1);
                strcpy(output_devices[n_output_devices].description,device_id);
                input_devices[n_output_devices].index=0; // not used
                n_output_devices++;
g_print("output_device: %s\n",device_id);
              }
	    g_free(device_id);
            }
          }
          snd_ctl_close(handle);
        }
	
        // look for dmix
        void **hints, **n;
        char *name, *descr, *io;

        if (snd_device_name_hint(-1, "pcm", &hints) < 0)
          return;
        n = hints;
        while (*n != NULL) {
          name = snd_device_name_get_hint(*n, "NAME");
          descr = snd_device_name_get_hint(*n, "DESC");
          io = snd_device_name_get_hint(*n, "IOID");

          if(strncmp("dmix:", name, 5)==0) {
            if(n_output_devices<MAX_AUDIO_DEVICES) {
              output_devices[n_output_devices].name=g_new0(char,strlen(name)+1);
              strcpy(output_devices[n_output_devices].name,name);
              output_devices[n_output_devices].description=g_new0(char,strlen(descr)+1);
	      i=0;
              while(i<strlen(descr) && descr[i]!='\n') {
		output_devices[n_output_devices].description[i]=descr[i];
	        i++;
	      }
          output_devices[n_output_devices].description[i]='\0';
              input_devices[n_output_devices].index=0;  // not used
              n_output_devices++;
g_print("output_device: name=%s descr=%s\n",name,descr);
            }
fprintf(stderr,"output_device: %s\n",device_id);
          }

          if (name != NULL)
            free(name);
          if (descr != NULL)
            free(descr);
          if (io != NULL)
            free(io);
          n++;
        }
        snd_device_name_free_hint(hints);
      }
      break;
#endif
  }
  g_print("n_input_devices=%d\n", n_input_devices);
}

int audio_get_backends(RADIO *r) {
  int count=0;
  switch(r->which_audio) {
    case USE_SOUNDIO:
      count=soundio_backend_count(soundio);
      break;
#ifndef __APPLE__
    case USE_PULSEAUDIO:
      count=0;
      break;
#endif
  }
g_print("audio_get_backends: %d\n",count);
  return count;
}

const char *audio_get_backend_name(int backend_index) {
  const char *name=soundio_backend_name(soundio_get_backend(soundio,backend_index));
g_print("audio_get_backend_name: %s\n",name);
  return name;
}
