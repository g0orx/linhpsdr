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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <gtk/gtk.h>

#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#endif

#include "discovered.h"
#include "bpsk.h"
#include "band.h"
#include "adc.h"
#include "dac.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "radio.h"
#include "main.h"
#include "vfo.h"
#include "ext.h"

int ext_num_pad(void *data) {
  gint val=GPOINTER_TO_INT(data);
  RECEIVER *rx=radio->active_receiver;
  g_print("%s: %d\n",__FUNCTION__,val);
  if(!rx->entering_frequency) {
    rx->entered_frequency=0;
    rx->entering_frequency=true;
  }
  switch(val) {
    case -1: // clear
      rx->entered_frequency=0;
      rx->entering_frequency=false;
      break;
    case -2: // enter
      if(rx->entered_frequency!=0) {
        rx->frequency_a=rx->entered_frequency;
      }
      rx->entering_frequency=false;
      break;
    default:
      rx->entered_frequency=(rx->entered_frequency*10)+val;
      break;
  } 
  update_vfo(rx);
  return 0;
}

int ext_band_select(void *data) {
  int b=GPOINTER_TO_INT(data);
  g_print("%s: %d\n",__FUNCTION__,b);
  set_band(radio->active_receiver,b,-1);
  update_vfo(radio->active_receiver);
  return 0;
}

int ext_vox_changed(void *data) {
  vox_changed((RADIO *)data);
  return 0;
}

int ext_ptt_changed(void *data) {
g_print("ext_ptt_changed\n");
  ptt_changed((RADIO *)data);
  return 0;
}

int ext_set_mox(void *data) {
  MOX_STATE *m=(MOX_STATE *)data;
  set_mox(m->radio,m->state);
  g_free(m);
  return 0;
}

int ext_set_frequency_a(void *data) {
  RX_FREQUENCY *f=(RX_FREQUENCY *)data;
  
  g_mutex_lock(&f->rx->mutex);  
  
  if(f->rx!=NULL) {
    f->rx->frequency_a=f->frequency;
    f->rx->band_a=get_band_from_frequency(f->frequency);
  }
  frequency_changed(f->rx);
  update_vfo(f->rx);
  g_mutex_unlock(&f->rx->mutex);
  
  g_free(f);
  return 0;
}

int ext_set_mode(void *data) {
  MODE *m=(MODE *)data;
  if (m->rx != NULL) {
    m->rx->mode_a = m->mode_a;
  }
  receiver_mode_changed(m->rx, m->rx->mode_a);    
  update_vfo(m->rx);
  g_free(m);
  return 0;
}


int ext_tx_set_ps(void *data) {
  if(radio->transmitter) transmitter_set_ps(radio->transmitter,(uintptr_t)data);
  return 0;
}

/*
int ext_ps_twotone(void *data) {
  ps_twotone((uintptr_t)data);
  return 0;
}
*/

int ext_vfo_update(void *data) {
  RECEIVER *rx=(RECEIVER *)data;
  update_vfo(rx);
  return 0;
}

int ext_vfo_step(void *data) {
  RX_STEP *s=(RX_STEP *)data;
  RECEIVER *rx=s->rx;
  if(rx!=NULL) {
    rx->frequency_a=rx->frequency_a+(rx->step*s->step);
    rx->band_a=get_band_from_frequency(rx->frequency_a);
  }
  update_vfo(s->rx);
  g_free(s);
  return 0;
}

int ext_set_afgain(void *data) {
  RX_GAIN *s=(RX_GAIN *)data;
  RECEIVER *rx=s->rx;
  if(rx!=NULL) {
    rx->volume=s->gain;
  }
  update_vfo(s->rx);
  g_free(s);
  return 0;
}
