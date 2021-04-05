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

typedef struct _RX_FREQUENCY {
  RECEIVER *rx;
  long long frequency;
} RX_FREQUENCY;

typedef struct _RX_STEP {
  RECEIVER *rx;
  int step;
} RX_STEP;

typedef struct _RX_GAIN {
  RECEIVER *rx;
  double gain;
} RX_GAIN;

typedef struct _MOX_STATE {
  RADIO *radio;
  gboolean state;
} MOX_STATE;

typedef struct _MODE {
  RECEIVER *rx;
  int mode_a;
  gboolean state;
} MODE;


extern int ext_num_pad(void *data);
extern int ext_band_select(void *data);
extern int ext_vox_changed(void *data);
extern int ext_ptt_changed(void *data);
extern int ext_set_mox(void *data);
extern int ext_set_frequency_a(void *data);
extern int ext_set_mode(void *data);
extern int ext_tx_set_ps(void *data);
//extern int ext_ps_twotone(void *data);
extern int ext_vfo_update(void *data);
extern int ext_vfo_step(void *data);
extern int ext_set_afgain(void *data);

