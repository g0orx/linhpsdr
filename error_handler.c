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

#include <SoapySDR/Device.h>

#include "error_handler.h"
#include "discovered.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "radio.h"
#include "main.h"

static GtkWidget *dialog;
static gint timer;

int timeout_cb(gpointer data) {
  gtk_widget_destroy(dialog);
  exit(1);
}

/*
int show_error(void *data) {
  dialog=gtk_dialog_new_with_buttons("ERROR",GTK_WINDOW(main_window),GTK_DIALOG_DESTROY_WITH_PARENT,NULL,NULL);
  GtkWidget *content=gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *label=gtk_label_new((char *)data);
  gtk_container_add(GTK_CONTAINER(content),label);
  gtk_widget_show(label);
  timer=g_timeout_add(5000,timeout_cb,NULL);
  int result=gtk_dialog_run(GTK_DIALOG(dialog));
}
*/

void error_handler(char *text,char *err) {
  char message[1024];
  sprintf(message,"ERROR: %s: %s\n",text,err);
  fprintf(stderr,"%s\n",message);

  sprintf(message,"ERROR\n\n    %s:\n\n    %s\n\npiHPSDR will terminate in 5 seconds",text,err);

  //g_idle_add(show_error,message);

}
