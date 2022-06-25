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

#ifndef PEAKDETECT_H
#define PEAKDETECT_H

typedef struct _peakdetector {
  gdouble *queue;
  guint queue_in; 
  
  guint queue_size;
} PEAKDETECTOR;

extern PEAKDETECTOR *create_peak_detector(guint num_samples, gdouble init_val);
extern gdouble get_peak(PEAKDETECTOR *rbuf, gdouble new_val);
void peak_queue_put(PEAKDETECTOR *rbuf, gdouble new_value);

#endif
