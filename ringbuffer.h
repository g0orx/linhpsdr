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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

typedef struct _ringbuffer_l {
  glong *queue;
  glong queue_in; 
  glong queue_out;
  
  guint queue_size;
} RINGBUFFERL;

extern RINGBUFFERL *create_long_ringbuffer(glong queue_elements, glong init_val);

extern int queue_put(RINGBUFFERL *rbuf, glong new_value);
extern int queue_get(RINGBUFFERL *rbuf, long *old);

#endif
