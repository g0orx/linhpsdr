#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wdsp.h>

#include "SoapySDR/Constants.h"
#include "SoapySDR/Device.h"
#include "SoapySDR/Formats.h"

//#define TIMING
#ifdef TIMING
#include <sys/time.h>
#endif

#include "band.h"
#include "channel.h"
#include "discovered.h"
#include "mode.h"
#include "filter.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "radio.h"
#include "main.h"
#include "protocol1.h"
#include "audio.h"
#include "signal.h"
#include "vfo.h"
#ifdef FREEDV
#include "freedv.h"
#endif
#ifdef PSK
#include "psk.h"
#endif
#include "ext.h"
#include "error_handler.h"

static double bandwidth=3000000.0;

static size_t soapy_receiver;
static SoapySDRDevice *soapy_device;
static SoapySDRStream *stream;
static int soapy_sample_rate;
static int soapy_buffer_size=4096;
static int outputsamples;
static int soapy_fft_size=32768;
static int dspRate=48000;
static int outputRate=48000;
static float *buffer;
static int max_samples;

//static long long saved_frequency=105400000LL;
static int saved_antenna=-1;

static double audiooutputbuffer[4096*2];
static int samples=0;

static GThread *receive_thread_id;
static gpointer receive_thread(gpointer data);

static int actual_rate;
#ifdef RESAMPLE
static void *resampler;
static double resamples[1024*2];
static double resampled[1024*2];
#endif

#ifdef TIMING
static int rate_samples;
#endif

static int running;

SoapySDRDevice *get_soapy_device() {
  return soapy_device;
}

void soapy_protocol_change_sample_rate(int rate) {
}

static void soapy_protocol_start_receiver(RECEIVER *r) {
}

void soapy_protocol_init(RADIO *r,int rx) {
  SoapySDRKwargs args={};
  int rc;
  int i;

fprintf(stderr,"soapy_protocol_init\n");

  for(i=0;i<r->discovered->supported_receivers;i++) {
    if(r->receiver[i]!=NULL) {
      soapy_protocol_start_receiver(r->receiver[i]);
      soapy_receiver=i;
      break;
    }
  }


  soapy_sample_rate=r->sample_rate;

  outputsamples=4096/(soapy_sample_rate/48000);

  // initialize the radio
fprintf(stderr,"soapy_protocol: receive_thread: SoapySDRDevice_make\n");
  SoapySDRKwargs_set(&args, "driver", radio->discovered->name);
  soapy_device=SoapySDRDevice_make(&args);
  if(soapy_device==NULL) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_make failed: %s\n",SoapySDRDevice_lastError());
    _exit(-1);
  }
  SoapySDRKwargs_clear(&args);

fprintf(stderr,"soapy_protocol: setting samplerate=%f\n",(double)soapy_sample_rate);
  rc=SoapySDRDevice_setSampleRate(soapy_device,SOAPY_SDR_RX,soapy_receiver,(double)soapy_sample_rate);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_setSampleRate(%f) failed: %s\n",(double)soapy_sample_rate,SoapySDRDevice_lastError());
  }

/*
fprintf(stderr,"soapy_protocol: set antenna to NONE\n");
  soapy_protocol_set_antenna(0);
*/

fprintf(stderr,"soapy_protocol: setting frequency: %ld\n",r->receiver[rx]->frequency_a);
    soapy_protocol_set_frequency((double)r->receiver[rx]->frequency_a);

/*
fprintf(stderr,"soapy_protocol: set baseband frequency\n");
  rc=SoapySDRDevice_setFrequencyComponent(soapy_device,SOAPY_SDR_RX,soapy_receiver,"BB",0.0,NULL);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_setFrequencyComponent(BB) failed: %s\n",SoapySDRDevice_lastError());
  }
*/
  
  soapy_protocol_set_antenna(r->adc[0].antenna);
  if(strcmp(r->discovered->name,"limesdr")==0) {
    soapy_protocol_set_gain("LNA",r->adc[0].lna);
    soapy_protocol_set_gain("PGA",r->adc[0].pga);
    soapy_protocol_set_gain("TIA",r->adc[0].tia);
  }
  if(strcmp(r->discovered->name,"rtlsdr")==0) {
    soapy_protocol_set_gain("TUNER",r->adc[0].tuner);
  }

