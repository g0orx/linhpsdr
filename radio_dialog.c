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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
#ifdef SOAPYSDR
#include "soapy_protocol.h"
#endif
#include "audio.h"
#include "receiver_dialog.h"
//#include "rigctl.h"

#ifdef CWDAEMON
#include "cwdaemon.h"
#endif

static GtkWidget *filter_board_combo_box;
static GtkWidget *adc0_frame;
static GtkWidget *adc0_antenna_combo_box;
static GtkWidget *adc0_filters_combo_box;
static GtkWidget *adc0_hpf_combo_box;
static GtkWidget *adc0_lpf_combo_box;
static GtkWidget *duplex_b;
static GtkWidget *sat_combo;
static GtkWidget *mute_rx_b;
static GtkWidget *dither_b;
static GtkWidget *random_b;
static GtkWidget *preamp_b;
static GtkWidget *attenuation_label;
static GtkWidget *attenuation_b;
static GtkWidget *enable_attenuation_b;
static GtkWidget *disable_fpgaclk_b;
static GtkWidget *swr_alarm_b;
static GtkWidget *temperature_alarm_b;

static GtkWidget *adc1_frame;
static GtkWidget *adc1_antenna_combo_box;
static GtkWidget *adc1_filters_combo_box;
static GtkWidget *adc1_hpf_combo_box;

static GtkWidget *cw_keyer_sidetone_frequency_b;
static GtkWidget *cw_keyer_speed_b;
static GtkWidget *cw_keyer_weight_b;
static GtkWidget *cw_keyer_sidetone_level_b;

//static GtkWidget *rigctl_base;

#ifdef SOAPYSDR
static GtkWidget *dac0_frame;
static GtkWidget *dac0_antenna_combo_box;
#endif

#ifdef CWDAEMON
static GtkWidget *cwport;
#endif

static GtkWidget *audio_backend_combo_box;

/* TO REMOVE
static gboolean close_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->dialog=NULL;
  return TRUE;
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->dialog=NULL;
  return FALSE;
}
*/

static void radio_dialog_update_controls() {
	g_print("%s: model=%d\n",__FUNCTION__,radio->model);
  switch(radio->model) {
    case ANAN_10:
    case ANAN_10E:
    case ANAN_100:
    case ANAN_100D:
    case ANAN_200D:
      radio->filter_board=ALEX;
      break;
    case ANAN_7000DLE:
    case ANAN_8000DLE:
      radio->filter_board=ALEX;
      break;
    case HERMES_LITE_2:
      radio->filter_board=N2ADR;
      break;
    case ATLAS:
    case HERMES:
    case HERMES_2:
    case ANGELIA:
    case ORION_1:
    case ORION_2:
#ifdef SOAPYSDR
    case SOAPY_DEVICE:
#endif
      radio->filter_board=NONE;
      break;
  }

  switch(radio->model) {
    case ANAN_7000DLE:
    case ANAN_8000DLE:
      gtk_widget_set_sensitive(adc0_antenna_combo_box, TRUE);
      gtk_widget_set_sensitive(adc0_filters_combo_box, TRUE);
      if(radio->adc[0].filters==AUTOMATIC) {
        gtk_widget_set_sensitive(adc0_lpf_combo_box, FALSE);
        gtk_widget_set_sensitive(adc0_hpf_combo_box, FALSE);
      } else {
        gtk_widget_set_sensitive(adc0_hpf_combo_box, TRUE);
        gtk_widget_set_sensitive(adc0_lpf_combo_box, TRUE);
      }
      gtk_widget_set_sensitive(adc1_antenna_combo_box, TRUE);
      gtk_widget_set_sensitive(adc1_filters_combo_box, TRUE);
      if(radio->adc[1].filters==AUTOMATIC) {
        gtk_widget_set_sensitive(adc1_hpf_combo_box, FALSE);
      } else {
        gtk_widget_set_sensitive(adc1_hpf_combo_box, TRUE);
      }
      break;
    case ANAN_100:
    case ANAN_100D:
    case ANAN_200D:
      gtk_widget_set_sensitive(adc0_antenna_combo_box, TRUE);
      gtk_widget_set_sensitive(adc0_filters_combo_box, TRUE);
      if(radio->adc[0].filters==AUTOMATIC) {
        gtk_widget_set_sensitive(adc0_lpf_combo_box, FALSE);
        gtk_widget_set_sensitive(adc0_hpf_combo_box, FALSE);
      } else {
        gtk_widget_set_sensitive(adc0_hpf_combo_box, TRUE);
        gtk_widget_set_sensitive(adc0_lpf_combo_box, TRUE);
      }
      gtk_widget_set_sensitive(adc1_antenna_combo_box, FALSE);
      gtk_widget_set_sensitive(adc1_filters_combo_box, FALSE);
      gtk_widget_set_sensitive(adc0_lpf_combo_box, FALSE);
      gtk_widget_set_sensitive(adc0_hpf_combo_box, FALSE);
      break;
    case HERMES_LITE:
    case HERMES_LITE_2:
      break;
#ifdef SOAPYSDR
    case SOAPY_DEVICE:
      break;
#endif
    default:
      g_print("%s: defualt set_sensitive\n",__FUNCTION__);
      gtk_widget_set_sensitive(adc0_antenna_combo_box, FALSE);
      gtk_widget_set_sensitive(adc0_filters_combo_box, FALSE);
      gtk_widget_set_sensitive(adc0_hpf_combo_box, FALSE);
      gtk_widget_set_sensitive(adc0_lpf_combo_box, FALSE);

      gtk_widget_set_sensitive(adc1_antenna_combo_box, FALSE);
      gtk_widget_set_sensitive(adc1_filters_combo_box, FALSE);
      gtk_widget_set_sensitive(adc1_hpf_combo_box, FALSE);
      break;
  }

#ifdef SOAPYSDR
  if(radio->discovered->device!=DEVICE_SOAPYSDR) {
#endif
    gtk_combo_box_set_active(GTK_COMBO_BOX(filter_board_combo_box),radio->filter_board);
#ifdef SOAPYSDR
  }
#endif
}

static void model_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->model=gtk_combo_box_get_active(widget);
  radio_dialog_update_controls();
}

