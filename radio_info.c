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
#include "adc.h"
#include "dac.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "radio.h"
#include "radio_info.h"
#include "main.h"
#include "vfo.h"

typedef struct _choice {
  RECEIVER *rx;
  int selection;
} CHOICE;

static gboolean info_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {

  RECEIVER *rx=(RECEIVER *)data;
  int meter_width = gtk_widget_get_allocated_width(widget);
  int meter_height = gtk_widget_get_allocated_height(widget);
  if (rx->radio_info_surface) {
    cairo_surface_destroy (rx->radio_info_surface);
  }
  rx->radio_info_surface=gdk_window_create_similar_surface(gtk_widget_get_window(widget),CAIRO_CONTENT_COLOR,meter_width,meter_height);
  
  return TRUE;
}


static gboolean radio_info_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  cairo_set_source_surface(cr, rx->radio_info_surface, 0.0, 0.0);
  cairo_paint(cr);
  return TRUE;
}

static void radio_info_cb(GtkWidget *menu_item,gpointer data) {
  /*
  CHOICE *choice=(CHOICE *)data;
  choice->rx->smeter=choice->selection;
  */
}

static gboolean radio_info_press_event_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  /*
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
  */
  return TRUE;
}

GtkWidget *create_radio_info_visual(RECEIVER *rx) {
  RADIO_INFO *info=g_new(RADIO_INFO,1);
  info->radio_info=gtk_layout_new(NULL,NULL);
  gtk_widget_set_name(info->radio_info,"info");

  int x=0;
  int y=10;

  // ********** WARNINGS ****************************
  // HL2 Buffer over/underflow
  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    info->buffer_overflow_underflow_b=gtk_toggle_button_new_with_label("BUF");
    gtk_widget_set_name(info->buffer_overflow_underflow_b,"info-warning");
    gtk_layout_put(GTK_LAYOUT(info->radio_info),info->buffer_overflow_underflow_b,x,y);
    x+=40;
  }

  // HERMES/HL2 ADC Clipping
  info->adc_overload_b=gtk_toggle_button_new_with_label("ADC");
  gtk_widget_set_name(info->adc_overload_b,"info-warning");
  gtk_layout_put(GTK_LAYOUT(info->radio_info),info->adc_overload_b,x,y);
  x+=40;

  // SWR is above a threshold
  info->swr_b=gtk_toggle_button_new_with_label("SWR");
  gtk_widget_set_name(info->swr_b,"info-warning");
  gtk_layout_put(GTK_LAYOUT(info->radio_info),info->swr_b,x,y);
  x+=40;

  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    info->temp_b=gtk_toggle_button_new_with_label("TEMP");
    gtk_widget_set_name(info->temp_b,"info-warning");
    gtk_layout_put(GTK_LAYOUT(info->radio_info),info->temp_b,x,y);
    x+=40;
  }


  x=0;
  y=36;

  // CAT
  info->cat_b=gtk_toggle_button_new_with_label("CAT");
  gtk_widget_set_name(info->cat_b,"info-button");
  gtk_layout_put(GTK_LAYOUT(info->radio_info),info->cat_b,x,y);

  x+=40;

  // MIDI
  info->midi_b=gtk_toggle_button_new_with_label("MIDI");
  gtk_widget_set_name(info->midi_b,"info-button");
  gtk_layout_put(GTK_LAYOUT(info->radio_info),info->midi_b,x,y);

#ifdef CWDAEMON
  x+=40;
  info->cwdaemon_b=gtk_toggle_button_new_with_label("CWX");
  gtk_widget_set_name(info->cwdaemon_b,"info-button");
  gtk_layout_put(GTK_LAYOUT(info->radio_info),info->cwdaemon_b,x,y);
#endif

  g_object_set_data ((GObject *)info->radio_info,"info_data",info);

  return info->radio_info;
}

void update_radio_info(RECEIVER *rx) {


  RADIO_INFO *info=(RADIO_INFO *)g_object_get_data((GObject *)rx->radio_info,"info_data");

  if(info==NULL) return;

  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->adc_overload_b),radio->adc_overload && (!isTransmitting(radio)));
  }


  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->temp_b),radio->transmitter->temperature>radio->temperature_alarm_value);
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->swr_b),radio->transmitter->swr>radio->swr_alarm_value);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->midi_b),radio->midi!=0);

#ifdef CWDAEMON
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->cwdaemon_b),radio->cwdaemon);
#endif

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->temp_b),radio->transmitter->temperature>radio->temperature_alarm_value);
}
