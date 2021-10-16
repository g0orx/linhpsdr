/* Copyright (C)
* 2019 - John Melton, G0ORX/N6LYT
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

#include <math.h>
#include <gtk/gtk.h>

#include <wdsp.h>

#include "bpsk.h"
#include "discovered.h"
#include "receiver.h"
#include "bpsk.h"
#include "band.h"
#include "bandstack.h"
#include "mode.h"
#include "filter.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "xvtr_dialog.h"

static int my_pixels=-1;
static float *my_pixel_samples=NULL;

void bpsk_init_analyzer(BPSK *bpsk) {
    int flp[] = {0};
    double keep_time = 0.1;
    int n_pixout=1;
    int spur_elimination_ffts = 1;
    int data_type = 1;
    int fft_size = 8192;
    int window_type = 4;
    double kaiser_pi = 14.0;
    int overlap = 2048;
    int clip = 0;
    int span_clip_l = 0;
    int span_clip_h = 0;
    int pixels=bpsk->pixels;
    int stitches = 1;
    int calibration_data_set = 0;
    double span_min_freq = 0.0;
    double span_max_freq = 0.0;

g_print("bpsk_init_analyzer: channel=%d pixels=%d pixel_samples=%p\n",bpsk->channel,bpsk->pixels,bpsk->pixel_samples);

    int max_w = fft_size + (int) fmin(keep_time * (double) bpsk->fps, keep_time * (double) fft_size * (double) bpsk->fps);


g_print("SetAnalyzer id=%d buffer_size=%d fft_size=%d overlap=%d\n",bpsk->channel,bpsk->buffer_size,fft_size,overlap);


    SetAnalyzer(bpsk->channel,
            n_pixout,
            spur_elimination_ffts, //number of LO frequencies = number of ffts used in elimination
            data_type, //0 for real input data (I only); 1 for complex input data (I & Q)
            flp, //vector with one elt for each LO frequency, 1 if high-side LO, 0 otherwise
            fft_size, //size of the fft, i.e., number of input samples
            bpsk->buffer_size, //number of samples transferred for each OpenBuffer()/CloseBuffer()
            window_type, //integer specifying which window function to use
            kaiser_pi, //PiAlpha parameter for Kaiser window
            overlap, //number of samples each fft (other than the first) is to re-use from the previous
            clip, //number of fft output bins to be clipped from EACH side of each sub-span
            span_clip_l, //number of bins to clip from low end of entire span
            span_clip_h, //number of bins to clip from high end of entire span
            pixels, //number of pixel values to return.  may be either <= or > number of bins
            stitches, //number of sub-spans to concatenate to form a complete span
            calibration_data_set, //identifier of which set of calibration data to use
            span_min_freq, //frequency at first pixel value8192
            span_max_freq, //frequency at last pixel value
            max_w //max samples to hold in input ring buffers
    );
}

int maximum(float data[], int len) {
    float m = -200.0;
    int mi=-1;
    int i;
    for(i=0; i<len; ++i) {
        if(data[i]>m) {
          m=data[i];
          mi=i;
        }
    }
    return mi;
}

static gboolean bpsk_local_timer_cb(void *data) {
  // look +/-1KHz
  // assume sample rate is 768000
  // assume 15360 samples
  // assume 50Hz per sample

#define SIGNALS 16 
#define SAMPLES 50

  BPSK *bpsk=(BPSK *)data;
  int mid=bpsk->pixels/2;
  int signal[SIGNALS];
  int lag=10;
  float threshold = -70.0;
  float influence = 1;
  int rc;

  g_mutex_lock(&bpsk->mutex);
  GetPixels(bpsk->channel,0,bpsk->pixel_samples,&rc);
  g_mutex_unlock(&bpsk->mutex);

  if(rc) {
    int max1=maximum(&bpsk->pixel_samples[mid-(SIGNALS/2)],SIGNALS);

    int max2=-1;;
    float max2_value=-200.0;

    for(int i=max1-16;i<max1-4;i++) {
      if(bpsk->pixel_samples[mid-(SIGNALS/2)+i]>max2_value) {
        max2_value=bpsk->pixel_samples[mid-(SIGNALS/2)+i];
        max2=i;
      }
    }

    for(int i=max1+5;i<max1+17;i++) {
      if(bpsk->pixel_samples[mid-(SIGNALS/2)+i]>max2_value) {
        max2_value=bpsk->pixel_samples[mid-(SIGNALS/2)+i];
        max2=i;
      }
    }

    int offset;
    if(max1>max2) {
      offset=max2+((max1-max2)/2);
    } else {
      offset=max1+((max2-max1)/2);
    }
    offset=offset-(SIGNALS/2);
    offset=offset*50;

    bpsk->offset+=(double)offset;
    bpsk->count++;
    if(bpsk->count==SAMPLES) {
      offset=(int)(bpsk->offset/(double)SAMPLES);
      //g_print("offset=%d\n",offset);
      if(abs(offset)>10) {
        lo_error_update(bpsk->band,(long long)offset);
      }
      bpsk->offset=0.0;
      bpsk->count=0;
    }
  }
  return TRUE;
}

void bpsk_add_iq_samples(BPSK *bpsk,double i_sample,double q_sample) {
  bpsk->input_buffer[bpsk->samples*2]=i_sample;
  bpsk->input_buffer[(bpsk->samples*2)+1]=q_sample;
  bpsk->samples++;

  if(bpsk->samples>=bpsk->buffer_size) {
    g_mutex_lock(&bpsk->mutex);
    Spectrum0(1, bpsk->channel, 0, 0, bpsk->input_buffer);
    g_mutex_unlock(&bpsk->mutex);
    bpsk->samples=0;
  }
}

BPSK *create_bpsk(int channel,int band) {
  BPSK *bpsk=g_new0(BPSK,1);

g_print("create_bpsk: channel=%d\n",channel);
  bpsk->channel=channel;
  bpsk->band=band;
  bpsk->pixels=15360; // 50Hz per pixel at 768000 sample rate
  bpsk->buffer_size=2048;
  bpsk->input_buffer=g_new0(gdouble,bpsk->buffer_size*2);
  bpsk->fft_size=bpsk->buffer_size;
  bpsk->pixel_samples=g_new0(float,bpsk->pixels);
  bpsk->fps=10;
  bpsk->samples=0;
  bpsk->count=0;
  bpsk->offset=0.0;
  g_mutex_init(&bpsk->mutex);

  int result;
  XCreateAnalyzer(bpsk->channel, &result, 262144, 1, 1, "");
  if(result != 0) {
    g_print("XCreateAnalyzer channel=%d failed: %d\n", bpsk->channel, result);
  } else {
    bpsk_init_analyzer(bpsk);
  }

  SetDisplayDetectorMode(bpsk->channel, 0, DETECTOR_MODE_AVERAGE);
  SetDisplayAverageMode(bpsk->channel, 0,  AVERAGE_MODE_LOG_RECURSIVE);

  bpsk->update_timer_id=g_timeout_add(100,bpsk_local_timer_cb,(gpointer)bpsk);
  return bpsk;
}

void destroy_bpsk(BPSK *bpsk) {
g_print("destroy_bpsk\n");
  g_source_remove(bpsk->update_timer_id);
  g_free(bpsk->input_buffer);
  g_free(bpsk->pixel_samples);
  g_free(bpsk);
}

void reset_bpsk(BPSK *bpsk) {
g_print("reset_bpsk\n");
}

