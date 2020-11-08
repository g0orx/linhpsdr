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

#ifndef _OLD_PROTOCOL_H
#define _OLD_PROTOCOL_H

extern void protocol1_stop();
extern void protocol1_run();

extern void protocol1_init(RADIO *r);
extern void protocol1_set_mic_sample_rate(int rate);

extern void protocol1_process_local_mic(RADIO *r);
extern void protocol1_audio_samples(RECEIVER *rx,short left_audio_sample,short right_audio_sample);
extern void protocol1_iq_samples(int isample,int qsample);
extern void protocol1_eer_iq_samples(int isample,int qsample,int lasample,int rasample);
extern gboolean protocol1_is_running();
#endif
