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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wdsp.h>

#include "agc.h"
#include "mode.h"
#include "filter.h"
#include "bandstack.h"
#include "band.h"
#include "discovered.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "radio.h"
#include "main.h"
#include "vfo.h"
#include "meter.h"
#include "rx_panadapter.h"
#include "tx_panadapter.h"
#include "waterfall.h"
#include "protocol1.h"
#include "protocol2.h"
#include "audio.h"
//#include "receiver_dialog.h"
#include "configure_dialog.h"
#include "property.h"
#include "rigctl.h"

void receiver_save_state(RECEIVER *rx) {
  char name[80];
  char value[80];
  int i;
  gint x;
  gint y;
  gint width;
  gint height;

  sprintf(name,"receiver[%d].channel",rx->channel);
  sprintf(value,"%d",rx->channel);
  setProperty(name,value);
  sprintf(name,"receiver[%d].adc",rx->channel);
  sprintf(value,"%d",rx->adc);
  setProperty(name,value);
  sprintf(name,"receiver[%d].sample_rate",rx->channel);
  sprintf(value,"%d",rx->sample_rate);
  setProperty(name,value);
  sprintf(name,"receiver[%d].dsp_rate",rx->channel);
  sprintf(value,"%d",rx->dsp_rate);
  setProperty(name,value);
  sprintf(name,"receiver[%d].output_rate",rx->channel);
  sprintf(value,"%d",rx->output_rate);
  setProperty(name,value);
  sprintf(name,"receiver[%d].buffer_size",rx->channel);
  sprintf(value,"%d",rx->buffer_size);
  setProperty(name,value);
  sprintf(name,"receiver[%d].fft_size",rx->channel);
  sprintf(value,"%d",rx->fft_size);
  setProperty(name,value);
  sprintf(name,"receiver[%d].low_latency",rx->channel);
  sprintf(value,"%d",rx->low_latency);
  setProperty(name,value);
  sprintf(name,"receiver[%d].low_latency",rx->channel);
  sprintf(value,"%d",rx->low_latency);
  setProperty(name,value);
  sprintf(name,"receiver[%d].fps",rx->channel);
  sprintf(value,"%d",rx->fps);
  setProperty(name,value);

  sprintf(name,"receiver[%d].display_average_time",rx->channel);
  sprintf(value,"%f",rx->display_average_time);
  setProperty(name,value);  
  sprintf(name,"receiver[%d].panadapter_low",rx->channel);
  sprintf(value,"%d",rx->panadapter_low);
  setProperty(name,value);  
  sprintf(name,"receiver[%d].panadapter_high",rx->channel);
  sprintf(value,"%d",rx->panadapter_high);
  setProperty(name,value);
  sprintf(name,"receiver[%d].panadapter_filled",rx->channel);
  sprintf(value,"%d",rx->panadapter_filled);
  setProperty(name,value);
  sprintf(name,"receiver[%d].panadapter_gradient",rx->channel);
  sprintf(value,"%d",rx->panadapter_gradient);
  setProperty(name,value);
  sprintf(name,"receiver[%d].panadapter_agc_line",rx->channel);
  sprintf(value,"%d",rx->panadapter_agc_line);
  setProperty(name,value);

  if(rx->waterfall_automatic == FALSE) {
      sprintf(name,"receiver[%d].waterfall_low",rx->channel);
      sprintf(value,"%d",rx->waterfall_low);
      setProperty(name,value);  
      sprintf(name,"receiver[%d].waterfall_high",rx->channel);
      sprintf(value,"%d",rx->waterfall_high);
      setProperty(name,value);
  }
  sprintf(name,"receiver[%d].waterfall_automatic",rx->channel);
  sprintf(value,"%d",rx->waterfall_automatic);
  setProperty(name,value);
  sprintf(name,"receiver[%d].waterfall_ft8_marker",rx->channel);
  sprintf(value,"%d",rx->waterfall_ft8_marker);
  setProperty(name,value);

  sprintf(name,"receiver[%d].frequency_a",rx->channel);
  sprintf(value,"%ld",rx->frequency_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].lo_a",rx->channel);
  sprintf(value,"%ld",rx->lo_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].error_a",rx->channel);
  sprintf(value,"%ld",rx->error_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].lo_tx",rx->channel);
  sprintf(value,"%ld",rx->lo_tx);
  setProperty(name,value);
  sprintf(name,"receiver[%d].tx_track_rx",rx->channel);
  sprintf(value,"%d",rx->tx_track_rx);
  setProperty(name,value);
  sprintf(name,"receiver[%d].band_a",rx->channel);
  sprintf(value,"%d",rx->band_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].mode_a",rx->channel);
  sprintf(value,"%d",rx->mode_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].filter_a",rx->channel);
  sprintf(value,"%d",rx->filter_a);
  setProperty(name,value);


  sprintf(name,"receiver[%d].frequency_b",rx->channel);
  sprintf(value,"%ld",rx->frequency_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].lo_b",rx->channel);
  sprintf(value,"%ld",rx->lo_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].error_b",rx->channel);
  sprintf(value,"%ld",rx->error_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].band_b",rx->channel);
  sprintf(value,"%d",rx->band_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].mode_b",rx->channel);
  sprintf(value,"%d",rx->mode_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].filter_b",rx->channel);
  sprintf(value,"%d",rx->filter_b);
  setProperty(name,value);

  sprintf(name,"receiver[%d].ctun",rx->channel);
  sprintf(value,"%d",rx->ctun);
  setProperty(name,value);
  sprintf(name,"receiver[%d].ctun_offset",rx->channel);
  sprintf(value,"%ld",rx->ctun_offset);
  setProperty(name,value);
  sprintf(name,"receiver[%d].ctun_min",rx->channel);
  sprintf(value,"%ld",rx->ctun_min);
  setProperty(name,value);
  sprintf(name,"receiver[%d].ctun_max",rx->channel);
  sprintf(value,"%ld",rx->ctun_max);
  setProperty(name,value);
  
  sprintf(name,"receiver[%d].split",rx->channel);
  sprintf(value,"%d",rx->split);
  setProperty(name,value);

  sprintf(name,"receiver[%d].offset",rx->channel);
  sprintf(value,"%ld",rx->offset);
  setProperty(name,value);
  sprintf(name,"receiver[%d].bandstack",rx->channel);
  sprintf(value,"%d",rx->bandstack);
  setProperty(name,value);

  sprintf(name,"receiver[%d].remote_audio",rx->channel);
  sprintf(value,"%d",rx->remote_audio);
  setProperty(name,value);

  sprintf(name,"receiver[%d].local_audio",rx->channel);
  sprintf(value,"%d",rx->local_audio);
  setProperty(name,value);
  if(rx->audio_name!=NULL) {
    sprintf(name,"receiver[%d].audio_name",rx->channel);
    setProperty(name,rx->audio_name);
  }
  sprintf(name,"receiver[%d].mute_when_not_active",rx->channel);
  sprintf(value,"%d",rx->mute_when_not_active);
  setProperty(name,value);
  sprintf(name,"receiver[%d].local_audio_buffer_size",rx->channel);
  sprintf(value,"%d",rx->local_audio_buffer_size);
  setProperty(name,value);
  sprintf(name,"receiver[%d].local_audio_latency",rx->channel);
  sprintf(value,"%d",rx->local_audio_latency);
  setProperty(name,value);

  sprintf(name,"receiver[%d].step",rx->channel);
  sprintf(value,"%ld",rx->step);
  setProperty(name,value);
  sprintf(name,"receiver[%d].zoom",rx->channel);
  sprintf(value,"%d",rx->zoom);
  setProperty(name,value);

  sprintf(name,"receiver[%d].agc",rx->channel);
  sprintf(value,"%d",rx->agc);
  setProperty(name,value);
  sprintf(name,"receiver[%d].agc_gain",rx->channel);
  sprintf(value,"%f",rx->agc_gain);
  setProperty(name,value);
  sprintf(name,"receiver[%d].agc_slope",rx->channel);
  sprintf(value,"%f",rx->agc_slope);
  setProperty(name,value);
  sprintf(name,"receiver[%d].agc_hang_threshold",rx->channel);
  sprintf(value,"%f",rx->agc_hang_threshold);
  setProperty(name,value);
  
  sprintf(name,"receiver[%d].enable_equalizer",rx->channel);
  sprintf(value,"%d",rx->enable_equalizer);
  setProperty(name,value);
  for(i=0;i<4;i++) {
    sprintf(name,"receiver[%d].equalizer[%d]",rx->channel,i);
    sprintf(value,"%d",rx->equalizer[i]);
    setProperty(name,value);
  }

  sprintf(name,"receiver[%d].volume",rx->channel);
  sprintf(value,"%f",rx->volume);
  setProperty(name,value);

  sprintf(name,"receiver[%d].nr",rx->channel);
  sprintf(value,"%d",rx->nr);
  setProperty(name,value);
  sprintf(name,"receiver[%d].nr2",rx->channel);
  sprintf(value,"%d",rx->nr2);
  setProperty(name,value);
  sprintf(name,"receiver[%d].nb",rx->channel);
  sprintf(value,"%d",rx->nb);
  setProperty(name,value);
  sprintf(name,"receiver[%d].nb2",rx->channel);
  sprintf(value,"%d",rx->nb2);
  setProperty(name,value);
  sprintf(name,"receiver[%d].anf",rx->channel);
  sprintf(value,"%d",rx->anf);
  setProperty(name,value);
  sprintf(name,"receiver[%d].snb",rx->channel);
  sprintf(value,"%d",rx->snb);
  setProperty(name,value);

  sprintf(name,"receiver[%d].rit_enabled",rx->channel);
  sprintf(value,"%d",rx->rit_enabled);
  setProperty(name,value);
  sprintf(name,"receiver[%d].rit",rx->channel);
  sprintf(value,"%ld",rx->rit);
  setProperty(name,value);
  sprintf(name,"receiver[%d].rit_step",rx->channel);
  sprintf(value,"%ld",rx->rit_step);
  setProperty(name,value);

  gtk_window_get_position(GTK_WINDOW(rx->window),&x,&y);
  sprintf(name,"receiver[%d].x",rx->channel);
  sprintf(value,"%d",x);
  setProperty(name,value);
  sprintf(name,"receiver[%d].y",rx->channel);
  sprintf(value,"%d",y);
  setProperty(name,value);

  gtk_window_get_size(GTK_WINDOW(rx->window),&width,&height);
  sprintf(name,"receiver[%d].width",rx->channel);
  sprintf(value,"%d",width);
  setProperty(name,value);
  sprintf(name,"receiver[%d].height",rx->channel);
  sprintf(value,"%d",height);
  setProperty(name,value);

  rx->paned_position=gtk_paned_get_position(GTK_PANED(rx->vpaned));
  sprintf(name,"receiver[%d].paned_position",rx->channel);
  sprintf(value,"%d",rx->paned_position);
  setProperty(name,value);

  gint paned_height=gtk_widget_get_allocated_height(rx->vpaned);
  double paned_percent=(double)rx->paned_position/(double)paned_height;

  sprintf(name,"receiver[%d].paned_percent",rx->channel);
  sprintf(value,"%f",paned_percent);
  setProperty(name,value);
  
fprintf(stderr,"receiver_save_sate: paned_position=%d paned_height=%d paned_percent=%f\n",rx->paned_position, paned_height, paned_percent);
}

