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

#ifndef SUBRX_H
#define SUBRX_H

#include "receiver.h"

#define SUBRX_BASE_CHANNEL 16

typedef struct _subrx {
  gint channel;
  gint buffer_size;
  gint fft_size;
  gdouble *audio_output_buffer;
  GMutex mutex;
} SUBRX;

extern void create_subrx(RECEIVER *rx);
extern void destroy_subrx(RECEIVER *rx);
extern void subrx_iq_buffer(RECEIVER *rx);
extern void subrx_frequency_changed(RECEIVER *rx);
extern void subrx_mode_changed(RECEIVER *rx);
extern void subrx_filter_changed(RECEIVER *rx);
extern void subrx_volume_changed(RECEIVER *rx);

#endif
