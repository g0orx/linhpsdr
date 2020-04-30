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


static GtkWidget *microphone_frame;
static GtkWidget *tx_spin_low;
static GtkWidget *tx_spin_high;


/*
static gboolean close_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
g_print("tx->dialog: close_cb");
  tx->dialog=NULL;
  return TRUE;
}
*/

/*
static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
g_print("tx->dialog: delete_cb: %p\n",tx->dialog);
  tx->dialog=NULL;
  return FALSE;
}
*/

static void microphone_audio_cb(GtkWidget *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  if(radio->local_microphone) {
    audio_close_input(radio);
  }
  radio->local_microphone=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if(radio->local_microphone) {
    if(audio_open_input(radio)<0) {
      radio->local_microphone=FALSE;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (radio->transmitter->local_microphone_b),FALSE);
    }
  }
}

static void microphone_choice_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  int i;
  if(radio->local_microphone) {
    audio_close_input(radio);
    i=gtk_combo_box_get_active(widget);
    if(radio->microphone_name!=NULL) {
      g_free(radio->microphone_name);
    }
    radio->microphone_name=g_new0(gchar,strlen(input_devices[i].name)+1);
    strcpy(radio->microphone_name,input_devices[i].name);
    if(audio_open_input(radio)<0) {
      radio->local_microphone=FALSE;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (radio->transmitter->local_microphone_b),FALSE);
    }
  } else {
    i=gtk_combo_box_get_active(widget);
    if(radio->microphone_name!=NULL) {
      g_free(radio->microphone_name);
      radio->microphone_name=NULL;
    }
    if(i>=0) {
      radio->microphone_name=g_new0(gchar,strlen(input_devices[i].name)+1);
      strcpy(radio->microphone_name,input_devices[i].name);
    }
  }
  if(gtk_combo_box_get_active(GTK_COMBO_BOX(radio->transmitter->microphone_choice_b))==-1) {
    gtk_widget_set_sensitive(radio->transmitter->local_microphone_b, FALSE);
  } else {
    gtk_widget_set_sensitive(radio->transmitter->local_microphone_b, TRUE);
  }
  g_print("Input device changed: %d: %s (%s)\n",i,input_devices[i].name,output_devices[i].description);
}

/*
static void mic_gain_value_changed_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->mic_gain=gtk_range_get_value(GTK_RANGE(widget));
  SetTXAPanelGain1(tx->channel,pow(10.0, tx->mic_gain / 20.0));
}
*/

static void tune_value_changed_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->tune_percent=gtk_range_get_value(GTK_RANGE(widget));
}

static void tune_use_drive_cb(GtkWidget *widget,gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->tune_use_drive=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void use_rx_filter_cb(GtkWidget *widget,gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->use_rx_filter=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  transmitter_set_filter(tx,tx->filter_low,tx->filter_high);

  gtk_widget_set_sensitive(tx_spin_low,!tx->use_rx_filter);
  gtk_widget_set_sensitive(tx_spin_high,!tx->use_rx_filter);
}

static void tx_spin_low_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->filter_low=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  transmitter_set_filter(tx,tx->filter_low,tx->filter_high);
}

static void tx_spin_high_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->filter_high=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  transmitter_set_filter(tx,tx->filter_low,tx->filter_high);
}

static void emp_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->pre_emphasize=gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  transmitter_set_pre_emphasize(tx,tx->pre_emphasize);
}

static void am_carrier_level_value_changed_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->am_carrier_level=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  transmitter_set_am_carrier_level(tx);
}

static void enable_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->enable_equalizer=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  SetTXAEQRun(tx->channel, tx->enable_equalizer);
}

static void preamp_value_changed_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->equalizer[0]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetTXAGrphEQ(tx->channel, tx->equalizer);
}

static void low_value_changed_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->equalizer[1]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetTXAGrphEQ(tx->channel, tx->equalizer);
}

static void mid_value_changed_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->equalizer[2]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetTXAGrphEQ(tx->channel, tx->equalizer);
}

static void high_value_changed_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->equalizer[3]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetTXAGrphEQ(tx->channel, tx->equalizer);
}