void receiver_restore_state(RECEIVER *rx) {
  char name[80];
  char *value;
  int i;

  sprintf(name,"receiver[%d].adc",rx->channel);
  value=getProperty(name);
  if(value) rx->adc=atol(value);

  sprintf(name,"receiver[%d].sample_rate",rx->channel);
  value=getProperty(name);
  if(value) rx->sample_rate=atol(value);
  sprintf(name,"receiver[%d].dsp_rate",rx->channel);
  value=getProperty(name);
  if(value) rx->dsp_rate=atol(value);
  sprintf(name,"receiver[%d].output_rate",rx->channel);
  value=getProperty(name);
  if(value) rx->output_rate=atol(value);

  sprintf(name,"receiver[%d].frequency_a",rx->channel);
  value=getProperty(name);
  if(value) rx->frequency_a=atol(value);
  sprintf(name,"receiver[%d].lo_a",rx->channel);
  value=getProperty(name);
  if(value) rx->lo_a=atol(value);
  sprintf(name,"receiver[%d].lo_tx",rx->channel);
  value=getProperty(name);
  if(value) rx->lo_tx=atol(value);
  sprintf(name,"receiver[%d].tx_track_rx",rx->channel);
  value=getProperty(name);
  if(value) rx->tx_track_rx=atoi(value);
  sprintf(name,"receiver[%d].error_a",rx->channel);
  value=getProperty(name);
  if(value) rx->error_a=atol(value);
  sprintf(name,"receiver[%d].band_a",rx->channel);
  value=getProperty(name);
  if(value) rx->band_a=atoi(value);
  sprintf(name,"receiver[%d].mode_a",rx->channel);
  value=getProperty(name);
  if(value) rx->mode_a=atoi(value);
  sprintf(name,"receiver[%d].filter_a",rx->channel);
  value=getProperty(name);
  if(value) rx->filter_a=atoi(value);
  sprintf(name,"receiver[%d].frequency_b",rx->channel);
  value=getProperty(name);
  if(value) rx->frequency_b=atol(value);
  sprintf(name,"receiver[%d].lo_b",rx->channel);
  value=getProperty(name);
  if(value) rx->lo_b=atol(value);
  sprintf(name,"receiver[%d].error_b",rx->channel);
  value=getProperty(name);
  if(value) rx->error_b=atol(value);
  sprintf(name,"receiver[%d].band_b",rx->channel);
  value=getProperty(name);
  if(value) rx->band_b=atoi(value);
  sprintf(name,"receiver[%d].mode_b",rx->channel);
  value=getProperty(name);
  if(value) rx->mode_b=atoi(value);
  sprintf(name,"receiver[%d].filter_b",rx->channel);
  value=getProperty(name);
  if(value) rx->filter_b=atoi(value);

  sprintf(name,"receiver[%d].ctun",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun=atoi(value);
  sprintf(name,"receiver[%d].ctun_offset",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun_offset=atol(value);
  sprintf(name,"receiver[%d].ctun_min",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun_min=atol(value);
  sprintf(name,"receiver[%d].ctun_max",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun_max=atol(value);

  sprintf(name,"receiver[%d].split",rx->channel);
  value=getProperty(name);
  if(value) rx->split=atoi(value);

  sprintf(name,"receiver[%d].remote_audio",rx->channel);
  value=getProperty(name);
  if(value) rx->remote_audio=atoi(value);

  sprintf(name,"receiver[%d].local_audio",rx->channel);
  value=getProperty(name);
  if(value) rx->local_audio=atoi(value);
  sprintf(name,"receiver[%d].audio_name",rx->channel);
  value=getProperty(name);
  if(value) {
    rx->audio_name=g_new0(gchar,strlen(value)+1);
    strcpy(rx->audio_name,value);
  }
  sprintf(name,"receiver[%d].mute_when_not_active",rx->channel);
  value=getProperty(name);
  if(value) rx->mute_when_not_active=atoi(value);
  sprintf(name,"receiver[%d].local_audio_buffer_size",rx->channel);
  value=getProperty(name);
  if(value) rx->local_audio_buffer_size=atoi(value);
  sprintf(name,"receiver[%d].local_audio_latency",rx->channel);
  value=getProperty(name);
  if(value) rx->local_audio_latency=atoi(value);

  sprintf(name,"receiver[%d].step",rx->channel);
  value=getProperty(name);
  if(value) rx->step=atol(value);
  sprintf(name,"receiver[%d].zoom",rx->channel);
  value=getProperty(name);
  if(value) rx->zoom=atoi(value);

  sprintf(name,"receiver[%d].volume",rx->channel);
  value=getProperty(name);
  if(value) rx->volume=atof(value);

  sprintf(name,"receiver[%d].nr",rx->channel);
  value=getProperty(name);
  if(value) rx->nr=atoi(value);
  sprintf(name,"receiver[%d].nr2",rx->channel);
  value=getProperty(name);
  if(value) rx->nr2=atoi(value);
  sprintf(name,"receiver[%d].nb",rx->channel);
  value=getProperty(name);
  if(value) rx->nb=atoi(value);
  sprintf(name,"receiver[%d].nb2",rx->channel);
  value=getProperty(name);
  if(value) rx->nb2=atoi(value);
  sprintf(name,"receiver[%d].anf",rx->channel);
  value=getProperty(name);
  if(value) rx->anf=atoi(value);
  sprintf(name,"receiver[%d].snb",rx->channel);
  value=getProperty(name);
  if(value) rx->snb=atoi(value);

  sprintf(name,"receiver[%d].agc",rx->channel);
  value=getProperty(name);
  if(value) rx->agc=atoi(value);
  sprintf(name,"receiver[%d].agc_gain",rx->channel);
  value=getProperty(name);
  if(value) rx->agc_gain=atof(value);
  sprintf(name,"receiver[%d].agc_slope",rx->channel);
  value=getProperty(name);
  if(value) rx->agc_slope=atof(value);
  sprintf(name,"receiver[%d].agc_hang_threshold",rx->channel);
  value=getProperty(name);
  if(value) rx->agc_hang_threshold=atof(value);
  
  sprintf(name,"receiver[%d].enable_equalizer",rx->channel);
  value=getProperty(name);
  if(value) rx->enable_equalizer=atoi(value);
  for(i=0;i<4;i++) {
    sprintf(name,"receiver[%d].equalizer[%d]",rx->channel,i);
    value=getProperty(name);
    if(value) rx->equalizer[i]=atoi(value);
  }

  sprintf(name,"receiver[%d].rit_enabled",rx->channel);
  value=getProperty(name);
  if(value) rx->rit_enabled=atoi(value);
  sprintf(name,"receiver[%d].rit",rx->channel);
  value=getProperty(name);
  if(value) rx->rit=atol(value);
  sprintf(name,"receiver[%d].rit_step",rx->channel);
  value=getProperty(name);
  if(value) rx->rit_step=atol(value);
 
  sprintf(name,"receiver[%d].fps",rx->channel);
  value=getProperty(name);
  if(value) rx->fps=atoi(value);
  
  sprintf(name,"receiver[%d].display_average_time",rx->channel);
  value=getProperty(name);
  if(value) rx->display_average_time=atof(value);
  
  sprintf(name,"receiver[%d].panadapter_low",rx->channel);
  value=getProperty(name);
  if(value) rx->panadapter_low=atoi(value);
  sprintf(name,"receiver[%d].panadapter_high",rx->channel);
  value=getProperty(name);
  if(value) rx->panadapter_high=atoi(value);
  sprintf(name,"receiver[%d].panadapter_filled",rx->channel);
  value=getProperty(name);
  if(value) rx->panadapter_filled=atoi(value);
  sprintf(name,"receiver[%d].panadapter_gradient",rx->channel);
  value=getProperty(name);
  if(value) rx->panadapter_gradient=atoi(value);
  sprintf(name,"receiver[%d].panadapter_agc_line",rx->channel);
  value=getProperty(name);
  if(value) rx->panadapter_agc_line=atoi(value);

  sprintf(name,"receiver[%d].waterfall_low",rx->channel);
  value=getProperty(name);
  if(value) rx->waterfall_low=atoi(value);
  sprintf(name,"receiver[%d].waterfall_high",rx->channel);
  value=getProperty(name);
  if(value) rx->waterfall_high=atoi(value);
  sprintf(name,"receiver[%d].waterfall_automatic",rx->channel);
  value=getProperty(name);
  if(value) rx->waterfall_automatic=atoi(value);
  sprintf(name,"receiver[%d].waterfall_ft8_marker",rx->channel);
  value=getProperty(name);
  if(value) rx->waterfall_ft8_marker=atoi(value);

  sprintf(name,"receiver[%d].paned_position",rx->channel);
  value=getProperty(name);
  if(value) rx->paned_position=atoi(value);

  sprintf(name,"receiver[%d].paned_percent",rx->channel);
  value=getProperty(name);
  if(value) rx->paned_percent=atof(value);
}

void receiver_xvtr_changed(RECEIVER *rx) {
}

void receiver_change_sample_rate(RECEIVER *rx,int sample_rate) {
  SetChannelState(rx->channel,0,1);
  g_free(rx->audio_output_buffer);
  rx->audio_output_buffer=NULL;
  rx->sample_rate=sample_rate;
  rx->output_samples=rx->buffer_size/(rx->sample_rate/48000);
  rx->audio_output_buffer=g_new0(gdouble,2*rx->output_samples);
  rx->hz_per_pixel=(double)rx->sample_rate/(double)rx->samples;
  //SetInputSamplerate(rx->channel, sample_rate);
  SetAllRates(rx->channel,rx->sample_rate,48000,48000);

  receiver_init_analyzer(rx);
  SetEXTANBSamplerate (rx->channel, sample_rate);
  SetEXTNOBSamplerate (rx->channel, sample_rate);
fprintf(stderr,"receiver_change_sample_rate: channel=%d rate=%d buffer_size=%d output_samples=%d\n",rx->channel, rx->sample_rate, rx->buffer_size, rx->output_samples);
  SetChannelState(rx->channel,1,0);
  receiver_update_title(rx);
}

void update_noise(RECEIVER *rx) {
  SetEXTANBRun(rx->channel, rx->nb);
  SetEXTNOBRun(rx->channel, rx->nb2);
  SetRXAANRRun(rx->channel, rx->nr);
  SetRXAEMNRRun(rx->channel, rx->nr2);
  SetRXAANFRun(rx->channel, rx->anf);
  SetRXASNBARun(rx->channel, rx->snb);
  update_vfo(rx);
}

static gboolean window_delete(GtkWidget *widget,GdkEvent *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  g_source_remove(rx->update_timer_id);
  if(radio->dialog!=NULL) {
    gtk_widget_destroy(radio->dialog);
    radio->dialog=NULL;
  }
  if(rx->bookmark_dialog!=NULL) {
    gtk_widget_destroy(rx->bookmark_dialog);
    rx->bookmark_dialog=NULL;
  }
  delete_receiver(rx);
  return FALSE;
}

static void focus_in_event_cb(GtkWindow *window,GdkEventFocus *event,gpointer data) {
  if(event->in) {
    radio->active_receiver=(RECEIVER *)data;
  }
}

gboolean receiver_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  int x=(int)event->x;
  switch(event->button) {
    case 1: // left button
      if(!rx->locked) {
        rx->last_x=(int)event->x;
        rx->has_moved=FALSE;
      }
      break;
    case 3: // right button
      if(radio->dialog==NULL) {
        int i;
        for(i=0;i<radio->discovered->supported_receivers;i++) {
          if(rx==radio->receiver[i]) {
            break;
          }
        }
        radio->dialog=create_configure_dialog(radio,i+rx_base);
      }
      break;
  }
  return TRUE;
}

void update_frequency(RECEIVER *rx) {
  update_vfo(rx);
  if(radio->transmitter!=NULL) {
    if(radio->transmitter->rx==rx) {
      update_tx_panadapter(radio);
    }
  }
}

gboolean receiver_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  gint64 hz;
  RECEIVER *rx=(RECEIVER *)data;
  int x=(int)event->x;
  switch(event->button) {
    case 1: // left button
      if(!rx->locked) {
        if(rx->has_moved) {
          // drag
          hz=(gint64)((double)(x-rx->last_x)*rx->hz_per_pixel);
          if(rx->ctun) {
            rx->ctun_offset=(rx->ctun_offset-hz)/rx->step*rx->step;
          } else {
            rx->frequency_a=(rx->frequency_a+hz)/rx->step*rx->step;
          }
        } else {
          // move to this frequency
          hz=(gint64)((double)(x-(rx->panadapter_width/2))*rx->hz_per_pixel);
          if(rx->ctun) {
            rx->ctun_offset=hz/rx->step*rx->step;
          } else {
            rx->frequency_a=(rx->frequency_a+hz)/rx->step*rx->step;
            if(rx->mode_a==CWL) {
              rx->frequency_a+=radio->cw_keyer_sidetone_frequency;
            } else if(rx->mode_a==CWU) {
              rx->frequency_a-=radio->cw_keyer_sidetone_frequency;
            }
          }
        }
        if(rx->ctun) {
          if(rx->ctun_offset < rx->ctun_min) rx->ctun_offset=rx->ctun_min;
          if(rx->ctun_offset > rx->ctun_max) rx->ctun_offset=rx->ctun_max;
        }
        rx->last_x=x;
        frequency_changed(rx);
        update_frequency(rx);
      }
      break;
    case 3: // right button
      break;
  }
  return TRUE;
}

gboolean receiver_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  gint x,y;
  GdkModifierType state;
  RECEIVER *rx=(RECEIVER *)data;
  if(!rx->locked) {
    gdk_window_get_device_position(event->window,event->device,&x,&y,&state);
    if((state & GDK_BUTTON1_MASK) == GDK_BUTTON1_MASK) {
      int moved=rx->last_x-x;
      gint64 hz=(gint64)((double)moved*rx->hz_per_pixel);
      if(rx->ctun) {
        rx->ctun_offset=(rx->ctun_offset-hz)/rx->step*rx->step;
      } else {
        rx->frequency_a=(rx->frequency_a+hz)/rx->step*rx->step;
      }
      if(rx->ctun) {
        if(rx->ctun_offset < rx->ctun_min) rx->ctun_offset=rx->ctun_min;
        if(rx->ctun_offset > rx->ctun_max) rx->ctun_offset=rx->ctun_max;
      }
      rx->last_x=x;
      rx->has_moved=TRUE;
      frequency_changed(rx);
      update_frequency(rx);
    } else {
    }
  }
  return TRUE;
}

gboolean receiver_scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(!rx->locked) {
    if(event->direction==GDK_SCROLL_UP) {
      if(rx->ctun) {
        rx->ctun_offset+=rx->step;
      } else {
        rx->frequency_a=rx->frequency_a+rx->step;
      }
    } else {
      if(rx->ctun) {
        rx->ctun_offset-=rx->step;
      } else {
        rx->frequency_a=rx->frequency_a-rx->step;
      }
    }
    if(rx->ctun) {
      if(rx->ctun_offset < rx->ctun_min) rx->ctun_offset=rx->ctun_min;
      if(rx->ctun_offset > rx->ctun_max) rx->ctun_offset=rx->ctun_max;
    }
    frequency_changed(rx);
    update_frequency(rx);
  }
  return TRUE;
}

