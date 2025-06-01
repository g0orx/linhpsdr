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
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#endif

#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "radio_dialog.h"
#include "transmitter_dialog.h"
#include "puresignal_dialog.h"
#include "pa_dialog.h"
#include "eer_dialog.h"
#include "oc_dialog.h"
#include "xvtr_dialog.h"
#include "receiver_dialog.h"
#include "about_dialog.h"
#include "wideband_dialog.h"
#ifdef MIDI
#include "midi.h"
#include "midi_dialog.h"
#endif

int rx_base=3; // number of tabs before receivers

static GtkWidget *notebook;

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  RADIO *radio=(RADIO *)data;
  int i;

  save_xvtr();
#ifdef MIDI
  configure_midi_device(false);
#endif
  radio->dialog=NULL;
  for(i=0;i<radio->discovered->supported_receivers;i++) {
    if(radio->receiver[i]!=NULL) {
      radio->receiver[i]->dialog=NULL;
      radio->receiver[i]->band_grid=NULL;
      radio->receiver[i]->mode_grid=NULL;
      radio->receiver[i]->filter_frame=NULL;
      radio->receiver[i]->filter_grid=NULL;
    }
  }
  return FALSE;
}

static gboolean switch_page_event(GtkNotebook *notebook,GtkWidget *page,guint page_num,gpointer data) {
  GtkWidget *label=gtk_notebook_get_tab_label(notebook,page);
  const char *text=gtk_label_get_text(GTK_LABEL(label));
  g_print("switch_page: %d %s\n",page_num,text);
  if(strncmp("RX",text,2)==0) {
    int rx=atoi(&text[3]);
    //g_print("switch_page: %d %s rx=%d\n",page_num,text,rx);
    update_receiver_dialog(radio->receiver[rx]);
  }
  if(strncmp("TX",text,2)==0) {
    update_transmitter_dialog(radio->transmitter);
  }
  return TRUE;
}

GtkWidget *create_configure_dialog(RADIO *radio,int tab) {
  int i;
  gchar title[64];

  g_snprintf((gchar *)&title,sizeof(title),"Linux HPSDR: %s %s",radio->discovered->name,inet_ntoa(radio->discovered->info.network.address.sin_addr));

  GtkWidget *dialog=gtk_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(main_window));
  gtk_window_set_title(GTK_WINDOW(dialog),title);
  g_signal_connect (dialog,"delete_event",G_CALLBACK(delete_event),(gpointer)radio);

  GtkWidget *content=gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  notebook=gtk_notebook_new();

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_radio_dialog(radio),gtk_label_new("Radio"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_oc_dialog(radio),gtk_label_new("OC"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_xvtr_dialog(radio),gtk_label_new("XVTR"));

  for(i=0;i<radio->discovered->supported_receivers;i++) {
    if(radio->receiver[i]!=NULL) {
      g_snprintf((gchar *)&title,sizeof(title),"RX-%d",radio->receiver[i]->channel);
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_receiver_dialog(radio->receiver[i]),gtk_label_new(title));
    }
  }

  if(radio->can_transmit) {
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_transmitter_dialog(radio->transmitter),gtk_label_new("TX"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_puresignal_dialog(radio->transmitter),gtk_label_new("Pure Signal"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_pa_dialog(radio),gtk_label_new("PA"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_eer_dialog(radio),gtk_label_new("EER"));
  }

  if(radio->wideband) {
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_wideband_dialog(radio->wideband),gtk_label_new("Wideband"));
  }

#ifdef MIDI
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_midi_dialog(radio),gtk_label_new("MIDI"));
#endif

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),create_about_dialog(radio),gtk_label_new("About"));

  gtk_container_add(GTK_CONTAINER(content),notebook);
  gtk_widget_show_all(dialog);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),tab);

  g_signal_connect (notebook,"switch-page",G_CALLBACK(switch_page_event),(gpointer)radio);

  return dialog;

}

void configure_dialog_set_tab(int tab) {
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),tab);
}