fprintf(stderr,"soapy_protocol: receive_thread: SoapySDRDevice_setupStream\n");
  size_t channels=(size_t)soapy_receiver;
  rc=SoapySDRDevice_setupStream(soapy_device,&stream,SOAPY_SDR_RX,SOAPY_SDR_CF32,NULL,0,NULL);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_setupStream failed: %s\n",SoapySDRDevice_lastError());
    _exit(-1);
  }

  max_samples=SoapySDRDevice_getStreamMTU(soapy_device,stream);
fprintf(stderr,"soapy_protocol: max_samples=%d\n",max_samples);
  buffer=(float *)malloc(max_samples*sizeof(float)*2);

  rc=SoapySDRDevice_activateStream(soapy_device, stream, 0, 0LL, 0);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_activateStream failed: %s\n",SoapySDRDevice_lastError());
    _exit(-1);
  }


/*
  if(saved_antenna!=-1) {
fprintf(stderr,"soapy_protocol: setting save_antenna: %d\n",saved_antenna);
    soapy_protocol_set_antenna(saved_antenna);
  }

  if(saved_frequency!=0LL) {
fprintf(stderr,"soapy_protocol: setting save_frequency: %lld\n",saved_frequency);
      soapy_protocol_set_frequency(saved_frequency);
  }
*/

fprintf(stderr,"soapy_protocol_init: audio_open_output\n");
  if(audio_open_output(r->receiver[soapy_receiver])!=0) {
    r->receiver[soapy_receiver]->local_audio=false;
    fprintf(stderr,"audio_open_output failed\n");
  }

fprintf(stderr,"soapy_protocol_init: create receive_thread\n");
  receive_thread_id = g_thread_new( "soapy protocol", receive_thread, NULL);
  if( ! receive_thread_id )
  {
    fprintf(stderr,"g_thread_new failed for receive_thread\n");
    exit( -1 );
  }
  fprintf(stderr, "receive_thread: id=%p\n",receive_thread_id);
}

static void *receive_thread(void *arg) {
  float isample;
  float qsample;
  int outsamples;
  int elements;
  int flags=0;
  long long timeNs=0;
  long timeoutUs=10000L;
  int i;
#ifdef TIMING
  struct timeval tv;
  long start_time, end_time;
  rate_samples=0;
  gettimeofday(&tv, NULL); start_time=tv.tv_usec + 1000000 * tv.tv_sec;
#endif
  running=1;
fprintf(stderr,"soapy_protocol: receive_thread\n");
  while(running) {
    elements=SoapySDRDevice_readStream(soapy_device,stream,(void *)&buffer,max_samples,&flags,&timeNs,timeoutUs);
#ifdef RESAMPLE
    if(actual_rate!=soapy_sample_rate) {
      for(i=0;i<elements;i++) {
        resamples[i*2]=(double)buffer[i*2];
        resamples[(i*2)+1]=(double)buffer[(i*2)+1];
      }

      outsamples=xresample(resampler);

      for(i=0;i<outsamples;i++) {
        add_iq_samples(radio->receiver[soapy_receiver],(double)resampled[i*2],(double)resampled[(i*2)+1]);
      }
    } else {
#endif
      for(i=0;i<elements;i++) {
        qsample=buffer[i*2];
        isample=buffer[(i*2)+1];
        add_iq_samples(radio->receiver[soapy_receiver],(double)isample,(double)qsample);
      }
#ifdef RESAMPLE
    }
#endif
  }

fprintf(stderr,"soapy_protocol: receive_thread: SoapySDRDevice_closeStream\n");
  SoapySDRDevice_closeStream(soapy_device,stream);
fprintf(stderr,"soapy_protocol: receive_thread: SoapySDRDevice_unmake\n");
  SoapySDRDevice_unmake(soapy_device);

}