static gboolean update_timer_cb(void *data) {
  int rc;
  RECEIVER *rx=(RECEIVER *)data;

  g_mutex_lock(&rx->mutex);
  if(!isTransmitting(radio)) {
    if(rx->panadapter_resize_timer==-1) {
      GetPixels(rx->channel,0,rx->pixel_samples,&rc);
      if(rc) {
        update_rx_panadapter(rx);
        update_waterfall(rx);
      }
    }
    double m=GetRXAMeter(rx->channel,rx->smeter) + radio->meter_calibration;
    update_meter(rx,m);
  }
  g_mutex_unlock(&rx->mutex);
  return TRUE;
}
 
static void set_mode(RECEIVER *rx,int m) {
//fprintf(stderr,"set_mode: %d\n",m);
  int previous_mode;
  previous_mode=rx->mode_a;
  rx->mode_a=m;
  SetRXAMode(rx->channel, m);
  if(rx->mode_a==FMN && previous_mode!=FMN) {
    SetDSPSamplerate (rx->channel, 19200);
  } else if(rx->mode_a!=FMN && previous_mode==FMN) {
    SetDSPSamplerate (rx->channel, 48000);
  }
}

void set_filter(RECEIVER *rx,int low,int high) {
//fprintf(stderr,"set_filter: %d %d\n",low,high);
  if(rx->mode_a==CWL) {
    rx->filter_low=-radio->cw_keyer_sidetone_frequency-low;
    rx->filter_high=-radio->cw_keyer_sidetone_frequency+high;
  } else if(rx->mode_a==CWU) {
    rx->filter_low=radio->cw_keyer_sidetone_frequency-low;
    rx->filter_high=radio->cw_keyer_sidetone_frequency+high;
  } else {
    rx->filter_low=low;
    rx->filter_high=high;
  }
  RXASetPassband(rx->channel,(double)rx->filter_low,(double)rx->filter_high);
  if(radio->transmitter!=NULL && radio->transmitter->rx==rx && radio->transmitter->use_rx_filter) {
    transmitter_set_filter(radio->transmitter,rx->filter_low,rx->filter_high);
  }
}

