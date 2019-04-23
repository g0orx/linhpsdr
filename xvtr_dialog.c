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
#include <semaphore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#include "band.h"
#include "bandstack.h"
#include "mode.h"
#include "filter.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "adc.h"
#include "radio.h"
#include "vfo.h"
#include "main.h"
#include "protocol1.h"
#include "protocol2.h"
#include "tx_panadapter.h"
#include "xvtr_dialog.h"

static GtkWidget *title[BANDS+XVTRS];
static GtkWidget *min_frequency[BANDS+XVTRS];
static GtkWidget *max_frequency[BANDS+XVTRS];
static GtkWidget *lo_frequency[BANDS+XVTRS];
static GtkWidget *lo_error[BANDS+XVTRS];
static GtkWidget *tx_lo_frequency[BANDS+XVTRS];
static GtkWidget *tx_lo_error[BANDS+XVTRS];
static GtkWidget *disable_pa[BANDS+XVTRS];

void save_xvtr () {
  int i;
  int b;
  const char *t;
  const char *minf;
  const char *maxf;
  const char *lof;
  const char *loerr;
  const char *txlof;
  const char *txloerr;
  for(i=BANDS;i<BANDS+XVTRS;i++) {
    BAND *xvtr=band_get_band(i);
    BANDSTACK *bandstack=xvtr->bandstack;
    t=gtk_entry_get_text(GTK_ENTRY(title[i]));
    strcpy(xvtr->title,t);
    if(strlen(t)!=0) {
      minf=gtk_entry_get_text(GTK_ENTRY(min_frequency[i]));
      xvtr->frequencyMin=(long long)(atof(minf)*1000000.0);
      maxf=gtk_entry_get_text(GTK_ENTRY(max_frequency[i]));
      xvtr->frequencyMax=(long long)(atof(maxf)*1000000.0);
      lof=gtk_entry_get_text(GTK_ENTRY(lo_frequency[i]));
      xvtr->frequencyLO=(long long)(atof(lof)*1000000.0);
      loerr=gtk_entry_get_text(GTK_ENTRY(lo_error[i]));
      xvtr->errorLO=atoll(loerr);
      txlof=gtk_entry_get_text(GTK_ENTRY(tx_lo_frequency[i]));
      xvtr->txFrequencyLO=(long long)(atof(txlof)*1000000.0);
      txloerr=gtk_entry_get_text(GTK_ENTRY(tx_lo_error[i]));
      xvtr->txErrorLO=atoll(txloerr);
      xvtr->disablePA=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(disable_pa[i]));
      for(b=0;b<bandstack->entries;b++) {
        BANDSTACK_ENTRY *entry=&bandstack->entry[b];
        entry->frequency=xvtr->frequencyMin;
        entry->mode=USB;
        entry->filter=F6;
      }
//g_print("min=%s:%lld max=%s:%lld lo=%s:%lld\n",minf,xvtr->frequencyMin,maxf,xvtr->frequencyMax,lof,xvtr->frequencyLO);
    } else {
      xvtr->frequencyMin=0;
      xvtr->frequencyMax=0;
      xvtr->frequencyLO=0;
      xvtr->errorLO=0;
      xvtr->txFrequencyLO=0;
      xvtr->txErrorLO=0;
      xvtr->disablePA=0;
    }
  }
}

void update_receiver(int band,gboolean error) {
  int i;
  RECEIVER *rx;
  gboolean saved_ctun;
  for(i=0;i<MAX_RECEIVERS;i++) {
    rx=radio->receiver[i];
    if(rx!=NULL) {
fprintf(stderr,"update_receiver: band=%d rx=%d band_a=%d\n",band,i,rx->band_a);
      if(rx->band_a==band) {
        BAND *xvtr=band_get_band(band);
        rx->lo_a=xvtr->frequencyLO;
        rx->error_a=xvtr->errorLO;
        rx->lo_tx=xvtr->txFrequencyLO;
        rx->error_tx=xvtr->txErrorLO;
        saved_ctun=rx->ctun;
        if(saved_ctun) {
          rx->ctun=FALSE;
        }
        frequency_changed(rx);
        if(saved_ctun) {
          rx->ctun=TRUE;
        }

        if(radio->transmitter!=NULL) {
          if(radio->transmitter->rx==rx) {
            update_tx_panadapter(radio);
          }
        }

      }
    }
  }
}

void min_frequency_cb(GtkEditable *editable,gpointer user_data) {
  int band=GPOINTER_TO_INT(user_data);
  BAND *xvtr=band_get_band(band);
  const char* minf=gtk_entry_get_text(GTK_ENTRY(min_frequency[band]));
  xvtr->frequencyMin=(long long)(atof(minf)*1000000.0);
  update_receiver(band,FALSE);
}

void max_frequency_cb(GtkEditable *editable,gpointer user_data) {
  int band=GPOINTER_TO_INT(user_data);
  BAND *xvtr=band_get_band(band);
  const char* maxf=gtk_entry_get_text(GTK_ENTRY(max_frequency[band]));
  xvtr->frequencyMin=(long long)(atof(maxf)*1000000.0);
  update_receiver(band,FALSE);
}

void lo_frequency_cb(GtkEditable *editable,gpointer user_data) {
  int band=GPOINTER_TO_INT(user_data);
  BAND *xvtr=band_get_band(band);
  const char* lof=gtk_entry_get_text(GTK_ENTRY(lo_frequency[band]));
  xvtr->frequencyLO=(long long)(atof(lof)*1000000.0);
  update_receiver(band,FALSE);
}