static void fps_value_changed_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->fps=gtk_range_get_value(GTK_RANGE(widget));
  transmitter_fps_changed(tx);
}

static void panadapter_high_value_changed_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->panadapter_high=gtk_range_get_value(GTK_RANGE(widget));
}

static void panadapter_low_value_changed_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->panadapter_low=gtk_range_get_value(GTK_RANGE(widget));
}

static void ctcss_enable_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  int state=gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  transmitter_set_ctcss(tx,state,tx->ctcss_frequency);
}

static void ctcss_spin_cb (GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  double frequency=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  transmitter_set_ctcss(tx,tx->ctcss,frequency);
}

void update_transmitter_dialog(TRANSMITTER *tx) {
  int i;
g_print("transmitter: update_transmitter_dialog: tx=%d\n",tx->channel);


  g_signal_handler_block(G_OBJECT(tx->microphone_choice_b),tx->microphone_choice_signal_id);
  g_signal_handler_block(G_OBJECT(tx->local_microphone_b),tx->local_microphone_signal_id);

  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(radio->transmitter->microphone_choice_b));
  for(i=0;i<n_input_devices;i++) {
g_print("adding: %s\n",input_devices[i].description);
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(radio->transmitter->microphone_choice_b),NULL,input_devices[i].description);
    if(radio->microphone_name!=NULL) {
      if(strcmp(input_devices[i].name,radio->microphone_name)==0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(radio->transmitter->microphone_choice_b),i);
      }
    }
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tx->local_microphone_b), radio->local_microphone);

  if(gtk_combo_box_get_active(GTK_COMBO_BOX(radio->transmitter->microphone_choice_b))==-1) {
    gtk_widget_set_sensitive(radio->transmitter->local_microphone_b, FALSE);
  } else {
    gtk_widget_set_sensitive(radio->transmitter->local_microphone_b, TRUE);
  }

  g_signal_handler_unblock(G_OBJECT(tx->local_microphone_b),tx->local_microphone_signal_id);
  g_signal_handler_unblock(G_OBJECT(tx->microphone_choice_b),tx->microphone_choice_signal_id);
}