void set_deviation(RECEIVER *rx) {
fprintf(stderr,"set_deviation: %d\n",rx->deviation);
  SetRXAFMDeviation(rx->channel, (double)rx->deviation);
}

void calculate_display_average(RECEIVER *rx) {
  double display_avb;
  int display_average;

  double t=0.001 * rx->display_average_time;
  
  display_avb = exp(-1.0 / ((double)rx->fps * t));
  display_average = max(2, (int)min(60, (double)rx->fps * t));
  SetDisplayAvBackmult(rx->channel, 0, display_avb);
  SetDisplayNumAverage(rx->channel, 0, display_average);
}

void receiver_fps_changed(RECEIVER *rx) {
  g_source_remove(rx->update_timer_id);
  rx->update_timer_id=g_timeout_add(1000/rx->fps,update_timer_cb,(gpointer)rx);
  calculate_display_average(rx);
}

void receiver_filter_changed(RECEIVER *rx,int filter) {
  rx->filter_a=filter;
  if(rx->mode_a==FMN) {
    switch(filter) {
      case 0:
        rx->deviation=2500;
        set_filter(rx,-4000,4000);
        break;
      case 1:
        rx->deviation=5000;
        set_filter(rx,-8000,8000);
        break;
    }
    set_deviation(rx);
  } else {
    FILTER *mode_filters=filters[rx->mode_a];
    FILTER *f=&mode_filters[rx->filter_a];
    set_filter(rx,f->low,f->high);
  }
  update_vfo(rx);
}

