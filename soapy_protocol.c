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
#include "soapy_protocol.h"
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

static SoapySDRDevice *soapy_device;
static SoapySDRStream *rx_stream;
static SoapySDRStream *tx_stream;
static int soapy_rx_sample_rate;
static int soapy_tx_sample_rate;
static float *buffer;
static int max_samples;

static int samples=0;

static GThread *receive_thread_id;
static gpointer receive_thread(gpointer data);

static int actual_rate;

#ifdef TIMING
static int rate_samples;
#endif

static gboolean running;

SoapySDRDevice *get_soapy_device() {
  return soapy_device;
}

void soapy_protocol_change_sample_rate(RECEIVER *rx,int rate) {
}

static void soapy_protocol_start_receiver(RECEIVER *rx) {
  int rc;

  soapy_rx_sample_rate=rx->sample_rate;

fprintf(stderr,"soapy_protocol_start_receiver: setting samplerate=%f\n",(double)soapy_rx_sample_rate);
  rc=SoapySDRDevice_setSampleRate(soapy_device,SOAPY_SDR_RX,rx->adc,(double)soapy_rx_sample_rate);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_setSampleRate(%f) failed: %s\n",(double)soapy_rx_sample_rate,SoapySDR_errToStr(rc));
  }

  size_t channel=rx->adc;
fprintf(stderr,"soapy_protocol_start_receiver: SoapySDRDevice_setupStream: channel=%ld\n",channel);
  rc=SoapySDRDevice_setupStream(soapy_device,&rx_stream,SOAPY_SDR_RX,SOAPY_SDR_CF32,&channel,1,NULL);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol_start_receiver: SoapySDRDevice_setupStream failed: rc=%d %s\n",rc,SoapySDR_errToStr(rc));
    _exit(-1);
  }

  max_samples=SoapySDRDevice_getStreamMTU(soapy_device,rx_stream);
fprintf(stderr,"soapy_protocol_start_receiver: max_samples=%d\n",max_samples);
  buffer=(float *)malloc(max_samples*sizeof(float)*2);

  rc=SoapySDRDevice_activateStream(soapy_device, rx_stream, 0, 0LL, 0);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol_start_receiver: SoapySDRDevice_activateStream failed: %s\n",SoapySDR_errToStr(rc));
    _exit(-1);
  }

fprintf(stderr,"soapy_protocol_start_receiver: audio_open_output\n");
  if(audio_open_output(rx)!=0) {
    rx->local_audio=false;
    fprintf(stderr,"audio_open_output failed\n");
  }

fprintf(stderr,"soapy_protocol_start_receiver: create receive_thread\n");
  receive_thread_id = g_thread_new( "soapy protocol", receive_thread, rx);
  if( ! receive_thread_id )
  {
    fprintf(stderr,"g_thread_new failed for receive_thread\n");
    exit( -1 );
  }
  fprintf(stderr, "receive_thread: id=%p\n",receive_thread_id);
}

void soapy_protocol_configure_transmitter(TRANSMITTER *tx) {
  int rc;

  soapy_tx_sample_rate=tx->iq_output_rate;

fprintf(stderr,"soapy_protocol_configure_transmitter: setting samplerate=%f\n",(double)soapy_tx_sample_rate);
  rc=SoapySDRDevice_setSampleRate(soapy_device,SOAPY_SDR_TX,tx->rx->adc,(double)soapy_tx_sample_rate);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol_configure_transmitter: SoapySDRDevice_setSampleRate(%f) failed: %s\n",(double)soapy_tx_sample_rate,SoapySDR_errToStr(rc));
  }

  size_t channel=tx->rx->adc;
fprintf(stderr,"soapy_protocol_configure_transmitter: SoapySDRDevice_setupStream: channel=%ld\n",channel);
  rc=SoapySDRDevice_setupStream(soapy_device,&tx_stream,SOAPY_SDR_TX,SOAPY_SDR_CF32,&channel,1,NULL);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol_configure_transmitter: SoapySDRDevice_setupStream failed: rc=%d %s\n",rc,SoapySDR_errToStr(rc));
    _exit(-1);
  }

}

void soapy_protocol_start_transmitter(TRANSMITTER *tx) {
  int rc;

fprintf(stderr,"soapy_protocol_start_transmitter: activateStream\n");
  rc=SoapySDRDevice_activateStream(soapy_device, tx_stream, 0, 0LL, 0);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol_start_transmitter: SoapySDRDevice_activateStream failed: %s\n",SoapySDR_errToStr(rc));
    _exit(-1);
  }
}

void soapy_protocol_stop_transmitter(TRANSMITTER *tx) {
  int rc;

fprintf(stderr,"soapy_protocol_stop_transmitter: deactivateStream\n");
  rc=SoapySDRDevice_deactivateStream(soapy_device, tx_stream, 0, 0LL);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol_stop_transmitter: SoapySDRDevice_deactivateStream failed: %s\n",SoapySDR_errToStr(rc));
    _exit(-1);
  }
}

