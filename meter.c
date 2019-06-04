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
#include "adc.h"
#include "dac.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "radio.h"
#include "meter.h"
#include "main.h"

//static int meter_width;
//static int meter_height;

static int max_level=-200;
static int max_count=0;
static int max_reverse=0;
static gdouble m=0.0;

typedef struct _choice {
  RECEIVER *rx;
  int selection;
} CHOICE;

static gboolean meter_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  int meter_width=gtk_widget_get_allocated_width (widget);
  int meter_height=gtk_widget_get_allocated_height (widget);
//g_print("meter_configure_event_cb: width=%d height=%d\n",meter_width,meter_height);
  if (rx->meter_surface) {
    cairo_surface_destroy (rx->meter_surface);
  }
  rx->meter_surface=gdk_window_create_similar_surface(gtk_widget_get_window(widget),CAIRO_CONTENT_COLOR,meter_width,meter_height);
  return TRUE;
}


static gboolean meter_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  cairo_set_source_surface (cr, rx->meter_surface, 0.0, 0.0);
  cairo_paint (cr);
  return TRUE;
}

static void meter_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  choice->rx->smeter=choice->selection;
}

static gboolean meter_press_event_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  GtkWidget *menu;
  GtkWidget *menu_item;
  CHOICE *choice;
  RECEIVER *rx=(RECEIVER *)data;
  
  switch(event->button) {
    case 1: // LEFT
      menu=gtk_menu_new();
      menu_item=gtk_menu_item_new_with_label("S Meter Peak");
      choice=g_new0(CHOICE,1);
      choice->rx=rx;
      choice->selection=RXA_S_PK;
      g_signal_connect(menu_item,"activate",G_CALLBACK(meter_cb),choice);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      menu_item=gtk_menu_item_new_with_label("S Meter AVERAGE");
      choice=g_new0(CHOICE,1);
      choice->rx=rx;
      choice->selection=RXA_S_AV;
      g_signal_connect(menu_item,"activate",G_CALLBACK(meter_cb),choice);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
      gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
      gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
      break;
  }
  return TRUE;
}

GtkWidget *create_meter_visual(RECEIVER *rx) {

  GtkWidget *meter = gtk_drawing_area_new ();

  g_signal_connect (meter,"configure-event", G_CALLBACK (meter_configure_event_cb), (gpointer)rx);
  g_signal_connect (meter, "draw", G_CALLBACK (meter_draw_cb), (gpointer)rx);
  g_signal_connect (meter, "button-press-event", G_CALLBACK (meter_press_event_cb), (gpointer)rx);
  gtk_widget_set_events (meter, gtk_widget_get_events (meter) | GDK_BUTTON_PRESS_MASK);

  return meter;

}

void update_meter(RECEIVER *rx,gdouble value) {
  char sf[32];
  cairo_t *cr;

  int meter_width=gtk_widget_get_allocated_width (rx->meter);
  int meter_height=gtk_widget_get_allocated_height (rx->meter);


  m=value;

  cr = cairo_create (rx->meter_surface);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_paint (cr);
  cairo_set_font_size(cr, 12);

  double level=value+radio->adc[rx->adc].attenuation;

  double offset=210.0;
  int i;
  double x;
  double y;
  double angle;
  double radians;
  double cx=(double)meter_width-100.0;
  double cy=100.0;
  double radius=cy-20.0;

  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_arc(cr, cx, cy, radius, 216.0*M_PI/180.0, 324.0*M_PI/180.0);
  cairo_stroke(cr);

  cairo_set_line_width(cr, 2.0);
  cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
  cairo_arc(cr, cx, cy, radius+2, 264.0*M_PI/180.0, 324.0*M_PI/180.0);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

  for(i=1;i<10;i++) {
    angle=((double)i*6.0)+offset;
    radians=angle*M_PI/180.0;

    if((i%2)==1) {
      cairo_arc(cr, cx, cy, radius+4, radians, radians);
      cairo_get_current_point(cr, &x, &y);
      cairo_arc(cr, cx, cy, radius, radians, radians);
      cairo_line_to(cr, x, y);
      cairo_stroke(cr);
      sprintf(sf,"%d",i);
      cairo_arc(cr, cx, cy, radius+5, radians, radians);
      cairo_get_current_point(cr, &x, &y);
      cairo_new_path(cr);
      x-=4.0;
      cairo_move_to(cr, x, y);
      cairo_show_text(cr, sf);
    } else {
      cairo_arc(cr, cx, cy, radius+2, radians, radians);
      cairo_get_current_point(cr, &x, &y);
      cairo_arc(cr, cx, cy, radius, radians, radians);
      cairo_line_to(cr, x, y);
      cairo_stroke(cr);
    }
    cairo_new_path(cr);
  }

  for(i=20;i<=60;i+=20) {
    angle=((double)i+54.0)+offset;
    radians=angle*M_PI/180.0;
    cairo_arc(cr, cx, cy, radius+4, radians, radians);
    cairo_get_current_point(cr, &x, &y);
    cairo_arc(cr, cx, cy, radius, radians, radians);
    cairo_line_to(cr, x, y);
    cairo_stroke(cr);

    sprintf(sf,"+%d",i);
    cairo_arc(cr, cx, cy, radius+5, radians, radians);
    cairo_get_current_point(cr, &x, &y);
    cairo_new_path(cr);
    x-=4.0;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, sf);
    cairo_new_path(cr);
  }

  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

  angle=level+127.0+offset;
  radians=angle*M_PI/180.0;
  cairo_arc(cr, cx, cy, radius+8, radians, radians);
  cairo_line_to(cr, cx, cy);
  cairo_stroke(cr);

  cairo_set_source_rgb (cr, 1.0, 1.0, 0.0);
  sprintf(sf,"%d dBm %s",(int)level,rx->smeter==RXA_S_AV?"Av":"Pk");
  cairo_move_to(cr, meter_width-130, meter_height-2);
  cairo_show_text(cr, sf);

  cairo_destroy(cr);
  gtk_widget_queue_draw (rx->meter);

}
