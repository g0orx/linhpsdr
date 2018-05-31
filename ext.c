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

#include "discovered.h"
#include "band.h"
#include "adc.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "radio.h"
#include "main.h"
#include "vfo.h"
#include "ext.h"

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
  MOX *m=(MOX *)data;
  set_mox(m->radio,m->state);
  g_free(m);
  return 0;
}

int ext_set_frequency_a(void *data) {
  RX_FREQUENCY *f=(RX_FREQUENCY *)data;
  if(f->rx!=NULL) {
    f->rx->frequency_a=f->frequency;
    f->rx->band_a=get_band_from_frequency(f->frequency);
  }
  update_vfo(f->rx);
  g_free(f);
  return 0;
}

int ext_tx_set_ps(void *data) {
  transmitter_set_ps(radio->transmitter,(uintptr_t)data);
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
