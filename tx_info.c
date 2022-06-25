/* Copyright (C)
* 2021 - m5evt
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
#include <stdlib.h>

#include "mode.h"
#include "discovered.h"
#include "bpsk.h"
#include "adc.h"
#include "dac.h"
#include "wideband.h"
#include "receiver.h"
#include "transmitter.h"
#include "radio.h"
#include "vfo.h"
#include "level_meter.h"
#include "tx_info.h"

#include "configure_dialog.h"
#include "main.h"
#ifdef SOAPYSDR
#include "soapy_protocol.h"
#endif

#include "mic_level.h"
#include "mic_gain.h"
#include "drive_level.h"

#include "tx_info_meter.h"


static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  TRANSMITTER *tx = (TRANSMITTER *)data;
  tx->tx_info = NULL;
  return FALSE;
}

GtkWidget *create_tx_info(TRANSMITTER *tx) {
  GtkWidget *dialog = gtk_dialog_new();
  
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
  g_signal_connect (dialog, "delete_event", G_CALLBACK(delete_event), (gpointer)tx);
  
  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  GtkWidget *grid=gtk_grid_new();
  
  gtk_grid_set_row_homogeneous(GTK_GRID(grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_row_spacing(GTK_GRID(grid),10);
  gtk_grid_set_column_spacing(GTK_GRID(grid),10);

  int row=0;
  int col=0;
  // Remember, after initialisation here, filter board could change. This
  // could cause problems in the update function.
  // So must be set once here, then if filter board is changed, window
  // must be closed and opened again to refresh
  if (radio->filter_board == N2ADR) {
    tx->num_tx_info_meters = 2;
  }
  else if (radio->filter_board == HL2_MRF101) {
    tx->num_tx_info_meters = 4;
  } 

  for (int i = 0; i < 4; i++) {
    tx->tx_info_meter[i] = create_tx_info_meter();
    gtk_grid_attach(GTK_GRID(grid), tx->tx_info_meter[i]->tx_meter_drawing, col, row, 1, 1);
    row++;
  }

  configure_meter(tx->tx_info_meter[0], "HL2 temperature", 0, 55);
  
  if (radio->filter_board == N2ADR) {  
    configure_meter(tx->tx_info_meter[1], "Power out", 0, 6);     
  }
  else {
    // Default 100 W
    configure_meter(tx->tx_info_meter[1], "Power out", 0, 110);  
  }    
      
  if (tx->num_tx_info_meters == 4) {
    configure_meter(tx->tx_info_meter[2], "MRF101 temperature", 0, 55);  
    configure_meter(tx->tx_info_meter[3], "MRF101 current", 0, 6.0);  
  } 

  gtk_container_add(GTK_CONTAINER(content),grid);
  
  gtk_widget_show_all(dialog);
    
  return dialog;
}

void update_tx_info(TRANSMITTER *tx) { 
  // HL2 temp
  update_tx_info_meter(tx->tx_info_meter[0], tx->temperature, tx->temperature);  

  // Power out
  update_tx_info_meter(tx->tx_info_meter[1], tx->fwd, tx->fwd_peak);  
 
  if (tx->num_tx_info_meters == 4) {
  // MRF101 temp
    if (radio->hl2 != NULL) {
      update_tx_info_meter(tx->tx_info_meter[2], radio->hl2->mrf101_temp, radio->hl2->mrf101_temp);  
      // MRF101 current
      update_tx_info_meter(tx->tx_info_meter[3], radio->hl2->mrf101_current, radio->hl2->current_peak);    
    } 
  }
}
