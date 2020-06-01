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
#include <wdsp.h>

#include "button_text.h"
#include "bpsk.h"
#include "discovered.h"
#include "mode.h"
#include "filter.h"
#include "band.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "receiver_dialog.h"
#include "vfo.h"
#include "audio.h"
#include "main.h"
/*
static gboolean close_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  w->dialog=NULL;
  return TRUE;
}
*/
/*
static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  w->dialog=NULL;
  return FALSE;
}
*/
static void panadapter_high_value_changed_cb(GtkWidget *widget, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  w->panadapter_high=gtk_range_get_value(GTK_RANGE(widget));
}

static void panadapter_low_value_changed_cb(GtkWidget *widget, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  w->panadapter_low=gtk_range_get_value(GTK_RANGE(widget));
}


static void waterfall_high_value_changed_cb(GtkWidget *widget, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  w->waterfall_high=gtk_range_get_value(GTK_RANGE(widget));
}

static void waterfall_low_value_changed_cb(GtkWidget *widget, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  w->waterfall_low=gtk_range_get_value(GTK_RANGE(widget));
}

static void waterfall_automatic_cb(GtkWidget *widget, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  w->waterfall_automatic=w->waterfall_automatic==1?0:1;
}

GtkWidget *create_wideband_dialog(WIDEBAND *w) {
  int col=0;
  int row=0;

  GtkWidget *grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(grid),5);

  row=0;
  col=0;

  GtkWidget *panadapter_frame=gtk_frame_new("Panadapter");
  GtkWidget *panadapter_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(panadapter_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(panadapter_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(panadapter_frame),panadapter_grid);
  gtk_grid_attach(GTK_GRID(grid),panadapter_frame,col,row++,1,1);

  GtkWidget *high_label=gtk_label_new("High:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),high_label,0,1,1,1);

  GtkWidget *panadapter_high_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,-200.0, 20.0, 1.00);
  gtk_widget_set_size_request(panadapter_high_scale,200,30);
  gtk_range_set_value (GTK_RANGE(panadapter_high_scale),w->panadapter_high);
  gtk_widget_show(panadapter_high_scale);
  g_signal_connect(G_OBJECT(panadapter_high_scale),"value_changed",G_CALLBACK(panadapter_high_value_changed_cb),w);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_high_scale,1,1,1,1);

  GtkWidget *waterfall_low_label=gtk_label_new("Low:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),waterfall_low_label,0,2,1,1);

  GtkWidget *panadapter_low_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,-200.0, 20.0, 1.00);
  gtk_widget_set_size_request(panadapter_low_scale,200,30);
  gtk_range_set_value (GTK_RANGE(panadapter_low_scale),w->panadapter_low);
  gtk_widget_show(panadapter_low_scale);
  g_signal_connect(G_OBJECT(panadapter_low_scale),"value_changed",G_CALLBACK(panadapter_low_value_changed_cb),w);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_low_scale,1,2,1,1);

  GtkWidget *waterfall_frame=gtk_frame_new("Waterfall");
  GtkWidget *waterfall_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(waterfall_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(waterfall_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(waterfall_frame),waterfall_grid);
  gtk_grid_attach(GTK_GRID(grid),waterfall_frame,col,row++,1,1);

  GtkWidget *waterfall_high_label=gtk_label_new("High:");
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_high_label,0,0,1,1);

  GtkWidget *waterfall_high_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,-200.0, 20.0, 1.00);
  gtk_widget_set_size_request(waterfall_high_scale,200,30);
  gtk_range_set_value (GTK_RANGE(waterfall_high_scale),w->waterfall_high);
  gtk_widget_show(waterfall_high_scale);
  g_signal_connect(G_OBJECT(waterfall_high_scale),"value_changed",G_CALLBACK(waterfall_high_value_changed_cb),w);
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_high_scale,1,0,1,1);

  GtkWidget *low_label=gtk_label_new("Low:");
  gtk_grid_attach(GTK_GRID(waterfall_grid),low_label,0,1,1,1);

  GtkWidget *waterfall_low_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,-200.0, 20.0, 1.00);
  gtk_widget_set_size_request(waterfall_low_scale,200,30);
  gtk_range_set_value (GTK_RANGE(waterfall_low_scale),w->waterfall_low);
  gtk_widget_show(waterfall_low_scale);
  g_signal_connect(G_OBJECT(waterfall_low_scale),"value_changed",G_CALLBACK(waterfall_low_value_changed_cb),w);
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_low_scale,1,1,1,1);

  GtkWidget *waterfall_automatic=gtk_check_button_new_with_label("Waterfall Automatic");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (waterfall_automatic), w->waterfall_automatic);
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_automatic,0,2,2,1);
  g_signal_connect(waterfall_automatic,"toggled",G_CALLBACK(waterfall_automatic_cb),w);

  return grid;
}