GtkWidget *create_transmitter_dialog(TRANSMITTER *tx) {
  int i;

  GtkWidget *grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(grid),5);
  gtk_grid_set_row_spacing(GTK_GRID(grid),5);

  int row=0;
  int col=0;

  microphone_frame=gtk_frame_new("Microphone");
  GtkWidget *microphone_grid=gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(microphone_grid),10);
  gtk_grid_set_row_homogeneous(GTK_GRID(microphone_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(microphone_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(microphone_frame),microphone_grid);
  gtk_grid_attach(GTK_GRID(grid),microphone_frame,col,row++,2,1);

  if(n_input_devices>=0) {
    radio->transmitter->local_microphone_b=gtk_check_button_new_with_label("Local Microphone");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio->transmitter->local_microphone_b), radio->local_microphone);
    gtk_grid_attach(GTK_GRID(microphone_grid),radio->transmitter->local_microphone_b,0,0,1,1);
    radio->transmitter->local_microphone_signal_id=g_signal_connect(radio->transmitter->local_microphone_b,"toggled",G_CALLBACK(microphone_audio_cb),radio);

    radio->transmitter->microphone_choice_b=gtk_combo_box_text_new();
    //update_transmitter_audio_choices(tx);
    // TO REMOVE because the variable n_input_devices is always zero here
    // for(i=0;i<n_input_devices;i++) {
    //   g_print("adding: %s\n",input_devices[i].description);
    //   gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(radio->transmitter->microphone_choice_b),NULL,input_devices[i].description);
    //   if(radio->microphone_name!=NULL) {
    //     if(strcmp(input_devices[i].name,radio->microphone_name)==0) {
    //       gtk_combo_box_set_active(GTK_COMBO_BOX(radio->transmitter->microphone_choice_b),i);
    //     }
    //   }
    // }
    // Moved to update_transmitter_dialog
    // if(gtk_combo_box_get_active(GTK_COMBO_BOX(radio->transmitter->microphone_choice_b))==-1) {
    //   gtk_widget_set_sensitive(radio->transmitter->local_microphone_b, FALSE);
    // } else {
    //   gtk_widget_set_sensitive(radio->transmitter->local_microphone_b, TRUE);
    // }

    gtk_grid_attach(GTK_GRID(microphone_grid),radio->transmitter->microphone_choice_b,1,0,1,1);
    radio->transmitter->microphone_choice_signal_id=g_signal_connect(radio->transmitter->microphone_choice_b,"changed",G_CALLBACK(microphone_choice_cb),radio);

  }

  GtkWidget *tune_frame=gtk_frame_new("Tune");
  GtkWidget *tune_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(tune_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(tune_grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(tune_grid),10);
  gtk_container_add(GTK_CONTAINER(tune_frame),tune_grid);
  gtk_grid_attach(GTK_GRID(grid),tune_frame,col,row++,1,1);

  GtkWidget *tune_label=gtk_label_new("Tune Percent:");
  gtk_widget_show(tune_label);
  gtk_grid_attach(GTK_GRID(tune_grid),tune_label,0,1,1,1);

  GtkWidget *tune_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,0.0,100.0,1.00);
  gtk_widget_set_size_request (tune_scale, 300, 32);
  gtk_range_set_value (GTK_RANGE(tune_scale),tx->tune_percent);
  gtk_widget_show(tune_scale);
  g_signal_connect(G_OBJECT(tune_scale),"value_changed",G_CALLBACK(tune_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(tune_grid),tune_scale,1,1,1,1);

  GtkWidget *tune_use_drive=gtk_check_button_new_with_label("Tune Use Drive:");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tune_use_drive), tx->tune_use_drive);
  gtk_grid_attach(GTK_GRID(tune_grid),tune_use_drive,0,2,1,1);
  g_signal_connect(tune_use_drive,"toggled",G_CALLBACK(tune_use_drive_cb),tx);

  GtkWidget *filter_frame=gtk_frame_new("TX Filter");
  GtkWidget *filter_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(filter_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(filter_grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(filter_grid),10);
  gtk_container_add(GTK_CONTAINER(filter_frame),filter_grid);
  gtk_grid_attach(GTK_GRID(grid),filter_frame,col,row++,1,1);

  GtkWidget *use_rx_filter=gtk_check_button_new_with_label("Use Rx Filter:");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_rx_filter), tx->use_rx_filter);
  gtk_grid_attach(GTK_GRID(filter_grid),use_rx_filter,0,0,2,1);
  g_signal_connect(use_rx_filter,"toggled",G_CALLBACK(use_rx_filter_cb),tx);

  GtkWidget *low_label=gtk_label_new("Low:");
  gtk_widget_show(low_label);
  gtk_grid_attach(GTK_GRID(filter_grid),low_label,0,1,1,1);

  tx_spin_low=gtk_spin_button_new_with_range(0.0,8000.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tx_spin_low),(double)tx->filter_low);
  gtk_grid_attach(GTK_GRID(filter_grid),tx_spin_low,1,1,1,1);
  g_signal_connect(tx_spin_low,"value-changed",G_CALLBACK(tx_spin_low_cb),tx);

  GtkWidget *high_label=gtk_label_new("High:");
  gtk_widget_show(high_label);
  gtk_grid_attach(GTK_GRID(filter_grid),high_label,2,1,1,1);

  tx_spin_high=gtk_spin_button_new_with_range(0.0,8000.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tx_spin_high),(double)tx->filter_high);
  gtk_grid_attach(GTK_GRID(filter_grid),tx_spin_high,3,1,1,1);
  g_signal_connect(tx_spin_high,"value-changed",G_CALLBACK(tx_spin_high_cb),tx);

  gtk_widget_set_sensitive(tx_spin_low,!tx->use_rx_filter);
  gtk_widget_set_sensitive(tx_spin_high,!tx->use_rx_filter);

  GtkWidget *fm_frame=gtk_frame_new("FM");
  GtkWidget *fm_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(fm_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(fm_grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(fm_grid),10);
  gtk_container_add(GTK_CONTAINER(fm_frame),fm_grid);
  gtk_grid_attach(GTK_GRID(grid),fm_frame,col,row++,1,1);

  GtkWidget *emp_b=gtk_check_button_new_with_label("FM TX Pre-emphasize before limiting");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (emp_b), tx->pre_emphasize);
  gtk_widget_show(emp_b);
  gtk_grid_attach(GTK_GRID(fm_grid),emp_b,0,0,1,1);
  g_signal_connect(emp_b,"toggled",G_CALLBACK(emp_cb),tx);

  GtkWidget *am_frame=gtk_frame_new("AM");
  GtkWidget *am_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(am_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(am_grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(am_grid),10);
  gtk_container_add(GTK_CONTAINER(am_frame),am_grid);
  gtk_grid_attach(GTK_GRID(grid),am_frame,col,row++,1,1);

  GtkWidget *am_carrier_level_label=gtk_label_new("AM Carrier Level: ");
#ifdef GTK316
  gtk_label_set_xalign(GTK_LABEL(am_carrier_level_label),0);
#endif
  gtk_widget_show(am_carrier_level_label);
  gtk_grid_attach(GTK_GRID(am_grid),am_carrier_level_label,0,0,1,1);
  GtkWidget *am_carrier_level=gtk_spin_button_new_with_range(0.0,1.0,0.1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(am_carrier_level),(double)tx->am_carrier_level);
  gtk_widget_show(am_carrier_level);
  gtk_grid_attach(GTK_GRID(am_grid),am_carrier_level,1,0,1,1);
  g_signal_connect(am_carrier_level,"value_changed",G_CALLBACK(am_carrier_level_value_changed_cb),tx);

  GtkWidget *ctcss_frame=gtk_frame_new("CTCSS");
  GtkWidget *ctcss_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(ctcss_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(ctcss_grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(ctcss_grid),10);
  gtk_container_add(GTK_CONTAINER(ctcss_frame),ctcss_grid);
  gtk_grid_attach(GTK_GRID(grid),ctcss_frame,col,row++,1,1);

  GtkWidget *ctcss_enable=gtk_check_button_new_with_label("Enable CTCSS");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctcss_enable), tx->ctcss);
  gtk_grid_attach(GTK_GRID(ctcss_grid),ctcss_enable,0,0,1,1);
  g_signal_connect(ctcss_enable,"toggled",G_CALLBACK(ctcss_enable_cb),tx);

  GtkWidget *ctcss_spin=gtk_spin_button_new_with_range(67.0,250.3,0.1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctcss_spin),(double)tx->ctcss_frequency);
  gtk_grid_attach(GTK_GRID(ctcss_grid),ctcss_spin,1,0,1,1);
  g_signal_connect(ctcss_spin,"value-changed",G_CALLBACK(ctcss_spin_cb),tx);

  GtkWidget *panadapter_frame=gtk_frame_new("Panadapter");
  GtkWidget *panadapter_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(panadapter_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(panadapter_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(panadapter_frame),panadapter_grid);
  gtk_grid_attach(GTK_GRID(grid),panadapter_frame,col,row++,1,1);

  GtkWidget *fps_label=gtk_label_new("FPS:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),fps_label,0,0,1,1);

  GtkWidget *fps_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,1.0, 50.0, 1.00);
  GtkAdjustment *adj=gtk_range_get_adjustment(GTK_RANGE(fps_scale));
  gtk_adjustment_set_page_increment(adj,1.0);
  gtk_widget_set_size_request(fps_scale,200,30);
  gtk_range_set_value (GTK_RANGE(fps_scale),tx->fps);
  gtk_widget_show(fps_scale);
  g_signal_connect(G_OBJECT(fps_scale),"value_changed",G_CALLBACK(fps_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),fps_scale,1,0,1,1);

  high_label=gtk_label_new("High:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),high_label,0,1,1,1);

  GtkWidget *panadapter_high_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,-200.0, 20.0, 1.00);
  gtk_widget_set_size_request(panadapter_high_scale,200,30);
  gtk_range_set_value (GTK_RANGE(panadapter_high_scale),tx->panadapter_high);
  gtk_widget_show(panadapter_high_scale);
  g_signal_connect(G_OBJECT(panadapter_high_scale),"value_changed",G_CALLBACK(panadapter_high_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_high_scale,1,1,1,1);

  GtkWidget *waterfall_low_label=gtk_label_new("Low:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),waterfall_low_label,0,2,1,1);

  GtkWidget *panadapter_low_scale=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,-200.0, 20.0, 1.00);
  gtk_widget_set_size_request(panadapter_low_scale,200,30);
  gtk_range_set_value (GTK_RANGE(panadapter_low_scale),tx->panadapter_low);
  gtk_widget_show(panadapter_low_scale);
  g_signal_connect(G_OBJECT(panadapter_low_scale),"value_changed",G_CALLBACK(panadapter_low_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_low_scale,1,2,1,1);

  col++;
  row=1;

  GtkWidget *equalizer_frame=gtk_frame_new("Equalizer");
  GtkWidget *equalizer_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(equalizer_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(equalizer_grid),TRUE);
  gtk_container_add(GTK_CONTAINER(equalizer_frame),equalizer_grid);
  gtk_grid_attach(GTK_GRID(grid),equalizer_frame,col,row++,1,4);

  GtkWidget *enable_b=gtk_check_button_new_with_label("Enable Equalizer");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (enable_b), tx->enable_equalizer);
  g_signal_connect(enable_b,"toggled",G_CALLBACK(enable_cb),tx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),enable_b,0,0,4,1);

  GtkWidget *label=gtk_label_new("Preamp");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,0,1,1,1);

  label=gtk_label_new("Low");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,1,1,1,1);

  label=gtk_label_new("Mid");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,2,1,1,1);

  label=gtk_label_new("High");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,3,1,1,1);

  GtkWidget *preamp_scale=gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,-12.0,15.0,1.0);
  adj=gtk_range_get_adjustment(GTK_RANGE(preamp_scale));
  gtk_adjustment_set_page_increment(adj,1.0);
  gtk_range_set_value(GTK_RANGE(preamp_scale),(double)tx->equalizer[0]);
  g_signal_connect(preamp_scale,"value-changed",G_CALLBACK(preamp_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),preamp_scale,0,2,1,10);
  gtk_widget_set_size_request(preamp_scale,10,270);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),15.0,GTK_POS_LEFT,"15dB");

  GtkWidget *low_scale=gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,-12.0,15.0,1.0);
  adj=gtk_range_get_adjustment(GTK_RANGE(low_scale));
  gtk_adjustment_set_page_increment(adj,1.0);
  gtk_range_set_value(GTK_RANGE(low_scale),(double)tx->equalizer[1]);
  g_signal_connect(low_scale,"value-changed",G_CALLBACK(low_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),low_scale,1,2,1,10);
  gtk_scale_add_mark(GTK_SCALE(low_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(low_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(low_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),15.0,GTK_POS_LEFT,"15dB");

  GtkWidget *mid_scale=gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,-12.0,15.0,1.0);
  adj=gtk_range_get_adjustment(GTK_RANGE(mid_scale));
  gtk_adjustment_set_page_increment(adj,1.0);
  gtk_range_set_value(GTK_RANGE(mid_scale),(double)tx->equalizer[2]);
  g_signal_connect(mid_scale,"value-changed",G_CALLBACK(mid_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),mid_scale,2,2,1,10);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(mid_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),15.0,GTK_POS_LEFT,"15dB");

  GtkWidget *high_scale=gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,-12.0,15.0,1.0);
  adj=gtk_range_get_adjustment(GTK_RANGE(high_scale));
  gtk_adjustment_set_page_increment(adj,1.0);
  gtk_range_set_value(GTK_RANGE(high_scale),(double)tx->equalizer[3]);
  g_signal_connect(high_scale,"value-changed",G_CALLBACK(high_value_changed_cb),tx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),high_scale,3,2,1,10);
  gtk_scale_add_mark(GTK_SCALE(high_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(high_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(high_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),15.0,GTK_POS_LEFT,"15dB");

  return grid;
}
