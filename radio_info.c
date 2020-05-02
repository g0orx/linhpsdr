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
  GtkWidget *radio_info = gtk_drawing_area_new();

  g_signal_connect(radio_info,"configure-event", G_CALLBACK (info_configure_event_cb), (gpointer)rx);
  g_signal_connect(radio_info, "draw", G_CALLBACK (radio_info_draw_cb), (gpointer)rx);
  //g_signal_connect(radio_info, "button-press-event", G_CALLBACK (radio_info_press_event_cb), (gpointer)rx);
  gtk_widget_set_events(radio_info, gtk_widget_get_events (radio_info) | GDK_BUTTON_PRESS_MASK);

  return radio_info;
}

void update_radio_info(RECEIVER *rx) {
  char sf[32];
  cairo_t *cr;

  int info_width=gtk_widget_get_allocated_width(rx->radio_info);
  int info_height=gtk_widget_get_allocated_height(rx->radio_info);


  cr = cairo_create (rx->radio_info_surface);
  SetColour(cr, BACKGROUND);
  cairo_paint(cr);
  cairo_set_font_size(cr,12);
  cairo_select_font_face(cr, "Noto Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  
  double top_y = 18;
  double y_space = 24;
  double x = 5;
  
  // ********** WARNINGS ****************************
  // HL2 Buffer over/underflow
  if (radio->discovered->device == DEVICE_HERMES_LITE2) {  
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);  
    x+=40;  
  }
  // HERMES/HL2 ADC Clipping
  if ((radio->adc_overload) && (!isTransmitting(radio))) {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, WARNING_ON);     
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);  
  }
  x+=40; 
  // SWR is above a threshold   
  if (radio->transmitter!=NULL && radio->transmitter->swr > radio->swr_alarm_value) {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, WARNING_ON);   
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);       
  }
  x+=40;   
  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    // Temperature   
    if (radio->transmitter->temperature > radio->temperature_alarm_value) {  
      RoundedRectangle(cr, x, top_y, 25.0, 6.0, WARNING_ON);     
    } else {
      RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);     
    }
  }
  // ********** Radio Info/Settings *****************
  x = 5;
  top_y += y_space;
    
  // Rigctl CAT control
  if (rx->cat_control>-1) {  
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_TRUE);  
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);    
  }
  x+=40;  
  // Duplex mode 
  if (radio->duplex) {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_TRUE);
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);    
  }
  x+=40;  
  // Alsa MIDI server  
  if (radio->midi) {  
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_TRUE);  
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);  
  }
  x+=40;
  // Sat mode 
  if (radio->sat_mode==SAT_NONE) {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);    
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_TRUE);
  }
  x+=40;  
  // Mute RX while transmitting (when in duplex mode)
  if (radio->mute_rx_while_transmitting) {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_TRUE);
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);
  }
  x+=40;    
  #ifdef CWDAEMON
  // CWdaemon CWX mode (only HL2 for now)
  if (radio->cwdaemon) {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_TRUE);
  } else {
    RoundedRectangle(cr, x, top_y, 25.0, 6.0, INFO_FALSE);    
  }
  #endif  




  // ********** WARNINGS ****************************
  top_y = 18;
  x = 5;  
  // HL2 Buffer over/underflow 
  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    SetColour(cr, BACKGROUND);
    cairo_move_to(cr, x, top_y + 7);  
    cairo_show_text(cr, "BUF");
    x += 40;
  }
  // HERMES/HL2 ADC Clipping   
  SetColour(cr, BACKGROUND);
  cairo_move_to(cr, x, top_y + 7);  
  cairo_show_text(cr, "ADC");  
  x += 40;  
  // SWR is above a threshold  
  SetColour(cr, BACKGROUND);
  cairo_move_to(cr, x, top_y + 7);  
  cairo_show_text(cr, "SWR");  
  x += 40;  
  // INFO  
  if (radio->discovered->device == DEVICE_HERMES_LITE2) {  
    SetColour(cr, BACKGROUND);
    cairo_move_to(cr, x-3, top_y + 7);  
    cairo_show_text(cr, "TEMP");  
  }
  // ********** Radio Info/Settings *****************
  top_y += y_space;
  x = 5;
  // CWdaemon CWX mode (only HL2+Hermes for now)

  // Rigctl CAT control  
  SetColour(cr, OFF_WHITE);
  cairo_move_to(cr, 5, top_y + 7);    
  cairo_show_text(cr, "CAT");
  // Duplex mode    
  SetColour(cr, OFF_WHITE);
  cairo_move_to(cr, 45, top_y + 7);    
  cairo_show_text(cr, "DUP");
  // Alsa MIDI server
  SetColour(cr, OFF_WHITE);
  cairo_move_to(cr, 83, top_y + 7);    
  cairo_show_text(cr, "MIDI"); 
  // Sat mode
  cairo_move_to(cr, 126, top_y + 7);    
  switch(radio->sat_mode) {
    case SAT_NONE:
      SetColour(cr, DARK_TEXT);
      cairo_show_text(cr, "SAT"); 
      break;
    case SAT_MODE:
      SetColour(cr, OFF_WHITE);
      cairo_show_text(cr, "SAT"); 
      break;
    case RSAT_MODE:
      cairo_move_to(cr, 123, top_y + 7);     
      SetColour(cr, OFF_WHITE);
      cairo_show_text(cr, "RSAT"); 
      break;
  }
  // Mute RX while transmitting    
  SetColour(cr, OFF_WHITE);
  cairo_move_to(cr, 162, top_y + 7);    
  cairo_show_text(cr, "MUTE");

  #ifdef CWDAEMON
  SetColour(cr, OFF_WHITE);
  cairo_move_to(cr, 203, top_y + 7);    
  cairo_show_text(cr, "CWX"); 
  #endif 

  cairo_destroy(cr);
  gtk_widget_queue_draw(rx->radio_info);

}