static void sample_rate_cb(GtkComboBoxText *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  int rate;
  int i;

  rate=atoi(gtk_combo_box_text_get_active_text(widget));

  switch(radio->discovered->protocol) {
    case PROTOCOL_1:
      protocol1_stop();
      break;
    case PROTOCOL_2:
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      //soapy_protocol_stop();
      break;
#endif
  }
  radio->sample_rate=rate;
  for(i=0;i<radio->discovered->supported_receivers;i++) {
    if(radio->receiver[i]!=NULL) {
      receiver_change_sample_rate(radio->receiver[i],rate);
    }
  }
  switch(radio->discovered->protocol) {
    case PROTOCOL_1:
      protocol1_set_mic_sample_rate(rate);
      break;
    case PROTOCOL_2:
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      break;
#endif
  }

  g_idle_add(radio_restart,(void *)radio);
}

static void filter_board_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->filter_board=gtk_combo_box_get_active(widget);
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

static void adc0_antenna_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->adc[0].antenna=gtk_combo_box_get_active(widget);
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
#ifdef SOAPYSDR
  } else if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
    soapy_protocol_set_rx_antenna(radio->receiver[0],radio->adc[0].antenna);
#endif
  }
}

static void adc1_antenna_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->adc[1].antenna=gtk_combo_box_get_active(widget);
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

#ifdef SOAPYSDR
static void dac0_antenna_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->dac[0].antenna=gtk_combo_box_get_active(widget);
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();

  } else if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
    soapy_protocol_set_tx_antenna(radio->transmitter,radio->dac[0].antenna);
  }
}
#endif

static void adc0_filters_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->adc[0].filters=gtk_combo_box_get_active(widget);
  if(radio->adc[0].filters==MANUAL) {
    gtk_widget_set_sensitive(adc0_hpf_combo_box, TRUE);
    gtk_widget_set_sensitive(adc0_lpf_combo_box, TRUE);
  } else {
    gtk_widget_set_sensitive(adc0_hpf_combo_box, FALSE);
    gtk_widget_set_sensitive(adc0_lpf_combo_box, FALSE);
  }
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

static void adc0_hpf_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->adc[0].hpf=gtk_combo_box_get_active(widget);
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

static void adc0_lpf_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->adc[0].lpf=gtk_combo_box_get_active(widget);
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

static void adc1_filters_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->adc[1].filters=gtk_combo_box_get_active(widget);
  if(radio->adc[1].filters==MANUAL) {
    gtk_widget_set_sensitive(adc1_hpf_combo_box, TRUE);
  } else {
    gtk_widget_set_sensitive(adc1_hpf_combo_box, FALSE);
  }
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }

}

static void adc1_hpf_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->adc[1].hpf=gtk_combo_box_get_active(widget);
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

static void ptt_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->mic_ptt_enabled=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void ptt_ring_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    radio->mic_ptt_tip_bias_ring=0;
  }
}

static void ptt_tip_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    radio->mic_ptt_tip_bias_ring=1;
  }
}

static void bias_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->mic_bias_enabled=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void boost_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->mic_boost=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void update_audio_backends(RADIO *radio) {
  int i;
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(audio_backend_combo_box));
  if(radio->which_audio==USE_SOUNDIO) {
    for(i=0;i<audio_get_backends(radio);i++) {
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(audio_backend_combo_box),NULL,audio_get_backend_name(i));
    }
  }
  if(radio->which_audio_backend>=0) {
    radio_change_audio_backend(radio,radio->which_audio_backend);
  }
}

static void audio_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  int selected=gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
g_print("radio_dialog: audio_cb: selected=%d\n",selected);
  radio_change_audio(radio,selected);
  update_audio_backends(radio);
}

static void audio_backend_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  int selected=gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
g_print("radio_dialog: audio_backend_cb: selected=%d\n",selected);
  radio_change_audio_backend(radio,selected);
}

static void smeter_calibrate_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->meter_calibration=gtk_range_get_value(GTK_RANGE(widget));
}

static void panadapter_calibrate_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->panadapter_calibration=gtk_range_get_value(GTK_RANGE(widget));
}

static void swr_alarm_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->swr_alarm_value=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
}

static void temperature_alarm_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->temperature_alarm_value=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}


static void cw_keyer_speed_value_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cw_keyer_speed=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void cw_breakin_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cw_breakin=radio->cw_breakin==1?0:1;
}

static void cw_keyer_hang_time_value_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cw_keyer_hang_time=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void cw_keyer_weight_value_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cw_keyer_weight=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void cw_keys_reversed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cw_keys_reversed=radio->cw_keys_reversed==1?0:1;
}

static void cw_keyer_cb(GtkComboBox *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cw_keyer_mode=gtk_combo_box_get_active(widget);
}

static void cw_keyer_sidetone_level_value_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cw_keyer_sidetone_volume=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void cw_keyer_sidetone_frequency_value_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  int i;

  radio->cw_keyer_sidetone_frequency=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  for(i=0;i<radio->discovered->supported_receivers;i++) {
    if(radio->receiver[i]!=NULL) {
      receiver_filter_changed(radio->receiver[i],radio->receiver[i]->filter_a);
    }
  }
}

static void psu_clk_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->psu_clk=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void region_cb(GtkWidget *widget, gpointer data) {
  radio->region=gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  radio_change_region(radio);
}

