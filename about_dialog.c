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

#include <SoapySDR/Device.h>

#include "discovered.h"
#include "receiver.h"
#include "transmitter.h"
#include "receiver.h"
#include "wideband.h"
#include "adc.h"
#include "radio.h"
#include "version.h"
#include "about_dialog.h"

static GtkWidget *label;

GtkWidget *create_about_dialog(RADIO *r) {
  int i;
  char text[2048];
  char addr[64];
  char interface_addr[64];

  GtkWidget *grid=gtk_grid_new();

  gtk_grid_set_column_homogeneous(GTK_GRID(grid),TRUE);
  //gtk_grid_set_row_homogeneous(GTK_GRID(grid),TRUE);
  gtk_grid_set_column_spacing (GTK_GRID(grid),4);
  //gtk_grid_set_row_spacing (GTK_GRID(grid),4);

  int row=0;

  int lines=0;

  sprintf(text,"\n\nlinHPSDR by John Melton G0ORX/N6LYT");
  lines++;
  sprintf(text,"%s\n\nWith help from:",text);
  lines++;
  sprintf(text,"%s\n    Steve Wilson, KA6S, RIGCTL (CAT over TCP) and Testing",text);
  lines++;
  sprintf(text,"%s\n    Ken Hopper, N9VV, Testing and Documentation",text);
  lines++;
  lines++;

  sprintf(text,"%s\n\nBuild date: %s", text, build_date);
  lines++;

  sprintf(text,"%s\nBuild version: %s", text, version);
  lines++;

  sprintf(text,"%s\n\nWDSP v%d.%02d", text, GetWDSPVersion()/100, GetWDSPVersion()%100);
  lines++;

  sprintf(text,"%s\n\nDevice: %s\nProtocol: %s v%d.%d",text,r->discovered->name,r->discovered->protocol==PROTOCOL_1?"1":"2",r->discovered->software_version/10,r->discovered->software_version%10);
  lines++;

  switch(r->discovered->protocol) {
    case PROTOCOL_1:
    case PROTOCOL_2:
#ifdef USBOZY
      if(d->device==DEVICE_OZY) {
        sprintf(text,"%s\nDevice OZY: USB /dev/ozy",text,r->protocol==PROTOCOL_1?"1":"2",r->discovered->software_version/10,r->discovered->software_version%10);
      } else {
#endif
        
        strcpy(addr,inet_ntoa(r->discovered->info.network.address.sin_addr));
        strcpy(interface_addr,inet_ntoa(r->discovered->info.network.interface_address.sin_addr));
        sprintf(text,"%s\nDevice Mac Address: %02X:%02X:%02X:%02X:%02X:%02X",text,
                r->discovered->info.network.mac_address[0],
                r->discovered->info.network.mac_address[1],
                r->discovered->info.network.mac_address[2],
                r->discovered->info.network.mac_address[3],
                r->discovered->info.network.mac_address[4],
                r->discovered->info.network.mac_address[5]);
        sprintf(text,"%s\nDevice IP Address: %s on %s (%s)",text,addr,r->discovered->info.network.interface_name,interface_addr);

#ifdef USBOZY
      }
#endif
      break;
  }
  lines++;

  label=gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_grid_attach(GTK_GRID(grid),label,1,row,4,1);

  return grid;

}
