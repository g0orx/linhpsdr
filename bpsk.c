#include <math.h>
#include <gtk/gtk.h>
#include <netinet/in.h>

#include <wdsp.h>

#include "receiver.h"
#include "bpsk.h"
#include "band.h"
#include "bandstack.h"
#include "mode.h"
#include "filter.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "adc.h"
#include "radio.h"
#include "xvtr_dialog.h"

static float *average=NULL;
static int pixels=0;
static int count=0;
static gint64 error=0;
static gint64 last_error=0;
static gboolean first=TRUE;

static gboolean ready=FALSE;

#define MULTIPLE 16

void create_bpsk(RECEIVER *rx) {
  // Open a channel for the BPSK receiver
  rx->bpsk_channel=11; // GetMaxChannels()-1;
  rx->bpsk_audio_output_buffer=g_new0(gdouble,2*rx->output_samples*MULTIPLE);
  rx->bpsk_buffer_size=rx->output_samples;

  g_print("create_bpsk: OpenChannel: channel=%d buffer_size=%d sample_rate=%d fft_size=%d output_samples=%d\n", rx->bpsk_channel, rx->buffer_size, rx->sample_rate, rx->fft_size,rx->output_samples);

  OpenChannel(rx->bpsk_channel,
              rx->buffer_size,
              rx->fft_size,
              rx->sample_rate,
              96000, // dsp rate
              48000, // output rate
              0, // receive
              1, // run
              0.010, 0.025, 0.0, 0.010, 0);

  create_anbEXT(rx->bpsk_channel,1,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  create_nobEXT(rx->bpsk_channel,1,0,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  RXASetNC(rx->bpsk_channel, rx->fft_size);
  RXASetMP(rx->bpsk_channel, rx->low_latency);

  SetRXAMode(rx->bpsk_channel, DSB);
  RXASetPassband(rx->bpsk_channel,-1000.0,1000.0);

  SetRXAPanelGain1(rx->bpsk_channel, 0.2);
  SetRXAPanelRun(rx->bpsk_channel, 1);

  rx->bpsk_offset=125000; // assume centre at 10489675000 and beacon at 10489800000 (+/- 1000 hz)
  SetRXAShiftFreq(rx->bpsk_channel, (double)rx->bpsk_offset);
  RXANBPSetShiftFrequency(rx->bpsk_channel, (double)rx->bpsk_offset);
  SetRXAShiftRun(rx->bpsk_channel, 1);

  rx->bpsk=TRUE;

}

void destroy_bpsk(RECEIVER *rx) {
  rx->bpsk=FALSE;
}

void reset_bpsk(RECEIVER *rx) {
fprintf(stderr,"reset_bpsk\n");
  error=0;
  last_error=0;
}

void process_bpsk(RECEIVER *rx) {

  double bpsk_frequency=48000; // sample frequency in Hz

  double sum=0.0, sum_old;
  int state=0;
  int period=0;
  double thresh=0.0;
  double f;
  int error;

  fexchange0(rx->bpsk_channel, rx->iq_input_buffer, &rx->bpsk_audio_output_buffer[count], &error);
  if(error!=0 && error!=-2) {
     fprintf(stderr,"full_rx_buffer: channel=%d fexchange0: error=%d\n",rx->bpsk_channel,error);
  }
  count+=2*rx->output_samples;
  if(count==(2*rx->output_samples*MULTIPLE)) {
    // Autocorrelation
    for(int i=0;i<rx->output_samples*MULTIPLE;i++) {
      sum_old=sum;
      sum=0.0;
      for(int k=0;k<(rx->output_samples*MULTIPLE)-i;k++) {
        sum+=rx->bpsk_audio_output_buffer[k*2]*rx->bpsk_audio_output_buffer[(k+i)*2];
      }
 
      // peak detect
      if(state==2 && (sum-sum_old)<=0) {
        period=i;
        state=3;
      }
      if(state==1 && (sum>thresh) && (sum-sum_old)>0.0) {
        state=2;
      }
      if(!i) {
        thresh=sum*0.5;
        state=1;
      }
    }

    f=bpsk_frequency/(double)period;

    if(period!=0) {
      g_print("process_bpsk: f=%f period=%d thresh=%f sum=%f old_sum=%f\n",f,period,thresh,sum,sum_old);
    }
    count=0;
  }
}
