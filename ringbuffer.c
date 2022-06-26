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
#include "ringbuffer.h"

//Put sample on the ring buffer
int queue_put(RINGBUFFERL *rbuf, glong new_value) {
  if(rbuf->queue_in == ((rbuf->queue_out - 1 + rbuf->queue_size) % rbuf->queue_size)) {
    return -1; // Queue Full
  }
  rbuf->queue[rbuf->queue_in] = new_value;
  rbuf->queue_in = (rbuf->queue_in + 1) % rbuf->queue_size;
  return 0; // No errors
}

//Get sample from the ring buffer
int queue_get(RINGBUFFERL *rbuf, long *old) {  
  // Queue Empty - nothing to get
  if(rbuf->queue_in == rbuf->queue_out) return -1; 

  *old = rbuf->queue[rbuf->queue_out];
  rbuf->queue_out = (rbuf->queue_out + 1) % rbuf->queue_size;
  return 0; // No errors
}

RINGBUFFERL *create_long_ringbuffer(glong queue_elements, glong init_val) {
g_print("create_long_ringbuffer: size %ld \n", queue_elements);
  RINGBUFFERL *rbuf = g_new0(RINGBUFFERL, 1);  
  
  //Initialise the ring buffer  
  rbuf->queue_size = queue_elements + 1;
  
  rbuf->queue = g_new0(glong, rbuf->queue_size);

  for (int i = 0; i < queue_elements; i++) {
    queue_put(rbuf, init_val);
  }
  
  rbuf->queue_in = 0;
  rbuf->queue_out = 0;  
  
  return rbuf;
}