static void dither_cb(GtkWidget *widget, gpointer data) {
  ADC *adc=(ADC *)data;
  adc->dither=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void random_cb(GtkWidget *widget, gpointer data) {
  ADC *adc=(ADC *)data;
  adc->random=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void preamp_cb(GtkWidget *widget, gpointer data) {
  ADC *adc=(ADC *)data;
  adc->preamp=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

#ifdef SOAPYSDR
static void adc_gain_value_changed_cb(GtkWidget *widget, gpointer data) {
  ADC *adc=(ADC *)data;
  adc->gain=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  if(radio->discovered->device==DEVICE_SOAPYSDR) {
    soapy_protocol_set_gain(adc);
  }
}


static void agc_changed_cb(GtkWidget *widget, gpointer data) {
  ADC *adc=(ADC *)data;
  gboolean agc=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  soapy_protocol_set_automatic_gain(radio->receiver[0],agc);
}

static void dac0_gain_value_changed_cb(GtkWidget *widget, gpointer data) {
  DAC *dac=(DAC *)data;
  if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
    dac->gain=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    soapy_protocol_set_tx_gain(dac);
  }
}
#endif

static void iqswap_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *r=(RADIO *)data;
  r->iqswap=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void enablepa_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *r=(RADIO *)data;
  r->enable_pa=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void attenuation_value_changed_cb(GtkWidget *widget, gpointer data) {
  ADC *adc=(ADC *)data;
  adc->attenuation=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

static void enable_step_attenuation_cb(GtkWidget *widget,gpointer data) {
  ADC *adc=(ADC *)data;
  adc->enable_step_attenuation=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_high_priority();
  }
}

static void penelope_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *r=(RADIO *)data;
  r->penelope=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

/*
static void rigctl_cb(GtkWidget *widget, gpointer data) {
  int i;

  rigctl_enable=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if(rigctl_enable) {
    for(i=0;i<radio->discovered->supported_receivers;i++) {
      if(radio->receiver[i]!=NULL) {
        launch_rigctl(radio->receiver[i]);
      }
    }
    gtk_widget_set_sensitive(rigctl_base, FALSE); 
  } else {
    for(i=0;i<radio->discovered->supported_receivers;i++) {
      if(radio->receiver[i]!=NULL) {
        close_rigctl_ports(radio->receiver[i]);
      }
    }
    gtk_widget_set_sensitive(rigctl_base, TRUE); 
  }
}

static void rigctl_debug_cb(GtkWidget *widget, gpointer data) {
  rigctl_debug=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void rigctl_value_changed_cb(GtkWidget *widget, gpointer data) {
  rigctl_port_base=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}
*/

#ifdef CWDAEMON

static GThread *cwdaemon_thread_id;


static void cwdaemon_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->cwdaemon=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  printf("CWdaemon %d\n", radio->cwdaemon);
  if(radio->cwdaemon) {
    printf("Starting CWdaemon\n");
    cwdaemon_thread_id = g_thread_new("cwdaemon thread...", cwdaemon_thread, (gpointer)radio);
    if(!cwdaemon_thread_id)
    {
      fprintf(stderr,"g_thread_new failed on cwdaemon_thread\n");
      exit(-1);
    }
    fprintf(stderr, "cwdaemon_thread: id=%p\n",cwdaemon_thread_id);    
    // CWdaemon now has control over keyer settings, user can't modify
    gtk_widget_set_sensitive(cw_keyer_speed_b, FALSE);    
    gtk_widget_set_sensitive(cw_keyer_sidetone_frequency_b, FALSE);
    gtk_widget_set_sensitive(cw_keyer_weight_b, FALSE);
    gtk_widget_set_sensitive(cw_keyer_sidetone_level_b, FALSE);     
    gtk_widget_set_sensitive(cwport, FALSE);  
    
  } 
  else {
    //printf("Stopping CWdaemon\n");
    cwdaemon_stop();
    gtk_widget_set_sensitive(cw_keyer_speed_b, TRUE);
    gtk_widget_set_sensitive(cw_keyer_sidetone_frequency_b, TRUE);
    gtk_widget_set_sensitive(cw_keyer_weight_b, TRUE);
    gtk_widget_set_sensitive(cw_keyer_sidetone_level_b, TRUE);    
    gtk_widget_set_sensitive(cwport, TRUE); 
    //g_thread_exit(cwdaemon_thread_id);
  }
}

static void cwport_value_changed_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;  
  radio->cwd_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

#endif

GtkWidget *create_radio_dialog(RADIO *radio) {
  g_print("%s\n",__FUNCTION__);
  GtkWidget *grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(grid),5);

  int row=0;
  int col=0;

  GtkWidget *model_frame=gtk_frame_new("Radio Model");
  GtkWidget *model_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(model_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(model_grid),TRUE);
  gtk_container_add(GTK_CONTAINER(model_frame),model_grid);
  gtk_grid_attach(GTK_GRID(grid),model_frame,col,row++,1,1);

  int x=0;
  int y=0;

  GtkWidget *model_combo_box=gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANAN_10");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANAN_10E");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANAN_100");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANAN_100D");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANAN_200D");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANAN_7000DLE");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANAN_8000DLE");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ATLAS");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"HERMES");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"HERMES 2");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ANGELIA");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ORION");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"ORION 2");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"HERMES LITE");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"HERMES LITE 2");
#ifdef SOAPYSDR
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(model_combo_box),NULL,"SoapySDR");
#endif
  gtk_combo_box_set_active(GTK_COMBO_BOX(model_combo_box),radio->model);
  g_signal_connect(model_combo_box,"changed",G_CALLBACK(model_cb),radio);
  gtk_grid_attach(GTK_GRID(model_grid),model_combo_box,x,0,1,1);
  x++;
  if ((radio->discovered->device == DEVICE_HERMES_LITE2) || (radio->discovered->device == DEVICE_HERMES_LITE)) {
    GtkWidget *paswap=gtk_check_button_new_with_label("Enable PA");
    gtk_grid_attach(GTK_GRID(model_grid),paswap,x,0,1,1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paswap),radio->enable_pa);
    g_signal_connect(paswap,"toggled",G_CALLBACK(enablepa_changed_cb),radio);
  }
  else if(radio->discovered->device!=DEVICE_SOAPYSDR) {
    GtkWidget *iqswap=gtk_check_button_new_with_label("Swap I & Q");
    gtk_grid_attach(GTK_GRID(model_grid),iqswap,x,0,1,1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iqswap),radio->iqswap);
    g_signal_connect(iqswap,"toggled",G_CALLBACK(iqswap_changed_cb),radio);
  } else if(radio->discovered->device!=DEVICE_METIS) {
    GtkWidget *penelope=gtk_check_button_new_with_label("Penelope");
    gtk_grid_attach(GTK_GRID(model_grid),penelope,x,0,1,1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(penelope),radio->penelope);
    g_signal_connect(penelope,"toggled",G_CALLBACK(penelope_changed_cb),radio);
  }
  x++;

#ifdef SOAPYSDR
  if(radio->discovered->device!=DEVICE_SOAPYSDR) {
#endif
    filter_board_combo_box=gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(filter_board_combo_box),NULL,"NONE");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(filter_board_combo_box),NULL,"ALEX FILTERS");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(filter_board_combo_box),NULL,"APOLLO FILTERS");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(filter_board_combo_box),NULL,"N2ADR FILTERS");
    gtk_combo_box_set_active(GTK_COMBO_BOX(filter_board_combo_box),radio->filter_board);
    g_signal_connect(filter_board_combo_box,"changed",G_CALLBACK(filter_board_cb),radio);
    gtk_grid_attach(GTK_GRID(model_grid),filter_board_combo_box,x,0,1,1);
    x++;
#ifdef SOAPYSDR
  }
