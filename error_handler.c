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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#endif

#include "error_handler.h"
#include "discovered.h"
#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"

static GtkWidget *dialog;

int timeout_cb(gpointer data) {
  gtk_widget_destroy(dialog);
  exit(1);
}

void error_handler(char *text,char *err) {
  char message[1024];
  sprintf(message,"ERROR: %s: %s\n",text,err);
  fprintf(stderr,"%s\n",message);

  sprintf(message,"ERROR\n\n    %s:\n\n    %s\n\npiHPSDR will terminate in 5 seconds",text,err);
}