void receiver_mode_changed(RECEIVER *rx,int mode) {
//fprintf(stderr,"receiver_mode_changed: %d\n",mode);
  set_mode(rx,mode);
  receiver_filter_changed(rx,rx->filter_a);
}

void receiver_band_changed(RECEIVER *rx,int band) {
#ifdef OLD_BAND
  BANDSTACK *bandstack=bandstack_get_bandstack(band);
  if(rx->band_a==band) {
    // same band selected - step to the next band stack
    rx->bandstack++;
    if(rx->bandstack>=bandstack->entries) {
      rx->bandstack=0;
    }
  } else {
    // new band - get band stack entry
    rx->band_a=band;
    rx->bandstack=bandstack->current_entry;
  }

  BANDSTACK_ENTRY *entry=&bandstack->entry[rx->bandstack];
  rx->frequency_a=entry->frequency;
  receiver_mode_changed(rx,entry->mode);
  receiver_filter_changed(rx,entry->filter);
#else
  rx->band_a=band;
  receiver_mode_changed(rx,rx->mode_a);
  receiver_filter_changed(rx,rx->filter_a);
#endif
  frequency_changed(rx);
}

static void process_rx_buffer(RECEIVER *rx) {
  short left_audio_sample;
  short right_audio_sample;
  int i;
  for(i=0;i<rx->output_samples;i++) {
    if(isTransmitting(radio)) {
      left_audio_sample=0;
      right_audio_sample=0;
    } else {
      left_audio_sample=(short)(rx->audio_output_buffer[i*2]*32767.0);
      right_audio_sample=(short)(rx->audio_output_buffer[(i*2)+1]*32767.0);
    }
    if(rx->local_audio) {
      //audio_write(rx,left_audio_sample,right_audio_sample);
      audio_write(rx,(float)rx->audio_output_buffer[i*2],(float)rx->audio_output_buffer[(i*2)+1]);
    } 
    if(radio->active_receiver==rx) {
      switch(radio->discovered->protocol) {
        case PROTOCOL_1:
          if(rx->remote_audio) {
            protocol1_audio_samples(rx,left_audio_sample,right_audio_sample);
          } else {
            protocol1_audio_samples(rx,0,0);
          }
          break;
        case PROTOCOL_2:
          if(rx->remote_audio) {
            protocol2_audio_samples(rx,left_audio_sample,right_audio_sample);
          } else {
            protocol2_audio_samples(rx,0,0);
          }
          break;
      }
    }
  }
}

static void full_rx_buffer(RECEIVER *rx) {
int j;
  int error;

  if(isTransmitting(radio)) return;

  // noise blanker works on origianl IQ samples
  if(rx->nb) {
     xanbEXT (rx->channel, rx->iq_input_buffer, rx->iq_input_buffer);
  }
  if(rx->nb2) {
     xnobEXT (rx->channel, rx->iq_input_buffer, rx->iq_input_buffer);
  }

  fexchange0(rx->channel, rx->iq_input_buffer, rx->audio_output_buffer, &error);
  if(error!=0) {
  //  fprintf(stderr,"full_rx_buffer: channel=%d fexchange0: error=%d\n",rx->channel,error);
  }

  Spectrum0(1, rx->channel, 0, 0, rx->iq_input_buffer);

#ifdef FREEDV
  g_mutex_lock(&rx->freedv_mutex);
  if(rx->freedv) {
    process_freedv_rx_buffer(rx);
  } else {
#endif
    process_rx_buffer(rx);
#ifdef FREEDV
  }
  g_mutex_unlock(&rx->freedv_mutex);
#endif
}

