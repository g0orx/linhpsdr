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

#ifndef WIDEBAND_H
#define WIDEBAND_H

typedef struct _wideband {
  gint channel; // WDSP channel
  gint adc;
  gint buffer_size;
  gint fft_size;

  guint32 sequence;
  gdouble *input_buffer;
  gfloat *pixel_samples;

  gint update_timer_id;

  gint samples;
  gint pixels;
  gint fps;


  GtkWidget *window;
  GtkWidget *table;

  gint window_x;
  gint window_y;
  gint window_width;
  gint window_height;

  GtkWidget *panadapter;
  gint panadapter_width;
  gint panadapter_height;
  gint panadapter_resize_width;
  gint panadapter_resize_height;
  guint panadapter_resize_timer;
  cairo_surface_t *panadapter_surface;

  gint panadapter_low;
  gint panadapter_high;

  GtkWidget *waterfall;
  gint waterfall_width;
  gint waterfall_height;
  gint waterfall_resize_width;
  gint waterfall_resize_height;
  guint waterfall_resize_timer;
  GdkPixbuf *waterfall_pixbuf;

  gint waterfall_low;
  gint waterfall_high;
  gboolean waterfall_automatic;
  gint64 waterfall_frequency;
  gint waterfall_sample_rate;
  
  gdouble hz_per_pixel;

  gboolean has_moved;
  gint last_x;

  GtkWidget *dialog;

} WIDEBAND;

extern WIDEBAND *create_wideband(int channel);
extern void wideband_init_analyzer(WIDEBAND *w);
extern void add_wideband_sample(WIDEBAND *w,double sample);
extern gboolean wideband_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
extern gboolean wideband_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
extern gboolean wideband_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data);
extern gboolean wideband_scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data);
extern void wideband_save_state(WIDEBAND *w);
extern void reset_wideband_buffer_index(WIDEBAND *w);

#endif
