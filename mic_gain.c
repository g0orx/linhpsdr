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

#include <gtk/gtk.h>
#include <math.h>

#include <wdsp.h>

#include "discovered.h"
#include "bpsk.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "receiver.h"
#include "transmitter.h"
#include "radio.h"
#include "main.h"
#include "mic_gain.h"

static char *title="Microphone Gain";

static gboolean mic_gain_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  return TRUE;
}

static gboolean mic_gain_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  double x;
  cairo_text_extents_t extents;
  char t[32];

  int width=gtk_widget_get_allocated_width(widget);
  int height=gtk_widget_get_allocated_height(widget);
  double bar_width=(double)width-10;

  cairo_set_line_width(cr,1.0);

  cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
  cairo_rectangle(cr,0,0,width,height);
  cairo_fill(cr);

  double v=radio->transmitter->mic_gain+10.0; // move from rabd -10..50 to range 0..60
  x=(bar_width/60.0)*v;

  cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
  cairo_rectangle(cr,5,0,x,(height/2)-1);
  cairo_fill(cr);

  cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
  cairo_move_to(cr,5,height/2);
  cairo_line_to(cr,width-5,height/2);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
  x=(10.0/60.0)*(double)bar_width;
  cairo_move_to(cr,x+5.0,(double)(height/2)-8.0);
  cairo_line_to(cr,x+5.0,height/2-1);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
  cairo_set_font_size(cr,10);
  cairo_text_extents(cr, title, &extents);
  sprintf(t,"%s (%ddB)",title,(int)radio->transmitter->mic_gain);
  cairo_move_to(cr,(5+width/2)-(extents.width/2.0),height-2);
  cairo_show_text(cr,t);

  return TRUE;
}

static gboolean mic_gain_press_event_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  int width=gtk_widget_get_allocated_width(widget)-10;
  double x=event->x-5.0;
  x=((x/(double)width)*60.0)-10.0;
  tx->mic_gain=x;
  if(tx->mic_gain<-10.0) tx->mic_gain=-10.0;
  if(tx->mic_gain>50.0) tx->mic_gain=50.0;
  SetTXAPanelGain1(tx->channel,pow(10.0, tx->mic_gain / 20.0));
  gtk_widget_queue_draw(radio->mic_gain);
  return TRUE;
}

static gboolean mic_gain_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  if(event->direction==GDK_SCROLL_UP) {
    tx->mic_gain+=1.0;
  } else {
    tx->mic_gain-=1.0;
  }
  if(tx->mic_gain<-10.0) tx->mic_gain=-10.0;
  if(tx->mic_gain>50.0) tx->mic_gain=50.0;
  SetTXAPanelGain1(tx->channel,pow(10.0, tx->mic_gain / 20.0));
  gtk_widget_queue_draw(radio->mic_gain);
  return TRUE;
}

static gboolean enter (GtkWidget *ebox, GdkEventCrossing *event) {
   //gdk_window_set_cursor(gtk_widget_get_window(ebox),gdk_cursor_new(GDK_DOUBLE_ARROW));
   gdk_window_set_cursor(gtk_widget_get_window(ebox),gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW));
   return FALSE;
}


static gboolean leave (GtkWidget *ebox, GdkEventCrossing *event) {
   gdk_window_set_cursor(gtk_widget_get_window(ebox),gdk_cursor_new(GDK_ARROW));
   return FALSE;
}

GtkWidget *create_mic_gain(TRANSMITTER *tx) {

  radio->mic_gain=gtk_drawing_area_new();
  //gtk_widget_set_size_request(radio->mic_gain, 10, 100);

  g_signal_connect(radio->mic_gain,"configure-event",G_CALLBACK(mic_gain_configure_event_cb),(gpointer)tx);
  g_signal_connect(radio->mic_gain,"draw",G_CALLBACK(mic_gain_draw_cb),(gpointer)tx);
  g_signal_connect(radio->mic_gain,"enter-notify-event",G_CALLBACK (enter),NULL);
  g_signal_connect(radio->mic_gain,"leave-notify-event",G_CALLBACK (leave),NULL);
  g_signal_connect(radio->mic_gain,"button-press-event",G_CALLBACK(mic_gain_press_event_cb),(gpointer)tx);
  g_signal_connect(radio->mic_gain,"scroll_event",G_CALLBACK(mic_gain_scroll_event_cb),(gpointer)tx);
  gtk_widget_set_events (radio->mic_gain, gtk_widget_get_events (radio->mic_gain)
                     | GDK_BUTTON_PRESS_MASK
                     | GDK_SCROLL_MASK
                     | GDK_ENTER_NOTIFY_MASK
                     | GDK_LEAVE_NOTIFY_MASK);

  return radio->mic_gain;
}