void lo_error_cb(GtkEditable *editable,gpointer user_data) {
  int band=GPOINTER_TO_INT(user_data);
  BAND *xvtr=band_get_band(band);
  const char* errorf=gtk_entry_get_text(GTK_ENTRY(lo_error[band]));
  xvtr->errorLO=atoll(errorf);
  update_receiver(band,TRUE);
}

void tx_lo_frequency_cb(GtkEditable *editable,gpointer user_data) {
  int band=GPOINTER_TO_INT(user_data);
  BAND *xvtr=band_get_band(band);
  const char* lof=gtk_entry_get_text(GTK_ENTRY(tx_lo_frequency[band]));
  xvtr->txFrequencyLO=(long long)(atof(lof)*1000000.0);
  update_receiver(band,FALSE);
}

void tx_lo_error_cb(GtkEditable *editable,gpointer user_data) {
  int band=GPOINTER_TO_INT(user_data);
  BAND *xvtr=band_get_band(band);
  const char* errorf=gtk_entry_get_text(GTK_ENTRY(tx_lo_error[band]));
  xvtr->txErrorLO=atoll(errorf);
  update_receiver(band,TRUE);
}

void update_error(int band,gint64 error) {
  BAND *xvtr=band_get_band(band);
  xvtr->errorLO+=error;
  update_receiver(band,TRUE);
}

GtkWidget *create_xvtr_dialog(RADIO *radio) {
  int row;
  int col;
  int i;
  char temp[32];

  GtkWidget *grid=gtk_grid_new();
  gtk_grid_set_column_spacing (GTK_GRID(grid),10);

  row=0;
  col=0;

  GtkWidget *label=gtk_label_new("Title");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);
  label=gtk_label_new("Min Frequency(MHz)");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);
  label=gtk_label_new("Max Frequency(MHz)");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);
  label=gtk_label_new("LO Frequency(MHz)");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);
  label=gtk_label_new("LO Error(Hz)");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);
  label=gtk_label_new("TX LO Frequency(MHz)");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);
  label=gtk_label_new("TX LO Error(Hz)");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);
  label=gtk_label_new("Disable PA");
  gtk_grid_attach(GTK_GRID(grid),label,col++,row,1,1);

  row++;
  col=0;

  for(i=BANDS;i<BANDS+XVTRS;i++) {
    BAND *xvtr=band_get_band(i);

    title[i]=gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(title[i]),7);
    gtk_entry_set_text(GTK_ENTRY(title[i]),xvtr->title);
    gtk_grid_attach(GTK_GRID(grid),title[i],col++,row,1,1);

    min_frequency[i]=gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(min_frequency[i]),7);
    sprintf(temp,"%f",(double)xvtr->frequencyMin/1000000.0);
    gtk_entry_set_text(GTK_ENTRY(min_frequency[i]),temp);
    gtk_grid_attach(GTK_GRID(grid),min_frequency[i],col++,row,1,1);
    g_signal_connect(min_frequency[i],"changed",G_CALLBACK(min_frequency_cb),GINT_TO_POINTER(i));

    max_frequency[i]=gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(max_frequency[i]),7);
    sprintf(temp,"%f",(double)xvtr->frequencyMax/1000000.0);
    gtk_entry_set_text(GTK_ENTRY(max_frequency[i]),temp);
    gtk_grid_attach(GTK_GRID(grid),max_frequency[i],col++,row,1,1);
    g_signal_connect(max_frequency[i],"changed",G_CALLBACK(max_frequency_cb),GINT_TO_POINTER(i));

    lo_frequency[i]=gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(lo_frequency[i]),7);
    sprintf(temp,"%f",(double)xvtr->frequencyLO/1000000.0);
    gtk_entry_set_text(GTK_ENTRY(lo_frequency[i]),temp);
    gtk_grid_attach(GTK_GRID(grid),lo_frequency[i],col++,row,1,1);
    g_signal_connect(lo_frequency[i],"changed",G_CALLBACK(lo_frequency_cb),GINT_TO_POINTER(i));

    lo_error[i]=gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(lo_error[i]),9);
    sprintf(temp,"%lld",xvtr->errorLO);
    gtk_entry_set_text(GTK_ENTRY(lo_error[i]),temp);
    gtk_grid_attach(GTK_GRID(grid),lo_error[i],col++,row,1,1);
    g_signal_connect(lo_error[i],"changed",G_CALLBACK(lo_error_cb),GINT_TO_POINTER(i));

    tx_lo_frequency[i]=gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(tx_lo_frequency[i]),7);
    sprintf(temp,"%f",(double)xvtr->txFrequencyLO/1000000.0);
    gtk_entry_set_text(GTK_ENTRY(tx_lo_frequency[i]),temp);
    gtk_grid_attach(GTK_GRID(grid),tx_lo_frequency[i],col++,row,1,1);
    g_signal_connect(tx_lo_frequency[i],"changed",G_CALLBACK(tx_lo_frequency_cb),GINT_TO_POINTER(i));

    tx_lo_error[i]=gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(tx_lo_error[i]),9);
    sprintf(temp,"%lld",xvtr->txErrorLO);
    gtk_entry_set_text(GTK_ENTRY(tx_lo_error[i]),temp);
    gtk_grid_attach(GTK_GRID(grid),tx_lo_error[i],col++,row,1,1);
    g_signal_connect(tx_lo_error[i],"changed",G_CALLBACK(tx_lo_error_cb),GINT_TO_POINTER(i));

    disable_pa[i]=gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disable_pa[i]),xvtr->disablePA);
    gtk_grid_attach(GTK_GRID(grid),disable_pa[i],col++,row,1,1);

    row++;
    col=0;

  }

  gtk_widget_show_all(grid);

  return grid;

}