#endif

  if(radio->discovered->protocol==PROTOCOL_1) {
    GtkWidget *sample_rate_combo_box=gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"48000");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"96000");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"192000");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"384000");
    switch(radio->sample_rate) {
      case 48000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),0);
        break;
      case 96000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),1);
        break;
      case 192000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),2);
        break;
      case 384000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),3);
        break;
    }
    g_signal_connect(sample_rate_combo_box,"changed",G_CALLBACK(sample_rate_cb),radio);
    gtk_grid_attach(GTK_GRID(model_grid),sample_rate_combo_box,x,0,1,1);
  }

#ifdef SOAPYSDR
  if(radio->discovered->device==DEVICE_SOAPYSDR &&
     strcmp(radio->discovered->name,"sdrplay")==0) {
    GtkWidget *sample_rate_combo_box=gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"96000");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"192000");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"384000");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sample_rate_combo_box),NULL,"768000");
    switch(radio->sample_rate) {
      case 96000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),0);
        break;
      case 192000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),1);
        break;
      case 384000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),2);
        break;
      case 768000:
        gtk_combo_box_set_active(GTK_COMBO_BOX(sample_rate_combo_box),3);
        break;
    }
    g_signal_connect(sample_rate_combo_box,"changed",G_CALLBACK(sample_rate_cb),radio);
    gtk_grid_attach(GTK_GRID(model_grid),sample_rate_combo_box,x,0,1,1);
  }
#endif


  adc0_frame=gtk_frame_new("ADC-0");
  GtkWidget *adc0_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(adc0_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(adc0_grid),TRUE);
  gtk_container_add(GTK_CONTAINER(adc0_frame),adc0_grid);
  gtk_grid_attach(GTK_GRID(grid),adc0_frame,col,row++,1,1);
 
  switch(radio->discovered->device) {

#ifdef SOAPYSDR
    case DEVICE_SOAPYSDR:
      {
      GtkWidget *antenna_label=gtk_label_new("Antenna:");
      gtk_grid_attach(GTK_GRID(adc0_grid),antenna_label,0,0,1,1);
      adc0_antenna_combo_box=gtk_combo_box_text_new();

      for(int i=0;i<radio->discovered->info.soapy.rx_antennas;i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_antenna_combo_box),NULL,radio->discovered->info.soapy.rx_antenna[i]);
      }

      gtk_combo_box_set_active(GTK_COMBO_BOX(adc0_antenna_combo_box),radio->adc[0].antenna);
      g_signal_connect(adc0_antenna_combo_box,"changed",G_CALLBACK(adc0_antenna_cb),radio);
      gtk_grid_attach(GTK_GRID(adc0_grid),adc0_antenna_combo_box,1,0,1,1);

      GtkWidget *adc_gain_label=gtk_label_new(NULL);
      gtk_label_set_markup(GTK_LABEL(adc_gain_label), "<b>Gain</b>");
      gtk_grid_attach(GTK_GRID(adc0_grid),adc_gain_label,0,1,1,1);

      double max=100;
      if(strcmp(radio->discovered->name,"lime")==0) {
        max=60.0;
      } else if(strcmp(radio->discovered->name,"plutosdr")==0) {
        max=73.0;
      }
      GtkWidget *adc_gain_b=gtk_spin_button_new_with_range(0.0,max,1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(adc_gain_b),radio->adc[0].gain);
      gtk_grid_attach(GTK_GRID(adc0_grid),adc_gain_b,1,1,1,1);
      g_signal_connect(adc_gain_b,"value_changed",G_CALLBACK(adc_gain_value_changed_cb),&radio->adc[0]);

      if(radio->discovered->info.soapy.rx_has_automatic_gain) {
        GtkWidget *agc=gtk_check_button_new_with_label("Hardware AGC: ");
        gtk_grid_attach(GTK_GRID(adc0_grid),agc,1,2,1,1);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(agc),radio->adc[0].agc);
        g_signal_connect(agc,"toggled",G_CALLBACK(agc_changed_cb),&radio->adc[0]);
      }
      }
      break;
