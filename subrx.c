/* Copyright (C)
* 2020 - John Melton, G0ORX/N6LYT
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

#include "agc.h"
#include "discovered.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "bpsk.h"
#include "subrx.h"
#include "mode.h"
#include "filter.h"
#include "radio.h"
#include "main.h"

void subrx_frequency_changed(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  gint64 offset=rx->frequency_b-rx->frequency_a;
  if(rx->mode_b==CWU) {
    offset+=(gint64)radio->cw_keyer_sidetone_frequency;
  } else if(rx->mode_b==CWL) {
    offset-=(gint64)radio->cw_keyer_sidetone_frequency;
  }
  SetRXAShiftFreq(subrx->channel, (double)offset);
  RXANBPSetShiftFrequency(subrx->channel, (double)offset);
  SetRXAShiftRun(subrx->channel, 1);
}

void subrx_set_mode(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  SetRXAMode(subrx->channel, rx->mode_b);
}

void subrx_set_filter(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  RXASetPassband(subrx->channel,(double)rx->filter_low_b,(double)rx->filter_high_b);
}

void subrx_set_deviation(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  SetRXAFMDeviation(subrx->channel, (double)rx->deviation);
}


void subrx_filter_changed(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  if(rx->mode_b==FMN) {
    subrx_set_deviation(rx);
  } else {
    subrx_set_filter(rx);
  }
}

void subrx_mode_changed(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  subrx_set_mode(rx);
  subrx_filter_changed(rx);
}

void subrx_set_agc(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  SetRXAAGCMode(subrx->channel, rx->agc);
  SetRXAAGCSlope(subrx->channel,rx->agc_slope);
  SetRXAAGCTop(subrx->channel,rx->agc_gain);
  switch(rx->agc) {
    case AGC_OFF:
      break;
    case AGC_LONG:
      SetRXAAGCAttack(subrx->channel,2);
      SetRXAAGCHang(subrx->channel,2000);
      SetRXAAGCDecay(subrx->channel,2000);
      SetRXAAGCHangThreshold(subrx->channel,(int)rx->agc_hang_threshold);
      break;
    case AGC_SLOW:
      SetRXAAGCAttack(subrx->channel,2);
      SetRXAAGCHang(subrx->channel,1000);
      SetRXAAGCDecay(subrx->channel,500);
      SetRXAAGCHangThreshold(subrx->channel,(int)rx->agc_hang_threshold);
      break;
    case AGC_MEDIUM:
      SetRXAAGCAttack(subrx->channel,2);
      SetRXAAGCHang(subrx->channel,0);
      SetRXAAGCDecay(subrx->channel,250);
      SetRXAAGCHangThreshold(subrx->channel,100);
      break;
    case AGC_FAST:
      SetRXAAGCAttack(subrx->channel,2);
      SetRXAAGCHang(subrx->channel,0);
      SetRXAAGCDecay(subrx->channel,50);
      SetRXAAGCHangThreshold(subrx->channel,100);
      break;
  }
}

void create_subrx(RECEIVER *rx) {
g_print("%s: rx=%d\n",__FUNCTION__,rx->channel);
  SUBRX *subrx=g_new(SUBRX,1);
  rx->subrx=subrx;
  subrx->channel=rx->channel+SUBRX_BASE_CHANNEL;
  g_mutex_init(&subrx->mutex);
  subrx->audio_output_buffer=g_new0(gdouble,2*rx->output_samples);
  OpenChannel(subrx->channel,
              rx->buffer_size,
              rx->fft_size,
              rx->sample_rate,
              48000, // dsp rate
              48000, // output rate
              0, // receive
              1, // run
              0.010, 0.025, 0.0, 0.010, 0);

  create_anbEXT(subrx->channel,1,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  create_nobEXT(subrx->channel,1,0,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  RXASetNC(subrx->channel, rx->fft_size);
  RXASetMP(subrx->channel, rx->low_latency);

  subrx_frequency_changed(rx);
  subrx_mode_changed(rx);

  SetRXAPanelGain1(subrx->channel, rx->volume);
  SetRXAPanelSelect(subrx->channel, 3);
  SetRXAPanelPan(subrx->channel, 0.5);
  SetRXAPanelCopy(subrx->channel, 0);
  SetRXAPanelBinaural(subrx->channel, 0);
  SetRXAPanelRun(subrx->channel, 1);

  if(rx->enable_equalizer) {
    SetRXAGrphEQ(subrx->channel, rx->equalizer);
    SetRXAEQRun(subrx->channel, 1);
  } else {
    SetRXAEQRun(subrx->channel, 0);
  }

  subrx_set_agc(rx);

  SetEXTANBRun(subrx->channel, rx->nb);
  SetEXTNOBRun(subrx->channel, rx->nb2);

  SetRXAEMNRPosition(subrx->channel, rx->nr_agc);
  SetRXAEMNRgainMethod(subrx->channel, rx->nr2_gain_method);
  SetRXAEMNRnpeMethod(subrx->channel, rx->nr2_npe_method);
  SetRXAEMNRRun(subrx->channel, rx->nr2);
  SetRXAEMNRaeRun(subrx->channel, rx->nr2_ae);

  SetRXAANRVals(subrx->channel, 64, 16, 16e-4, 10e-7); // defaults
  SetRXAANRRun(subrx->channel, rx->nr);
  SetRXAANFRun(subrx->channel, rx->anf);
  SetRXASNBARun(subrx->channel, rx->snb);

}

void subrx_iq_buffer(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  gint error;
  fexchange0(subrx->channel, rx->iq_input_buffer, subrx->audio_output_buffer, &error);
}

void subrx_volume_changed(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  SetRXAPanelGain1(subrx->channel, rx->volume);
}

void subrx_change_sample_rate(RECEIVER *rx) {
  SUBRX *subrx=(SUBRX *)rx->subrx;
  g_free(subrx->audio_output_buffer);
  subrx->audio_output_buffer=g_new0(gdouble,2*rx->output_samples);
}

void destroy_subrx(RECEIVER *rx) {
g_print("%s\n",__FUNCTION__);
  SUBRX *subrx=(SUBRX *)rx->subrx;
  g_free(subrx->audio_output_buffer);
  g_free(subrx);
}
