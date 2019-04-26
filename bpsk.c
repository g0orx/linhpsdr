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

void bpsk_init_analyzer(RECEIVER *rx) {
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
    int pixels=rx->bpsk_pixels;
    int stitches = 1;
    int avm = 0;
    double tau = 0.001 * 120.0;
    int calibration_data_set = 0;
    double span_min_freq = 0.0;
    double span_max_freq = 0.0;

g_print("bpsk_init_analyzer: channel=%d pixels=%d pixel_samples=%p\n",rx->bpsk_channel,rx->bpsk_pixels,rx->bpsk_pixel_samples);

  if(rx->bpsk_pixel_samples!=NULL) {
    g_free(rx->bpsk_pixel_samples);
    rx->bpsk_pixel_samples=NULL;
  }
  if(rx->bpsk_pixels>0) {
    rx->bpsk_pixel_samples=g_new0(float,rx->bpsk_pixels);
g_print("bpsk_init_analyzer: g_new0: channel=%d pixel_samples=%p\n",rx->bpsk_channel,rx->bpsk_pixel_samples);
    rx->bpsk_hz_per_pixel=(gdouble)rx->bpsk_sample_rate/(gdouble)rx->bpsk_pixels;

    int max_w = fft_size + (int) fmin(keep_time * (double) rx->fps, keep_time * (double) fft_size * (double) rx->fps);

    //overlap = (int)max(0.0, ceil(fft_size - (double)rx->sample_rate / (double)rx->fps));

g_print("SetAnalyzer id=%d buffer_size=%d fft_size=%d overlap=%d\n",rx->bpsk_channel,rx->bpsk_buffer_size,fft_size,overlap);

    SetAnalyzer(rx->bpsk_channel,
            n_pixout,
            spur_elimination_ffts, //number of LO frequencies = number of ffts used in elimination
            data_type, //0 for real input data (I only); 1 for complex input data (I & Q)
            flp, //vector with one elt for each LO frequency, 1 if high-side LO, 0 otherwise
            fft_size, //size of the fft, i.e., number of input samples
            rx->bpsk_buffer_size, //number of samples transferred for each OpenBuffer()/CloseBuffer()
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

}


void create_bpsk(RECEIVER *rx) {
  // Open a channel for the BPSK receiver
  rx->bpsk_channel=11; // GetMaxChannels()-1;
  rx->bpsk_audio_output_buffer=g_new0(gdouble,2*rx->output_samples);
  rx->bpsk_buffer_size=rx->output_samples;

  g_print("create_receiver: OpenChannel: channel=%d buffer_size=%d sample_rate=%d fft_size=%d\n", rx->bpsk_channel, rx->buffer_size, rx->sample_rate, rx->fft_size);

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

  SetRXAMode(rx->bpsk_channel, USB);
  RXASetPassband(rx->bpsk_channel,0.0,2000.0);

  SetRXAPanelGain1(rx->bpsk_channel, 0.2);
  SetRXAPanelRun(rx->bpsk_channel, 1);

  rx->bpsk_offset=124000; // assume centre at 10489675000 and beacon at 10489800000 (+/- 1000 hz)
  SetRXAShiftFreq(rx->bpsk_channel, (double)rx->bpsk_offset);
  RXANBPSetShiftFrequency(rx->bpsk_channel, (double)rx->bpsk_offset);
  SetRXAShiftRun(rx->bpsk_channel, 1);

  bpsk_init_analyzer(rx);

}

void reset_bpsk(RECEIVER *rx) {
fprintf(stderr,"reset_bpsk\n");
  error=0;
  last_error=0;
}

void process_bpsk(RECEIVER *rx) {
   Spectrum0(1, rx->channel, 0, 0, rx->audio_output_buffer);
}