#endif
    case DEVICE_HERMES_LITE2:
      attenuation_label=gtk_label_new("LNA gain (dB):");
      gtk_grid_attach(GTK_GRID(adc0_grid),attenuation_label,0,0,1,1);
      attenuation_b=gtk_spin_button_new_with_range(-12.0,48.0,1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(attenuation_b),(double)radio->adc[0].attenuation);
      gtk_grid_attach(GTK_GRID(adc0_grid),attenuation_b,2,0,1,1);
      g_signal_connect(attenuation_b,"value_changed",G_CALLBACK(attenuation_value_changed_cb),&radio->adc[0]);

      disable_fpgaclk_b=gtk_check_button_new_with_label("FPGA PSU clock");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (disable_fpgaclk_b), radio->psu_clk);
      gtk_grid_attach(GTK_GRID(adc0_grid),disable_fpgaclk_b,0,1,1,1);
      g_signal_connect(disable_fpgaclk_b,"toggled",G_CALLBACK(psu_clk_cb),radio);
      break;    

    case DEVICE_HERMES_LITE:
      attenuation_label=gtk_label_new("LNA gain (dB):");
      gtk_grid_attach(GTK_GRID(adc0_grid),attenuation_label,0,0,1,1);
      attenuation_b=gtk_spin_button_new_with_range(-12.0,48.0,1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(attenuation_b),(double)radio->adc[0].attenuation);
      gtk_grid_attach(GTK_GRID(adc0_grid),attenuation_b,2,0,1,1);
      g_signal_connect(attenuation_b,"value_changed",G_CALLBACK(attenuation_value_changed_cb),&radio->adc[0]);
      
      enable_attenuation_b=gtk_check_button_new_with_label("Enable 20dB Attenuation");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (enable_attenuation_b), radio->adc[0].dither);
      gtk_grid_attach(GTK_GRID(adc0_grid),enable_attenuation_b,0,1,1,1);
      //g_signal_connect(enable_attenuation_b,"toggled",G_CALLBACK(enable_step_attenuation_cb),&radio->adc[0]);
      g_signal_connect(enable_attenuation_b,"toggled",G_CALLBACK(dither_cb),&radio->adc[0]);
      
      break;
    
    default:
      adc0_antenna_combo_box=gtk_combo_box_text_new();
      // Row 1
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_antenna_combo_box),NULL,"ANT_1");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_antenna_combo_box),NULL,"ANT_2");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_antenna_combo_box),NULL,"ANT_3");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_antenna_combo_box),NULL,"XVTR");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_antenna_combo_box),NULL,"EXT1");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_antenna_combo_box),NULL,"EXT2");
      gtk_combo_box_set_active(GTK_COMBO_BOX(adc0_antenna_combo_box),radio->adc[0].antenna);
      g_signal_connect(adc0_antenna_combo_box,"changed",G_CALLBACK(adc0_antenna_cb),radio);
      gtk_grid_attach(GTK_GRID(adc0_grid),adc0_antenna_combo_box,0,0,1,1);

      adc0_filters_combo_box=gtk_combo_box_text_new();
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_filters_combo_box),NULL,"Automatic");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_filters_combo_box),NULL,"Manual");
      gtk_combo_box_set_active(GTK_COMBO_BOX(adc0_filters_combo_box),radio->adc[0].filters);
      g_signal_connect(adc0_filters_combo_box,"changed",G_CALLBACK(adc0_filters_cb),radio);
      gtk_grid_attach(GTK_GRID(adc0_grid),adc0_filters_combo_box,1,0,1,1);

      adc0_hpf_combo_box=gtk_combo_box_text_new();
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_hpf_combo_box),NULL,"BYPASS HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_hpf_combo_box),NULL,"1.5 MHz HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_hpf_combo_box),NULL,"6.5 MHZ HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_hpf_combo_box),NULL,"9.5 MHz HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_hpf_combo_box),NULL,"13 MHz HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_hpf_combo_box),NULL,"20 MHz HPF");
      gtk_combo_box_set_active(GTK_COMBO_BOX(adc0_hpf_combo_box),radio->adc[0].hpf);
      g_signal_connect(adc0_hpf_combo_box,"changed",G_CALLBACK(adc0_hpf_cb),radio);
      gtk_grid_attach(GTK_GRID(adc0_grid),adc0_hpf_combo_box,2,0,1,1);
  
      adc0_lpf_combo_box=gtk_combo_box_text_new();
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_lpf_combo_box),NULL,"160m LPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_lpf_combo_box),NULL,"80m LPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_lpf_combo_box),NULL,"60/40m LPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_lpf_combo_box),NULL,"30/20m LPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_lpf_combo_box),NULL,"17/15m LPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_lpf_combo_box),NULL,"12/10m LPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc0_lpf_combo_box),NULL,"6m LPF");
      gtk_combo_box_set_active(GTK_COMBO_BOX(adc0_lpf_combo_box),radio->adc[0].lpf);
      g_signal_connect(adc0_lpf_combo_box,"changed",G_CALLBACK(adc0_lpf_cb),radio);
      gtk_grid_attach(GTK_GRID(adc0_grid),adc0_lpf_combo_box,3,0,1,1);

      // Row 2
      dither_b=gtk_check_button_new_with_label("Dither");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dither_b), radio->adc[0].dither);
      gtk_grid_attach(GTK_GRID(adc0_grid),dither_b,0,1,1,1);
      g_signal_connect(dither_b,"toggled",G_CALLBACK(dither_cb),&radio->adc[0]);
  
      random_b=gtk_check_button_new_with_label("Random");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (random_b), radio->adc[0].random);
      gtk_grid_attach(GTK_GRID(adc0_grid),random_b,1,1,1,1);
      g_signal_connect(random_b,"toggled",G_CALLBACK(random_cb),&radio->adc[0]);
  
      preamp_b=gtk_check_button_new_with_label("Preamp");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preamp_b), radio->adc[0].preamp);
      gtk_grid_attach(GTK_GRID(adc0_grid),preamp_b,2,1,1,1);
      g_signal_connect(preamp_b,"toggled",G_CALLBACK(preamp_cb),&radio->adc[0]);
  
      attenuation_label=gtk_label_new("Attenuation (dB):");
      gtk_grid_attach(GTK_GRID(adc0_grid),attenuation_label,3,1,1,1);
  
      attenuation_b=gtk_spin_button_new_with_range(0.0,31.0,1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(attenuation_b),(double)radio->adc[0].attenuation);
      gtk_grid_attach(GTK_GRID(adc0_grid),attenuation_b,4,1,1,1);
      g_signal_connect(attenuation_b,"value_changed",G_CALLBACK(attenuation_value_changed_cb),&radio->adc[0]);
      break;
  }

  switch(radio->discovered->device) {
    case DEVICE_HERMES2:
    case DEVICE_ANGELIA:
    case DEVICE_ORION:
    case DEVICE_ORION2:
      adc1_frame=gtk_frame_new("ADC-1");
      GtkWidget *adc1_grid=gtk_grid_new();
      gtk_grid_set_row_homogeneous(GTK_GRID(adc1_grid),TRUE);
      gtk_grid_set_column_homogeneous(GTK_GRID(adc1_grid),TRUE);
      gtk_container_add(GTK_CONTAINER(adc1_frame),adc1_grid);
      gtk_grid_attach(GTK_GRID(grid),adc1_frame,col,row++,1,1);

      adc1_antenna_combo_box=gtk_combo_box_text_new();
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_antenna_combo_box),NULL,"RX2");
      gtk_combo_box_set_active(GTK_COMBO_BOX(adc1_antenna_combo_box),radio->adc[1].antenna);
      g_signal_connect(adc1_antenna_combo_box,"changed",G_CALLBACK(adc1_antenna_cb),radio);
      gtk_grid_attach(GTK_GRID(adc1_grid),adc1_antenna_combo_box,0,0,1,1);

      adc1_filters_combo_box=gtk_combo_box_text_new();
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_filters_combo_box),NULL,"Automatic");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_filters_combo_box),NULL,"Manual");
      gtk_combo_box_set_active(GTK_COMBO_BOX(adc1_filters_combo_box),radio->adc[1].filters);
      g_signal_connect(adc1_filters_combo_box,"changed",G_CALLBACK(adc1_filters_cb),radio);
      gtk_grid_attach(GTK_GRID(adc1_grid),adc1_filters_combo_box,1,0,1,1);

      adc1_hpf_combo_box=gtk_combo_box_text_new();
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_hpf_combo_box),NULL,"BYPASS HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_hpf_combo_box),NULL,"1.5 MHz HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_hpf_combo_box),NULL,"6.5 MHZ HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_hpf_combo_box),NULL,"9.5 MHz HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_hpf_combo_box),NULL,"13 MHz HPF");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(adc1_hpf_combo_box),NULL,"20 MHz HPF");
      gtk_combo_box_set_active(GTK_COMBO_BOX(adc1_hpf_combo_box),radio->adc[1].hpf);
      g_signal_connect(adc1_hpf_combo_box,"changed",G_CALLBACK(adc1_hpf_cb),radio);
      gtk_grid_attach(GTK_GRID(adc1_grid),adc1_hpf_combo_box,2,0,1,1);

      dither_b=gtk_check_button_new_with_label("Dither");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dither_b), radio->adc[1].dither);
      gtk_grid_attach(GTK_GRID(adc1_grid),dither_b,0,1,1,1);
      g_signal_connect(dither_b,"toggled",G_CALLBACK(dither_cb),&radio->adc[1]);

      random_b=gtk_check_button_new_with_label("Random");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (random_b), radio->adc[1].random);
      gtk_grid_attach(GTK_GRID(adc1_grid),random_b,1,1,1,1);
      g_signal_connect(random_b,"toggled",G_CALLBACK(random_cb),&radio->adc[1]);

      preamp_b=gtk_check_button_new_with_label("Preamp");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preamp_b), radio->adc[1].preamp);
      gtk_grid_attach(GTK_GRID(adc1_grid),preamp_b,2,1,1,1);
      g_signal_connect(preamp_b,"toggled",G_CALLBACK(preamp_cb),&radio->adc[1]);

      attenuation_label=gtk_label_new("Attenuation (dB):");
      gtk_grid_attach(GTK_GRID(adc1_grid),attenuation_label,3,1,1,1);

      attenuation_b=gtk_spin_button_new_with_range(0.0,31.0,1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(attenuation_b),(double)radio->adc[1].attenuation);
      gtk_grid_attach(GTK_GRID(adc1_grid),attenuation_b,4,1,1,1);
      g_signal_connect(attenuation_b,"value_changed",G_CALLBACK(attenuation_value_changed_cb),&radio->adc[1]);
      break;
    default:
      break;
  }

  radio_dialog_update_controls();

  switch(radio->discovered->device) {
#ifdef SOAPYSDR
    case DEVICE_SOAPYSDR:
      if(radio->can_transmit) {
        int r=0;
        dac0_frame=gtk_frame_new("DAC-0");
        GtkWidget *dac0_grid=gtk_grid_new();
        gtk_grid_set_row_homogeneous(GTK_GRID(dac0_grid),TRUE);
        gtk_grid_set_column_homogeneous(GTK_GRID(dac0_grid),TRUE);
        gtk_container_add(GTK_CONTAINER(dac0_frame),dac0_grid);
        gtk_grid_attach(GTK_GRID(grid),dac0_frame,col,row++,1,1);

        GtkWidget *antenna_label=gtk_label_new("Antenna:");
        gtk_grid_attach(GTK_GRID(dac0_grid),antenna_label,0,r,1,1);

        dac0_antenna_combo_box=gtk_combo_box_text_new();
        for(int i=0;i<radio->discovered->info.soapy.tx_antennas;i++) {
          gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dac0_antenna_combo_box),NULL,radio->discovered->info.soapy.tx_antenna[i]);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(dac0_antenna_combo_box),radio->dac[0].antenna);
        g_signal_connect(dac0_antenna_combo_box,"changed",G_CALLBACK(dac0_antenna_cb),radio);
        gtk_grid_attach(GTK_GRID(dac0_grid),dac0_antenna_combo_box,1,r,1,1);
        r++;

        GtkWidget *gain_label=gtk_label_new("Gain");
        gtk_grid_attach(GTK_GRID(dac0_grid),gain_label,0,r,1,1);

        double max=100.0;
        if(strcmp(radio->discovered->name,"lime")==0) {
          max=64.0;
        } else if(strcmp(radio->discovered->name,"plutosdr")==0) {
          max=89.0;
        }

        GtkWidget *gain_b=gtk_spin_button_new_with_range(0.0,max,1.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(gain_b),radio->dac[0].gain);
        gtk_grid_attach(GTK_GRID(dac0_grid),gain_b,1,r,1,1);
        g_signal_connect(gain_b,"value_changed",G_CALLBACK(dac0_gain_value_changed_cb),&radio->dac[0]);
        r++;
      }
      break;
