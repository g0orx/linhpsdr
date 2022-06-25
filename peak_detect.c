/* Copyright (C)
* 2021 - m5evt
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

#include <gtk/gtk.h>
#include "peak_detect.h"

//Put sample on the circular buffer
void peak_queue_put(PEAKDETECTOR *rbuf, gdouble new_value) {
  // Circular buffer, if at the end cycle back to 0
  if (rbuf->queue_in >= rbuf->queue_size) {
    rbuf->queue_in = 0;
  }
  // Write the value
  rbuf->queue[rbuf->queue_in] = new_value;
  // Write location for next time
  rbuf->queue_in++;
}

gdouble get_peak(PEAKDETECTOR *rbuf, gdouble new_val) {
  peak_queue_put(rbuf, new_val);
  
  gdouble peak = 0;
  
  for (int i = 0; i < rbuf->queue_size; i++) {
    if (rbuf->queue[i] > peak) peak = rbuf->queue[i];
  }
  return peak;
}

PEAKDETECTOR *create_peak_detector(guint num_samples, gdouble init_val) {  
g_print("create_peak_detector: size %i \n", num_samples);
  PEAKDETECTOR *rbuf = g_new0(PEAKDETECTOR, 1);  
  
  //Initialise the ring buffer  
  rbuf->queue_size = num_samples;
  
  rbuf->queue = g_new0(gdouble, rbuf->queue_size);

  for (int i = 0; i < num_samples; i++) {
    peak_queue_put(rbuf, init_val);
  }
  
  rbuf->queue_in = 0; 
  
  return rbuf;
}
