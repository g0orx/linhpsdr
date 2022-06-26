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
#include "configure_dialog.h"

#ifdef MIDI
static gboolean midi_b_press_cb(GtkWidget *widget,GdkEvent *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(radio->dialog==NULL) {
    int tab=rx_base+radio->receivers+(radio->can_transmit?4:0);
    radio->dialog=create_configure_dialog(radio,tab);
  }
  return TRUE;
}
#endif

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

#ifdef MIDI
  // MIDI
  info->midi_b=gtk_toggle_button_new_with_label("MIDI");
  gtk_widget_set_name(info->midi_b,"info-button");
  g_signal_connect(info->midi_b, "button_press_event", G_CALLBACK(midi_b_press_cb),rx);
  gtk_layout_put(GTK_LAYOUT(info->radio_info),info->midi_b,x,y);

  x+=40;
#endif

#ifdef CWDAEMON
  info->cwdaemon_b=gtk_toggle_button_new_with_label("CWX");
  gtk_widget_set_name(info->cwdaemon_b,"info-button");
  gtk_layout_put(GTK_LAYOUT(info->radio_info),info->cwdaemon_b,x,y);
#endif

  g_object_set_data ((GObject *)info->radio_info,"info_data",info);

  return info->radio_info;
}

extern int midi_rx;

void update_radio_info(RECEIVER *rx) {


  RADIO_INFO *info=(RADIO_INFO *)g_object_get_data((GObject *)rx->radio_info,"info_data");

  if(info==NULL) return;

  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->adc_overload_b),radio->adc_overload && (!isTransmitting(radio)));
  }


  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->temp_b),radio->transmitter->temperature>radio->temperature_alarm_value);
  }

  if (radio->transmitter!=NULL) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->swr_b),radio->transmitter->swr>radio->swr_alarm_value);
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->cat_b), rx->cat_client_connected);

#ifdef MIDI
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->midi_b),radio->midi_enabled && (radio->receiver[midi_rx]->channel==rx->channel));
#endif

#ifdef CWDAEMON
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->cwdaemon_b),radio->cwdaemon);
#endif

  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->temp_b),radio->transmitter->temperature>radio->temperature_alarm_value);
  }
}