void add_iq_samples(RECEIVER *rx,double i_sample,double q_sample) {
  rx->iq_input_buffer[rx->samples*2]=i_sample;
  rx->iq_input_buffer[(rx->samples*2)+1]=q_sample;
  rx->samples=rx->samples+1;
  if(rx->samples>=rx->buffer_size) {
    full_rx_buffer(rx);
    rx->samples=0;
  }
}

static gboolean receiver_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  char name[80];
  char *value;

  RECEIVER *rx=(RECEIVER *)data;
  gint width=gtk_widget_get_allocated_width(widget);
  gint height=gtk_widget_get_allocated_height(widget);
  rx->window_width=width;
  rx->window_height=height;
  return FALSE;
}

void set_agc(RECEIVER *rx) {

  SetRXAAGCMode(rx->channel, rx->agc);
  SetRXAAGCSlope(rx->channel,rx->agc_slope);
  SetRXAAGCTop(rx->channel,rx->agc_gain);
  switch(rx->agc) {
    case AGC_OFF:
      break;
    case AGC_LONG:
      SetRXAAGCAttack(rx->channel,2);
      SetRXAAGCHang(rx->channel,2000);
      SetRXAAGCDecay(rx->channel,2000);
      SetRXAAGCHangThreshold(rx->channel,(int)rx->agc_hang_threshold);
      break;
    case AGC_SLOW:
      SetRXAAGCAttack(rx->channel,2);
      SetRXAAGCHang(rx->channel,1000);
      SetRXAAGCDecay(rx->channel,500);
      SetRXAAGCHangThreshold(rx->channel,(int)rx->agc_hang_threshold);
      break;
    case AGC_MEDIUM:
      SetRXAAGCAttack(rx->channel,2);
      SetRXAAGCHang(rx->channel,0);
      SetRXAAGCDecay(rx->channel,250);
      SetRXAAGCHangThreshold(rx->channel,100);
      break;
    case AGC_FAST:
      SetRXAAGCAttack(rx->channel,2);
      SetRXAAGCHang(rx->channel,0);
      SetRXAAGCDecay(rx->channel,50);
      SetRXAAGCHangThreshold(rx->channel,100);
      break;
  }
}

void receiver_update_title(RECEIVER *rx) {
  gchar title[32];
  g_snprintf((gchar *)&title,sizeof(title),"Linux HPSDR: Rx-%d ADC-%d %d",rx->channel,rx->adc,rx->sample_rate);
g_print("receiver_update_title: %s\n",title);
  gtk_window_set_title(GTK_WINDOW(rx->window),title);
}

static gboolean enter (GtkWidget *ebox, GdkEventCrossing *event) {
   gdk_window_set_cursor(gtk_widget_get_window(ebox),gdk_cursor_new(GDK_TCROSS));
   return FALSE;
}


static gboolean leave (GtkWidget *ebox, GdkEventCrossing *event) {
   gdk_window_set_cursor(gtk_widget_get_window(ebox),gdk_cursor_new(GDK_ARROW));
   return FALSE;
}