#endif
    default:
      break;
  }


  if(radio->discovered->device==DEVICE_ANGELIA ||
     radio->discovered->device==DEVICE_ORION ||
     radio->discovered->device==DEVICE_ORION2) {
    GtkWidget *mic_frame=gtk_frame_new("Microphone");
    GtkWidget *mic_grid=gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(mic_grid),TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(mic_grid),FALSE);
    gtk_container_add(GTK_CONTAINER(mic_frame),mic_grid);
    gtk_grid_attach(GTK_GRID(grid),mic_frame,col,row++,1,1);
   
    x=0;
    y=0;

    GtkWidget *ptt_ring_b=gtk_radio_button_new_with_label(NULL,"PTT On Ring, Mic and Bias on Tip");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ptt_ring_b), radio->mic_ptt_tip_bias_ring==0);
    gtk_grid_attach(GTK_GRID(mic_grid),ptt_ring_b,x,y++,1,1);
    g_signal_connect(ptt_ring_b,"toggled",G_CALLBACK(ptt_ring_cb),radio);

    GtkWidget *ptt_tip_b=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ptt_ring_b),"PTT On Tip, Mic and Bias on Ring");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ptt_tip_b), radio->mic_ptt_tip_bias_ring==1);
    gtk_grid_attach(GTK_GRID(mic_grid),ptt_tip_b,x,y++,1,1);
    g_signal_connect(ptt_tip_b,"toggled",G_CALLBACK(ptt_tip_cb),radio);

    GtkWidget *ptt_b=gtk_check_button_new_with_label("PTT Enabled");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ptt_b), radio->mic_ptt_enabled);
    gtk_grid_attach(GTK_GRID(mic_grid),ptt_b,x,y++,1,1);
    g_signal_connect(ptt_b,"toggled",G_CALLBACK(ptt_cb),radio);

    GtkWidget *bias_b=gtk_check_button_new_with_label("BIAS Enabled");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bias_b), radio->mic_bias_enabled);
    gtk_grid_attach(GTK_GRID(mic_grid),bias_b,x,y++,1,1);
    g_signal_connect(bias_b,"toggled",G_CALLBACK(bias_cb),radio);

    GtkWidget *boost_b=gtk_check_button_new_with_label("20dB boost");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (boost_b), radio->mic_boost);
    gtk_grid_attach(GTK_GRID(mic_grid),boost_b,x,y++,1,1);
    g_signal_connect(boost_b,"toggled",G_CALLBACK(boost_cb),radio);
  }

  GtkWidget *config_frame=gtk_frame_new("Configuration");
  GtkWidget *config_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(config_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(config_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(config_frame),config_grid);
  gtk_grid_attach(GTK_GRID(grid),config_frame,col,row++,1,1);

#ifndef SOAPYSDR
  // SWR alarm threshold
  GtkWidget *swr_alarm_label=gtk_label_new("SWR alarm at ");
  gtk_widget_show(swr_alarm_label);
  gtk_grid_attach(GTK_GRID(config_grid),swr_alarm_label,0,0,1,1);

  swr_alarm_b=gtk_spin_button_new_with_range(1.0, 5.0, 0.1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(swr_alarm_b), radio->swr_alarm_value);
  gtk_widget_show(swr_alarm_b);
  gtk_grid_attach(GTK_GRID(config_grid),swr_alarm_b,1,0,1,1);
  g_signal_connect(swr_alarm_b,"value_changed",G_CALLBACK(swr_alarm_changed_cb),radio);

  // Temperature alarm threshold
  if (radio->discovered->device == DEVICE_HERMES_LITE2) {
    GtkWidget *temp_alarm_label=gtk_label_new("Temp alarm at ");
    gtk_widget_show(temp_alarm_label);
    gtk_grid_attach(GTK_GRID(config_grid),temp_alarm_label,2,0,1,1);

    temperature_alarm_b=gtk_spin_button_new_with_range(30.0, 60.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperature_alarm_b), (double)radio->temperature_alarm_value);
    gtk_widget_show(temperature_alarm_b);
    gtk_grid_attach(GTK_GRID(config_grid),temperature_alarm_b,3,0,1,1);
    g_signal_connect(temperature_alarm_b,"value_changed",G_CALLBACK(temperature_alarm_changed_cb),radio);
  }
