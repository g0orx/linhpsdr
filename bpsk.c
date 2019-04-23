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

void create_bpsk(RECEIVER *rx) {
  // Open a channel for the BPSK receiver
  rx->bpsk_channel=11; // GetMaxChannels()-1;
  rx->bpsk_audio_output_buffer=g_new0(gdouble,2*rx->output_samples);

  g_print("create_receiver: OpenChannel: channel=%d buffer_size=%d sample_rate=%d fft_size=%d\n", rx->bpsk_channel, rx->buffer_size, rx->sample_rate, rx->fft_size);

  OpenChannel(rx->bpsk_channel,
              rx->buffer_size,
              rx->fft_size,
              rx->sample_rate,
              48000, // dsp rate
              48000, // output rate
              0, // receive
              1, // run
              0.010, 0.025, 0.0, 0.010, 0);

  create_anbEXT(rx->bpsk_channel,1,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  create_nobEXT(rx->bpsk_channel,1,0,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  RXASetNC(rx->bpsk_channel, rx->fft_size);
  RXASetMP(rx->bpsk_channel, rx->low_latency);

  SetRXAMode(rx->bpsk_channel, USB);
  RXASetPassband(rx->bpsk_channel,150.0,1950.0);

  SetRXAPanelGain1(rx->bpsk_channel, 0.2);
  SetRXAPanelRun(rx->bpsk_channel, 1);

  rx->bpsk_offset=125000; // assume centre at 10489675000 and beacon at 10489800000
  SetRXAShiftFreq(rx->bpsk_channel, (double)rx->bpsk_offset);
  RXANBPSetShiftFrequency(rx->bpsk_channel, (double)rx->bpsk_offset);
  SetRXAShiftRun(rx->bpsk_channel, 1);

}

void reset_bpsk(RECEIVER *rx) {
fprintf(stderr,"reset_bpsk\n");
  error=0;
  last_error=0;
}

void process_bpsk(RECEIVER *rx) {

}
