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

#ifndef DIVMIXER_H
#define DIVMIXER_H

#include "receiver.h"

typedef struct _divmixer {
  gint id; // WDSP mixer ID (limited to 2)

  gdouble *iq_output_buffer;
  
  gdouble **iq_input;
  
  gint iq_buffer_size;
  
  gdouble *i_rotate;
  gdouble *q_rotate;
  
  gint num_streams;
  
  gdouble gain;
  gdouble phase;
  
  gdouble gain_fine;
  gdouble phase_fine;  
    
  gboolean calibrate_gain;
  gboolean flip;    
    
    
  RECEIVER *rx_visual;
  RECEIVER *rx_hidden;    

} DIVMIXER;

extern DIVMIXER *create_diversity_mixer(int id, RECEIVER *rxa, RECEIVER *rxb);
extern void diversity_add_buffer(DIVMIXER *dmix);
extern void diversity_mix_full_buffers(DIVMIXER *dmix);
extern void set_gain_phase(DIVMIXER *dmix);
extern void SetNumStreams(DIVMIXER *dmix);
extern void diversity_mix_calibrate_gain_visuals(DIVMIXER *dmix);

#endif
