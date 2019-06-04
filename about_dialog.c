/* Copyright (C)
* 2017 - John Melton, G0ORX/N6LYT
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wdsp.h>

#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#endif

#include "discovered.h"
#include "receiver.h"
#include "transmitter.h"
#include "receiver.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "version.h"
#include "about_dialog.h"

static GtkWidget *label;

GtkWidget *create_about_dialog(RADIO *r) {
  int i;
  char text[2048];
  char addr[64];
  char interface_addr[64];
  char protocol[32];

  GtkWidget *grid=gtk_grid_new();

  gtk_grid_set_column_homogeneous(GTK_GRID(grid),TRUE);
  gtk_grid_set_column_spacing (GTK_GRID(grid),4);

  int row=0;

  snprintf(text,sizeof(text),"linHPSDR by John Melton G0ORX/N6LYT");
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;
  snprintf(text,sizeof(text),"With help from:");
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;
  snprintf(text,sizeof(text),"    Steve Wilson, KA6S, RIGCTL (CAT over TCP) and Testing");
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;
  snprintf(text,sizeof(text),"    Ken Hopper, N9VV, Testing and Documentation");
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;

  snprintf(text,sizeof(text),"Build date: %s", build_date);
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;

  snprintf(text,sizeof(text),"Build version: %s", version);
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;

  snprintf(text,sizeof(text),"WDSP v%d.%02d", GetWDSPVersion()/100, GetWDSPVersion()%100);
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;


  switch(r->discovered->protocol) {
    case PROTOCOL_1:
      snprintf(protocol,sizeof(protocol),"Protocol: 1");
      break;
    case PROTOCOL_2:
      snprintf(protocol,sizeof(protocol),"Protocol: 2");
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      snprintf(protocol,sizeof(protocol),"Protocol: SOAPYSDR");
      break;
#endif
  }
  snprintf(text,sizeof(text),"Device: %s\n%s v%d.%d",r->discovered->name,protocol,r->discovered->software_version/10,r->discovered->software_version%10);
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;

  switch(r->discovered->protocol) {
    case PROTOCOL_1:
    case PROTOCOL_2:
#ifdef USBOZY
      if(d->device==DEVICE_OZY) {
        snprintf(text,sizeof(text),"Device OZY: USB /dev/ozy");
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;
      } else {
#endif
        
        strcpy(addr,inet_ntoa(r->discovered->info.network.address.sin_addr));
        strcpy(interface_addr,inet_ntoa(r->discovered->info.network.interface_address.sin_addr));
        snprintf(text,sizeof(text),"Device Mac Address: %02X:%02X:%02X:%02X:%02X:%02X",
                r->discovered->info.network.mac_address[0],
                r->discovered->info.network.mac_address[1],
                r->discovered->info.network.mac_address[2],
                r->discovered->info.network.mac_address[3],
                r->discovered->info.network.mac_address[4],
                r->discovered->info.network.mac_address[5]);
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;
        snprintf(text,sizeof(text),"Device IP Address: %s on %s (%s)",addr,r->discovered->info.network.interface_name,interface_addr);
  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,1,1);
  row++;

#ifdef USBOZY
      }
#endif
      break;
  }


  return grid;

}