static void create_visual(RECEIVER *rx) {

  rx->dialog=NULL;

  rx->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  if(rx->channel==0) {
    gtk_window_set_deletable (GTK_WINDOW(rx->window),FALSE);
  }
  g_signal_connect(rx->window,"configure-event",G_CALLBACK(receiver_configure_event_cb),rx);
  g_signal_connect(rx->window,"focus_in_event",G_CALLBACK(focus_in_event_cb),rx);
  g_signal_connect(rx->window,"delete-event",G_CALLBACK (window_delete), rx);

  receiver_update_title(rx);

  gtk_widget_set_size_request(rx->window,rx->window_width,rx->window_height);

  rx->table=gtk_table_new(4,6,FALSE);

  rx->vfo=create_vfo(rx);
  gtk_widget_set_size_request(rx->vfo,715,60);
  gtk_table_attach(GTK_TABLE(rx->table), rx->vfo, 0, 4, 0, 1,
      GTK_FILL, GTK_FILL, 0, 0);

  rx->meter=create_meter_visual(rx);
  gtk_widget_set_size_request(rx->meter,300,60);        // resize from 154 to 300 for custom s-meter

  gtk_table_attach(GTK_TABLE(rx->table), rx->meter, 4, 6, 0, 1,
      GTK_FILL, GTK_FILL, 0, 0);


  rx->vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_table_attach(GTK_TABLE(rx->table), rx->vpaned, 0, 6, 1, 3,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

  rx->panadapter=create_rx_panadapter(rx);
  //gtk_table_attach(GTK_TABLE(rx->table), rx->panadapter, 0, 4, 1, 2,
  //    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_paned_pack1 (GTK_PANED(rx->vpaned), rx->panadapter,TRUE,TRUE);
  gtk_widget_add_events (rx->panadapter,GDK_ENTER_NOTIFY_MASK);
  gtk_widget_add_events (rx->panadapter,GDK_LEAVE_NOTIFY_MASK);
  g_signal_connect (rx->panadapter, "enter-notify-event", G_CALLBACK (enter), rx->window);
  g_signal_connect (rx->panadapter, "leave-notify-event", G_CALLBACK (leave), rx->window);

  rx->waterfall=create_waterfall(rx);
  //gtk_table_attach(GTK_TABLE(rx->table), rx->waterfall, 0, 4, 2, 3,
  //    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_paned_pack2 (GTK_PANED(rx->vpaned), rx->waterfall,TRUE,TRUE);
  gtk_widget_add_events (rx->waterfall,GDK_ENTER_NOTIFY_MASK);
  gtk_widget_add_events (rx->waterfall,GDK_LEAVE_NOTIFY_MASK);
  g_signal_connect (rx->waterfall, "enter-notify-event", G_CALLBACK (enter), rx->window);
  g_signal_connect (rx->waterfall, "leave-notify-event", G_CALLBACK (leave), rx->window);

  gtk_container_add(GTK_CONTAINER(rx->window),rx->table);

  gtk_widget_set_events(rx->window,gtk_widget_get_events(rx->window)
                     | GDK_FOCUS_CHANGE_MASK
                     | GDK_BUTTON_PRESS_MASK
                     | GDK_BUTTON_RELEASE_MASK);
}

void receiver_init_analyzer(RECEIVER *rx) {
    int flp[] = {0};
    double keep_time = 0.1;
    int n_pixout=1;
    int spur_elimination_ffts = 1;
    int data_type = 1;
    int fft_size = 8192;
    int window_type = 4;
    double kaiser_pi = 14.0;
    int overlap = 4096;
    int clip = 0;
    int span_clip_l = 0;
    int span_clip_h = 0;
    int pixels=rx->pixels;
    int stitches = 1;
    int avm = 0;
    double tau = 0.001 * 120.0;
    int calibration_data_set = 0;
    double span_min_freq = 0.0;
    double span_max_freq = 0.0;

g_print("receiver_init_analyzer: channel=%d zoom=%d pixels=%d pixel_samples=%p\n",rx->channel,rx->zoom,rx->pixels,rx->pixel_samples);

  if(rx->pixel_samples!=NULL) {
//g_print("receiver_init_analyzer: g_free: channel=%d pixel_samples=%p\n",rx->channel,rx->pixel_samples);
    g_free(rx->pixel_samples);
    rx->pixel_samples=NULL;
  }
  if(rx->pixels>0) {
    rx->pixel_samples=g_new0(float,rx->pixels);
g_print("receiver_init_analyzer: g_new0: channel=%d pixel_samples=%p\n",rx->channel,rx->pixel_samples);
    rx->hz_per_pixel=(gdouble)rx->sample_rate/(gdouble)rx->pixels;

    int max_w = fft_size + (int) min(keep_time * (double) rx->fps, keep_time * (double) fft_size * (double) rx->fps);

    overlap = (int)max(0.0, ceil(fft_size - (double)rx->sample_rate / (double)rx->fps));

g_print("SetAnalyzer id=%d buffer_size=%d fft_size=%d overlap=%d\n",rx->channel,rx->buffer_size,fft_size,overlap);


    SetAnalyzer(rx->channel,
            n_pixout,
            spur_elimination_ffts, //number of LO frequencies = number of ffts used in elimination
            data_type, //0 for real input data (I only); 1 for complex input data (I & Q)
            flp, //vector with one elt for each LO frequency, 1 if high-side LO, 0 otherwise
            fft_size, //size of the fft, i.e., number of input samples
            rx->buffer_size, //number of samples transferred for each OpenBuffer()/CloseBuffer()
            window_type, //integer specifying which window function to use
            kaiser_pi, //PiAlpha parameter for Kaiser window
            overlap, //number of samples each fft (other than the first) is to re-use from the previous
            clip, //number of fft output bins to be clipped from EACH side of each sub-span
            span_clip_l, //number of bins to clip from low end of entire span
            span_clip_h, //number of bins to clip from high end of entire span
            pixels, //number of pixel values to return.  may be either <= or > number of bins
            stitches, //number of sub-spans to concatenate to form a complete span
            calibration_data_set, //identifier of which set of calibration data to use
            span_min_freq, //frequency at first pixel value8192
            span_max_freq, //frequency at last pixel value
            max_w //max samples to hold in input ring buffers
    );
  }

}


RECEIVER *create_receiver(int channel,int sample_rate) {
  RECEIVER *rx=g_new0(RECEIVER,1);
  char name [80];
  char *value;
  gint x=-1;
  gint y=-1;
  gint width;
  gint height;
  gint paned;

g_print("create_receiver: channel=%d sample_rate=%d\n", channel, sample_rate);
  g_mutex_init(&rx->mutex);
  rx->channel=channel;
  rx->adc=0;

  rx->frequency_min=(gint64)radio->discovered->frequency_min;
  rx->frequency_max=(gint64)radio->discovered->frequency_max;
g_print("create_receiver: channel=%d frequency_min=%ld frequency_max=%ld\n", channel, rx->frequency_min, rx->frequency_max);

// DEBUG
  if(channel==0 ) {
    rx->adc=0;
  } else {
    switch(radio->discovered->protocol) {
      case PROTOCOL_1:
        switch(radio->discovered->device) {
          case DEVICE_ANGELIA:
          case DEVICE_ORION:
          case DEVICE_ORION2:
            rx->adc=1;
            break;
          default:
            rx->adc=0;
            break;
        }
        break;
      case PROTOCOL_2:
        switch(radio->discovered->device) {
          case NEW_DEVICE_ANGELIA:
          case NEW_DEVICE_ORION:
          case NEW_DEVICE_ORION2:
            rx->adc=1;
            break;
          default:
            rx->adc=0;
            break;

        }
        break;
#ifdef SOAPYSDR
      case PROTOCOL_SOAPYSDR:
        if(radio->discovered->supported_receivers>1) {
          rx->adc=2;
        } else {
          rx->adc=1;
        }
        break;
#endif
    }
  }
   
  rx->sample_rate=sample_rate;
  rx->dsp_rate=48000;
  rx->output_rate=48000;

  rx->frequency_a=14200000;
  rx->band_a=band20;
  rx->mode_a=USB;
  rx->bandstack=1;

  rx->deviation=2500;
  rx->filter_a=F5;
  rx->lo_a=0;
  rx->error_a=0;
  rx->offset=0;
  rx->lo_tx=0;
  rx->tx_track_rx=FALSE;

  rx->ctun=FALSE;
  rx->ctun_offset=0;
  rx->ctun_min=-rx->sample_rate/2;
  rx->ctun_max=rx->sample_rate/2;

  rx->frequency_b=14300000;
  rx->band_b=band20;
  rx->mode_b=USB;
  rx->filter_b=F5;
  rx->lo_b=0;
  rx->error_b=0;

  rx->split=FALSE;

  rx->rit_enabled=FALSE;
  rx->rit=0;
  rx->rit_step=10;

  rx->step=100;
  rx->locked=FALSE;

  rx->volume=0.05;
  rx->agc=AGC_OFF;
  rx->agc_gain=80.0;
  rx->agc_slope=35.0;
  rx->agc_hang_threshold=0.0;

  rx->smeter=RXA_S_AV;

  rx->pixels=0;
  rx->pixel_samples=NULL;
  rx->waterfall_pixbuf=NULL;
  rx->iq_sequence=0;
#ifdef SOAPYSDR
  if(radio->discovered->device==DEVICE_SOAPYSDR_USB) {
    rx->buffer_size=2048;
  } else {
#endif
    rx->buffer_size=1024;
#ifdef SOAPYSDR
  }
#endif
fprintf(stderr,"create_receiver: buffer_size=%d\n",rx->buffer_size);
  rx->iq_input_buffer=g_new0(gdouble,2*rx->buffer_size);

  //rx->audio_buffer_size=260;
  rx->audio_buffer_size=480;
  rx->audio_buffer=g_new0(gchar,rx->audio_buffer_size);
  rx->audio_sequence=0;

  rx->mixed_audio=0;

  rx->fps=10;
  rx->display_average_time=170.0;

#ifdef SOAPYSDR
  if(radio->discovered->device==DEVICE_SOAPYSDR_USB) {
    rx->fft_size=2048;
  } else {
#endif
    rx->fft_size=2048;
#ifdef SOAPYSDR
  }
#endif
fprintf(stderr,"create_receiver: fft_size=%d\n",rx->fft_size);
  rx->low_latency=FALSE;

  rx->nb=FALSE;
  rx->nb2=FALSE;
  rx->nr=FALSE;
  rx->nr2=FALSE;
  rx->anf=FALSE;
  rx->snb=FALSE;

  rx->nr_agc=0;
  rx->nr2_gain_method=2;
  rx->nr2_npe_method=0;
  rx->nr2_ae=1;

  // set default location and sizes
  rx->window_x=channel*30;
  rx->window_y=channel*30;
  rx->window_width=820;
  rx->window_height=180;
  rx->window=NULL;
  rx->panadapter_low=-140;
  rx->panadapter_high=-60;
  rx->panadapter_surface=NULL;
  
  rx->panadapter_filled=TRUE;
  rx->panadapter_gradient=TRUE;
  rx->panadapter_agc_line=TRUE;

  rx->waterfall_automatic=TRUE;
  rx->waterfall_ft8_marker=FALSE;

  rx->vfo_surface=NULL;
  rx->meter_surface=NULL;

#ifdef SOAPYSDR
  if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
    rx->remote_audio=FALSE;
  } else {
#endif
    rx->remote_audio=TRUE;        // on or off remote audio
#ifdef SOAPYSDR
  }
#endif
  rx->local_audio=FALSE;
  rx->local_audio_buffer_size=4096; //112;
  rx->local_audio_buffer_offset=0;
  rx->local_audio_buffer=NULL;
  rx->local_audio_latency=50;
  g_mutex_init(&rx->local_audio_mutex);

  rx->audio_name=NULL;
  rx->mute_when_not_active=FALSE;

  rx->zoom=1;
  rx->enable_equalizer=FALSE;
  rx->equalizer[0]=0;
  rx->equalizer[1]=0;
  rx->equalizer[2]=0;
  rx->equalizer[3]=0;

  rx->bookmark_dialog=NULL;

  rx->filter_frame=NULL;
  rx->filter_grid=NULL;

  g_mutex_init(&rx->rigctl_mutex);
  rx->rigctl_server_socket=-1;
  rx->rigctl_client_socket=-1;
  rx->rigctl_running=FALSE;
  rx->cat_control=-1;

  rx->vpaned=NULL;
  rx->paned_position=-1;
  rx->paned_percent=0.5;

  receiver_restore_state(rx);

  if(radio->discovered->protocol==PROTOCOL_1) {
    if(rx->sample_rate!=sample_rate) {
      rx->sample_rate=sample_rate;
    }
  }

  rx->output_samples=rx->buffer_size/(rx->sample_rate/48000);
  rx->audio_output_buffer=g_new0(gdouble,2*rx->output_samples);

g_print("create_receiver: OpenChannel: channel=%d buffer_size=%d sample_rate=%d fft_size=%d\n", rx->channel, rx->buffer_size, rx->sample_rate, rx->fft_size);

  OpenChannel(rx->channel,
              rx->buffer_size,
              rx->fft_size,
              rx->sample_rate,
              48000, // dsp rate
              48000, // output rate
              0, // receive
              1, // run
              0.010, 0.025, 0.0, 0.010, 0);

  create_anbEXT(rx->channel,1,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  create_nobEXT(rx->channel,1,0,rx->buffer_size,rx->sample_rate,0.0001,0.0001,0.0001,0.05,20);
  RXASetNC(rx->channel, rx->fft_size);
  RXASetMP(rx->channel, rx->low_latency);

  frequency_changed(rx);
  receiver_mode_changed(rx,rx->mode_a);

  SetRXAPanelGain1(rx->channel, rx->volume);
  SetRXAPanelRun(rx->channel, 1);

  if(rx->enable_equalizer) {
    SetRXAGrphEQ(rx->channel, rx->equalizer);
    SetRXAEQRun(rx->channel, 1);
  } else {
    SetRXAEQRun(rx->channel, 0);
  }

  set_agc(rx);

  SetEXTANBRun(rx->channel, rx->nb);
  SetEXTNOBRun(rx->channel, rx->nb2);

  SetRXAEMNRPosition(rx->channel, rx->nr_agc);
  SetRXAEMNRgainMethod(rx->channel, rx->nr2_gain_method);
  SetRXAEMNRnpeMethod(rx->channel, rx->nr2_npe_method);
  SetRXAEMNRRun(rx->channel, rx->nr2);
  SetRXAEMNRaeRun(rx->channel, rx->nr2_ae);

  SetRXAANRVals(rx->channel, 64, 16, 16e-4, 10e-7); // defaults
  SetRXAANRRun(rx->channel, rx->nr);
  SetRXAANFRun(rx->channel, rx->anf);
  SetRXASNBARun(rx->channel, rx->snb);

  int result;
  XCreateAnalyzer(rx->channel, &result, 262144, 1, 1, "");
  if(result != 0) {
    g_print("XCreateAnalyzer channel=%d failed: %d\n", rx->channel, result);
  } else {
    receiver_init_analyzer(rx);
  }

  SetDisplayDetectorMode(rx->channel, 0, DETECTOR_MODE_AVERAGE/*display_detector_mode*/);
  SetDisplayAverageMode(rx->channel, 0,  AVERAGE_MODE_LOG_RECURSIVE/*display_average_mode*/);
  calculate_display_average(rx); 

  create_visual(rx);
  if(rx->window!=NULL) {
    gtk_widget_show_all(rx->window);
    sprintf(name,"receiver[%d].x",rx->channel);
    value=getProperty(name);
    if(value) x=atoi(value);
    sprintf(name,"receiver[%d].y",rx->channel);
    value=getProperty(name);
    if(value) y=atoi(value);
    if(x!=-1 && y!=-1) {
      gtk_window_move(GTK_WINDOW(rx->window),x,y);
    
      sprintf(name,"receiver[%d].width",rx->channel);
      value=getProperty(name);
      if(value) width=atoi(value);
      sprintf(name,"receiver[%d].height",rx->channel);
      value=getProperty(name);
      if(value) height=atoi(value);
      gtk_window_resize(GTK_WINDOW(rx->window),width,height);

      if(rx->vpaned!=NULL && rx->paned_position!=-1) {
        gint paned_height=gtk_widget_get_allocated_height(rx->vpaned);
        gint position=(gint)((double)paned_height*rx->paned_percent);
        gtk_paned_set_position(GTK_PANED(rx->vpaned),position);
g_print("receiver_configure_event: gtk_paned_set_position: rx=%d position=%d height=%d percent=%f\n",rx->channel,position,paned_height,rx->paned_percent);
      }
    }

    update_frequency(rx);
  }

  rx->update_timer_id=g_timeout_add(1000/rx->fps,update_timer_cb,(gpointer)rx);

  if(rx->local_audio) {
    if(audio_open_output(rx)<0) {
      rx->local_audio=FALSE;
    }
  }

  if(rigctl_enable) {
    launch_rigctl(rx);
  }

  return rx;
}
