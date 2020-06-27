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
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wdsp.h>

#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "protocol1.h"
#include "protocol2.h"
#include "audio.h"
#include "band.h"


static void pa_value_changed_cb(GtkWidget *widget, gpointer data) {
  BAND *band=(BAND *)data;

  band->pa_calibration=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

GtkWidget *create_pa_dialog(RADIO *r) {
  int i;

  GtkWidget *pa_frame=gtk_frame_new("PA Calibration");
  GtkWidget *pa_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(pa_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(pa_grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(pa_grid),10);
  gtk_container_add(GTK_CONTAINER(pa_frame),pa_grid);

  for(i=0;i<bandGen;i++) {
    BAND *band=band_get_band(i);

    GtkWidget *band_label=gtk_label_new(band->title);
    gtk_widget_show(band_label);
    gtk_grid_attach(GTK_GRID(pa_grid),band_label,(i%3)*2,i/3,1,1);

    GtkWidget *pa_r=gtk_spin_button_new_with_range(38.8,100.0,0.1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pa_r),(double)band->pa_calibration);
    gtk_widget_show(pa_r);
    gtk_grid_attach(GTK_GRID(pa_grid),pa_r,((i%3)*2)+1,i/3,1,1);
    g_signal_connect(pa_r,"value_changed",G_CALLBACK(pa_value_changed_cb),band);
  }

  return pa_frame;
}