void soapy_protocol_stop() {
  running=0;
}

void soapy_protocol_set_frequency(gint64 f) {
  int rc;
  char *ant;

  if(soapy_device!=NULL) {
//fprintf(stderr,"soapy_protocol: setFrequency: %ld\n",f);
    rc=SoapySDRDevice_setFrequency(soapy_device,SOAPY_SDR_RX,soapy_receiver,(double)f,NULL);
    if(rc!=0) {
      fprintf(stderr,"soapy_protocol: SoapySDRDevice_setFrequency() failed: %s\n",SoapySDRDevice_lastError());
    }
  }
}

void soapy_protocol_set_antenna(int ant) {
  int rc;
 // char *antstr;
  if(soapy_device!=NULL) {
/*
    antstr=SoapySDRDevice_getAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver);
    fprintf(stderr,"soapy_protocol: set_antenna: current antenna=%s\n",antstr);
*/
    switch(ant) {
      case 0:
fprintf(stderr,"soapy_protocol: setAntenna: NONE\n");
        rc=SoapySDRDevice_setAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver,"NONE");
        if(rc!=0) {
          fprintf(stderr,"soapy_protocol: SoapySDRDevice_setAntenna NONE failed: %s\n",SoapySDRDevice_lastError());
        }
        break;
      case 1:
fprintf(stderr,"soapy_protocol: setAntenna: LNAH\n");
        rc=SoapySDRDevice_setAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver,"LNAH");
        if(rc!=0) {
          fprintf(stderr,"soapy_protocol: SoapySDRDevice_setAntenna LNAH failed: %s\n",SoapySDRDevice_lastError());
        }
        break;
      case 2:
fprintf(stderr,"soapy_protocol: setAntenna: LNAL\n");
        rc=SoapySDRDevice_setAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver,"LNAL");
        if(rc!=0) {
          fprintf(stderr,"soapy_protocol: SoapySDRDevice_setAntenna LNAL failed: %s\n",SoapySDRDevice_lastError());
        }
        break;
      case 3:
fprintf(stderr,"soapy_protocol: setAntenna: LNAW\n");
        rc=SoapySDRDevice_setAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver,"LNAW");
        if(rc!=0) {
          fprintf(stderr,"soapy_protocol: SoapySDRDevice_setAntenna LNAW failed: %s\n",SoapySDRDevice_lastError());
        }
        break;
      case 4:
fprintf(stderr,"soapy_protocol: setAntenna: LB1\n");
        rc=SoapySDRDevice_setAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver,"LB1");
        if(rc!=0) {
          fprintf(stderr,"soapy_protocol: SoapySDRDevice_setAntenna LNAW failed: %s\n",SoapySDRDevice_lastError());
        }
        break;
      case 5:
fprintf(stderr,"soapy_protocol: setAntenna: LB2\n");
        rc=SoapySDRDevice_setAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver,"LB2");
        if(rc!=0) {
          fprintf(stderr,"soapy_protocol: SoapySDRDevice_setAntenna LNAW failed: %s\n",SoapySDRDevice_lastError());
        }
        break;
    }
/*
    antstr=SoapySDRDevice_getAntenna(soapy_device,SOAPY_SDR_RX,soapy_receiver);
    fprintf(stderr,"soapy_protocol: set_antenna: antenna=%s\n",antstr);
*/
  } else {
    fprintf(stderr,"soapy_protocol: setAntenna: device is NULL\n");
    saved_antenna=ant;
  }
}

void soapy_protocol_set_gain(char *name,int gain) {
  int rc;
  fprintf(stderr,"soapy_protocol: settIng Gain %s=%f\n",name,(double)gain);
  rc=SoapySDRDevice_setGainElement(soapy_device,SOAPY_SDR_RX,soapy_receiver,name,(double)gain);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_setGain %s failed: %s\n",name,SoapySDRDevice_lastError());
  }
}