#endif

  GtkWidget *audio_frame=gtk_frame_new("Audio");
  GtkWidget *audio_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(audio_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(audio_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(audio_frame),audio_grid);
  gtk_grid_attach(GTK_GRID(grid),audio_frame,col,row++,1,1);

  GtkWidget *audio_combo=gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(audio_combo),NULL,"SOUNDIO");
#ifndef __APPLE__
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(audio_combo),NULL,"PULSEAUDIO");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(audio_combo),NULL,"ALSA");
#endif
  gtk_combo_box_set_active(GTK_COMBO_BOX(audio_combo),radio->which_audio);
  gtk_grid_attach(GTK_GRID(audio_grid),audio_combo,0,0,1,1);
  g_signal_connect(audio_combo,"changed",G_CALLBACK(audio_cb),radio);

  GtkWidget *backend_label=gtk_label_new(" Backend:");
  gtk_grid_attach(GTK_GRID(audio_grid),backend_label,1,0,1,1);

  audio_backend_combo_box=gtk_combo_box_text_new();
  update_audio_backends(radio);
  gtk_combo_box_set_active(GTK_COMBO_BOX(audio_backend_combo_box),radio->which_audio_backend);
  gtk_grid_attach(GTK_GRID(audio_grid),audio_backend_combo_box,2,0,1,1);
  g_signal_connect(audio_backend_combo_box,"changed",G_CALLBACK(audio_backend_cb),radio);


  GtkWidget *calibration_frame=gtk_frame_new("Calibration [dBm]");
  GtkWidget *calibration_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(calibration_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(calibration_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(calibration_frame),calibration_grid);
  gtk_grid_attach(GTK_GRID(grid),calibration_frame,col,row++,1,1);

  GtkWidget *smeter_label=gtk_label_new(" S-Meter:");
  gtk_grid_attach(GTK_GRID(calibration_grid),smeter_label,0,1,1,1);

  GtkWidget *smeter_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(radio->meter_calibration, -100.0, 100.0, 1.0, 1.0, 1.0));
  gtk_widget_set_size_request(smeter_scale,200,30);
  gtk_widget_show(smeter_scale);
  g_signal_connect(G_OBJECT(smeter_scale),"value_changed",G_CALLBACK(smeter_calibrate_changed_cb),radio);
  gtk_grid_attach(GTK_GRID(calibration_grid),smeter_scale,1,1,1,1);

  GtkWidget *panadapter_label=gtk_label_new(" Panadapter:");
  gtk_grid_attach(GTK_GRID(calibration_grid),panadapter_label,0,2,1,1);

 GtkWidget *panadapter_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(radio->panadapter_calibration, -100.0, 100.0, 1.0, 1.0, 1.0));
  gtk_widget_set_size_request(panadapter_scale,200,30);
  gtk_range_set_value (GTK_RANGE(panadapter_scale),radio->panadapter_calibration);
  gtk_widget_show(panadapter_scale);
  g_signal_connect(G_OBJECT(panadapter_scale),"value_changed",G_CALLBACK(panadapter_calibrate_changed_cb),radio);
  gtk_grid_attach(GTK_GRID(calibration_grid),panadapter_scale,1,2,1,1);

  GtkWidget *cw_frame=gtk_frame_new("CW");
  GtkWidget *cw_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(cw_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(cw_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(cw_frame),cw_grid);
  gtk_grid_attach(GTK_GRID(grid),cw_frame,col,row++,1,1);

  x=0;
  y=0;

  if (radio->discovered->device != DEVICE_HERMES_LITE2) {
    GtkWidget *cw_keyer_mode_label=gtk_label_new("Keyer Mode:");
    gtk_widget_show(cw_keyer_mode_label);
    gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_mode_label,x++,y,1,1);

    GtkWidget *cw_keyer_combo_box=gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cw_keyer_combo_box),NULL,"Straight Key");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cw_keyer_combo_box),NULL,"Mode A");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cw_keyer_combo_box),NULL,"Mode B");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cw_keyer_combo_box),radio->cw_keyer_mode);
    g_signal_connect(cw_keyer_combo_box,"changed",G_CALLBACK(cw_keyer_cb),radio);
    gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_combo_box,x++,y,1,1);
  }

  if ((radio->discovered->device == DEVICE_HERMES_LITE2) || (radio->discovered->device == DEVICE_HERMES)) {
    #ifdef CWDAEMON    
    GtkWidget *cwdaemon_label=gtk_label_new("CWdaemon enabled:");
    gtk_widget_show(cwdaemon_label);
    gtk_grid_attach(GTK_GRID(cw_grid),cwdaemon_label,x++,y,1,1);

    GtkWidget *cwdaemon_tick=gtk_check_button_new();
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cwdaemon_tick), radio->cwdaemon);
    gtk_widget_show(cwdaemon_tick);
    gtk_grid_attach(GTK_GRID(cw_grid),cwdaemon_tick,x++,y,1,1);
    g_signal_connect(cwdaemon_tick,"toggled",G_CALLBACK(cwdaemon_cb),radio);    
        
    GtkWidget *cwport_label=gtk_label_new("Port:");
    gtk_widget_show(cwport_label);
    gtk_grid_attach(GTK_GRID(cw_grid),cwport_label,x++,y,1,1);
  
    cwport=gtk_spin_button_new_with_range(50000.0,52000.0,1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(cwport), radio->cwd_port);
    gtk_grid_attach(GTK_GRID(cw_grid),cwport,x,y,1,1);
    g_signal_connect(cwport,"value_changed",G_CALLBACK(cwport_value_changed_cb),NULL);
   
    
    #endif
  }
  else {
    GtkWidget *cw_keyer_reversed_label=gtk_label_new("Keys Reversed:");
    gtk_widget_show(cw_keyer_reversed_label);
    gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_reversed_label,x++,y,1,1);

    GtkWidget *cw_keys_reversed_b=gtk_check_button_new();
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_keys_reversed_b), radio->cw_keys_reversed);
    gtk_widget_show(cw_keys_reversed_b);
    gtk_grid_attach(GTK_GRID(cw_grid),cw_keys_reversed_b,x++,y,1,1);
    g_signal_connect(cw_keys_reversed_b,"toggled",G_CALLBACK(cw_keys_reversed_cb),radio);
  }
  x=0;
  y++;


  GtkWidget *cw_speed_label=gtk_label_new("CW Speed (WPM)");
  gtk_widget_show(cw_speed_label);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_speed_label,x++,y,1,1);


  cw_keyer_speed_b=gtk_spin_button_new_with_range(1.0,60.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_speed_b),(double)radio->cw_keyer_speed);
  gtk_widget_show(cw_keyer_speed_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_speed_b,x++,y,1,1);
  g_signal_connect(cw_keyer_speed_b,"value_changed",G_CALLBACK(cw_keyer_speed_value_changed_cb),radio);

  GtkWidget *cw_keyer_sidetone_level_label=gtk_label_new("Sidetone Level:");
  gtk_widget_show(cw_keyer_sidetone_level_label);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_sidetone_level_label,x++,y,1,1);

  cw_keyer_sidetone_level_b=gtk_spin_button_new_with_range(1.0,radio->discovered->protocol==PROTOCOL_2?255.0:127.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_sidetone_level_b),(double)radio->cw_keyer_sidetone_volume);
  gtk_widget_show(cw_keyer_sidetone_level_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_sidetone_level_b,x++,y,1,1);
  g_signal_connect(cw_keyer_sidetone_level_b,"value_changed",G_CALLBACK(cw_keyer_sidetone_level_value_changed_cb),radio);

  x=0;
  y++;

  GtkWidget *cw_keyer_sidetone_frequency_label=gtk_label_new("Sidetone Freq:");
  gtk_widget_show(cw_keyer_sidetone_frequency_label);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_sidetone_frequency_label,x++,y,1,1);

  cw_keyer_sidetone_frequency_b=gtk_spin_button_new_with_range(100.0,1000.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_sidetone_frequency_b),(double)radio->cw_keyer_sidetone_frequency);
  gtk_widget_show(cw_keyer_sidetone_frequency_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_sidetone_frequency_b,x++,y,1,1);
  g_signal_connect(cw_keyer_sidetone_frequency_b,"value_changed",G_CALLBACK(cw_keyer_sidetone_frequency_value_changed_cb),radio);

  GtkWidget *cw_keyer_weight_label=gtk_label_new("Weight:");
  gtk_widget_show(cw_keyer_weight_label);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_weight_label,x++,y,1,1);

  cw_keyer_weight_b=gtk_spin_button_new_with_range(0.0,100.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_weight_b),(double)radio->cw_keyer_weight);
  gtk_widget_show(cw_keyer_weight_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_weight_b,x++,y,1,1);
  g_signal_connect(cw_keyer_weight_b,"value_changed",G_CALLBACK(cw_keyer_weight_value_changed_cb),radio);

  x=0;
  y++;

  GtkWidget *cw_keyer_breakin_label=gtk_label_new("CW Break In:");
  gtk_widget_show(cw_keyer_breakin_label);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_breakin_label,x++,y,1,1);

  GtkWidget *cw_breakin_b=gtk_check_button_new();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cw_breakin_b), radio->cw_breakin);
  gtk_widget_show(cw_breakin_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_breakin_b,x++,y,1,1);
  g_signal_connect(cw_breakin_b,"toggled",G_CALLBACK(cw_breakin_cb),radio);

  GtkWidget *cw_keyer_delay_label=gtk_label_new("Break In Delay (Ms):");
  gtk_widget_show(cw_keyer_delay_label);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_delay_label,x++,y,1,1);

  GtkWidget *cw_keyer_hang_time_b=gtk_spin_button_new_with_range(0.0,1000.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(cw_keyer_hang_time_b),(double)radio->cw_keyer_hang_time);
  gtk_widget_show(cw_keyer_hang_time_b);
  gtk_grid_attach(GTK_GRID(cw_grid),cw_keyer_hang_time_b,x++,y,1,1);
  g_signal_connect(cw_keyer_hang_time_b,"value_changed",G_CALLBACK(cw_keyer_hang_time_value_changed_cb),radio);

  x=0;
  y=0;

  GtkWidget *region_frame=gtk_frame_new("Region");
  GtkWidget *region_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(region_grid),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(region_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(region_frame),region_grid);
  gtk_grid_attach(GTK_GRID(grid),region_frame,col,row++,1,1);

  GtkWidget *region_combo=gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(region_combo),NULL,"Other");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(region_combo),NULL,"UK");
  gtk_combo_box_set_active(GTK_COMBO_BOX(region_combo),radio->region);
  gtk_grid_attach(GTK_GRID(region_grid),region_combo,0,0,1,1);
  g_signal_connect(region_combo,"changed",G_CALLBACK(region_cb),radio);

  x=0;
  y=0;

  return grid;
}
