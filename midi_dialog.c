/* Copyright (C)
* 2020 - John Melton, G0ORX/N6LYT
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
#include <termios.h>

#include "discovered.h"
#include "bpsk.h"
#include "mode.h"
#include "filter.h"
#include "band.h"
#include "receiver.h"
#include "transmitter.h"
#include "receiver.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "midi.h"
#include "midi_dialog.h"

static GtkWidget *filename;

static gboolean midi_enable_cb(GtkWidget *widget,gpointer data) {
  RADIO *r=(RADIO *)data;
  r->midi_enabled=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
  if(r->midi_enabled) {
    strcpy(r->midi_filename,gtk_entry_get_text(GTK_ENTRY(filename)));
    int rc=MIDIstartup(r->midi_filename);
  } else {
    MIDIstop(r);
  }
  return TRUE;
}

static void debug_cb(GtkWidget *widget, gpointer data) {
  RADIO *r=(RADIO *)data;
  midi_debug=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
}

GtkWidget *create_midi_dialog(RADIO *r) {
  int i;
  int col=0;
  int row=0;

  GtkWidget *midi_frame=gtk_frame_new("MIDI");
  GtkWidget *grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(grid),5);
  gtk_container_add(GTK_CONTAINER(midi_frame),grid);

  row=0;
  col=0;

  GtkWidget *midi_enable_b=gtk_check_button_new_with_label("MIDI Enable");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (midi_enable_b), r->midi_enabled);
  gtk_grid_attach(GTK_GRID(grid),midi_enable_b,col,row,1,1);
  g_signal_connect(midi_enable_b,"toggled",G_CALLBACK(midi_enable_cb),r);

  row++;

  col++;

  filename=gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(filename),strlen(r->midi_filename));
  gtk_entry_set_text(GTK_ENTRY(filename),r->midi_filename);
  gtk_grid_attach(GTK_GRID(grid),filename,col,row,1,1);

  col=0;
  row++;

  GtkWidget *debug_b=gtk_check_button_new_with_label("MIDI Debug");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (debug_b), midi_debug);
  gtk_grid_attach(GTK_GRID(grid),debug_b,col,row,1,1);
  g_signal_connect(debug_b,"toggled",G_CALLBACK(debug_cb),r);


  return midi_frame;
}