void soapy_protocol_init(RADIO *r,int rx) {
  SoapySDRKwargs args={};
  int rc;
  int i;

fprintf(stderr,"soapy_protocol_init\n");

  // initialize the radio
fprintf(stderr,"soapy_protocol_init: SoapySDRDevice_make\n");
  SoapySDRKwargs_set(&args, "driver", radio->discovered->name);
  soapy_device=SoapySDRDevice_make(&args);
  if(soapy_device==NULL) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_make failed: %s\n",SoapySDRDevice_lastError());
    _exit(-1);
  }
  SoapySDRKwargs_clear(&args);

  for(i=0;i<r->discovered->supported_receivers;i++) {
    if(r->receiver[i]!=NULL) {
      soapy_protocol_start_receiver(r->receiver[i]);
      break;
    }
  }

  if(radio->discovered->info.soapy.tx_channels!=0) {
    soapy_protocol_configure_transmitter(r->transmitter);
  }
}

static void *receive_thread(void *arg) {
  float isample;
  float qsample;
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
  RECEIVER *rx=(RECEIVER *)arg;
  running=TRUE;
fprintf(stderr,"soapy_protocol: receive_thread\n");
  while(running) {
    elements=SoapySDRDevice_readStream(soapy_device,rx_stream,(void *)&buffer,max_samples,&flags,&timeNs,timeoutUs);
    for(i=0;i<elements;i++) {
      qsample=buffer[i*2];
      isample=buffer[(i*2)+1];
      add_iq_samples(rx,(double)isample,(double)qsample);
    }
  }

fprintf(stderr,"soapy_protocol: receive_thread: SoapySDRDevice_deactivateStream\n");
  SoapySDRDevice_deactivateStream(soapy_device,rx_stream,0,0LL);
fprintf(stderr,"soapy_protocol: receive_thread: SoapySDRDevice_closeStream\n");
  SoapySDRDevice_closeStream(soapy_device,rx_stream);
fprintf(stderr,"soapy_protocol: receive_thread: SoapySDRDevice_unmake\n");
  SoapySDRDevice_unmake(soapy_device);
  _exit(0);
}


void soapy_protocol_stop() {
fprintf(stderr,"soapy_protocol_stop\n");
  running=FALSE;
}

void soapy_protocol_set_frequency(RECEIVER *rx,double f) {
  int rc;
  char *ant;

  if(soapy_device!=NULL) {
fprintf(stderr,"soapy_protocol: setFrequency: %f\n",f);
    rc=SoapySDRDevice_setFrequency(soapy_device,SOAPY_SDR_RX,rx->adc,f,NULL);
    if(rc!=0) {
      fprintf(stderr,"soapy_protocol: SoapySDRDevice_setFrequency() failed: %s\n",SoapySDR_errToStr(rc));
    }
  }
}

void soapy_protocol_set_antenna(RECEIVER *rx,int ant) {
  int rc;
  if(soapy_device!=NULL) {
    fprintf(stderr,"soapy_protocol: set_antenna: %s\n",radio->discovered->info.soapy.rx_antenna[ant]);
    rc=SoapySDRDevice_setAntenna(soapy_device,SOAPY_SDR_RX,rx->adc,radio->discovered->info.soapy.rx_antenna[ant]);
    if(rc!=0) {
      fprintf(stderr,"soapy_protocol: SoapySDRDevice_setAntenna NONE failed: %s\n",SoapySDR_errToStr(rc));
    }
  }
}

void soapy_protocol_set_gain(RECEIVER *rx,char *name,int gain) {
  int rc;
fprintf(stderr,"soapy_protocol: settIng Gain %s=%f\n",name,(double)gain);
  rc=SoapySDRDevice_setGainElement(soapy_device,SOAPY_SDR_RX,rx->adc,name,(double)gain);
  if(rc!=0) {
    fprintf(stderr,"soapy_protocol: SoapySDRDevice_setGain %s failed: %s\n",name,SoapySDR_errToStr(rc));
  }
}

int soapy_protocol_get_gain(RECEIVER *rx,char *name) {
  double gain;
  gain=SoapySDRDevice_getGainElement(soapy_device,SOAPY_SDR_RX,rx->adc,name);
fprintf(stderr,"soapy_protocol: gettIng Gain %s=%f\n",name,gain);
  return (int)gain;
}

gboolean soapy_protocol_get_automatic_gain(RECEIVER *rx) {
  gboolean mode=SoapySDRDevice_getGainMode(soapy_device, SOAPY_SDR_RX, rx->adc);
fprintf(stderr,"soapy_protocol: gettIng GainMode=%d\n",mode);
  return mode;
}

void soapy_protocol_set_automatic_gain(RECEIVER *rx,gboolean mode) {
  int rc;
fprintf(stderr,"soapy_protocol: settIng GainMode=%d\n",mode);
  rc=SoapySDRDevice_setGainMode(soapy_device, SOAPY_SDR_RX, rx->adc,mode);
  if(rc!=0) {

    fprintf(stderr,"soapy_protocol: SoapySDRDevice_getGainMode failed: %s\n", SoapySDR_errToStr(rc));
  }
}
