/* Copyright (C)
*
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
#include <termios.h>
#include <wdsp.h>

#include "agc.h"
#include "mode.h"
#include "filter.h"
#include "bandstack.h"
#include "band.h"
#include "discovered.h"
#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "vfo.h"
#include "meter.h"
#include "radio_info.h"
#include "rx_panadapter.h"
#include "tx_panadapter.h"
#include "waterfall.h"
#include "protocol1.h"
#include "protocol2.h"
#include "audio.h"
#include "receiver_dialog.h"
#include "configure_dialog.h"
#include "property.h"
#include "rigctl.h"
#include "subrx.h"

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
  sprintf(name,"receiver[%d].panadapter_step",rx->channel);
  sprintf(value,"%d",rx->panadapter_step);
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
  sprintf(value,"%" G_GINT64_FORMAT,rx->frequency_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].lo_a",rx->channel);
  sprintf(value,"%" G_GINT64_FORMAT,rx->lo_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].error_a",rx->channel);
  sprintf(value,"%" G_GINT64_FORMAT,rx->error_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].lo_tx",rx->channel);
  sprintf(value,"%" G_GINT64_FORMAT,rx->lo_tx);
  setProperty(name,value);
  sprintf(name,"receiver[%d].error_tx",rx->channel);
  sprintf(value,"%" G_GINT64_FORMAT,rx->error_tx);
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
  sprintf(name,"receiver[%d].filter_low_a",rx->channel);
  sprintf(value,"%d",rx->filter_low_a);
  setProperty(name,value);
  sprintf(name,"receiver[%d].filter_high_a",rx->channel);
  sprintf(value,"%d",rx->filter_high_a);
  setProperty(name,value);


  sprintf(name,"receiver[%d].frequency_b",rx->channel);
  sprintf(value,"%" G_GINT64_FORMAT,rx->frequency_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].lo_b",rx->channel);
  sprintf(value,"%" G_GINT64_FORMAT,rx->lo_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].error_b",rx->channel);
  sprintf(value,"%" G_GINT64_FORMAT,rx->error_b);
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
  sprintf(name,"receiver[%d].filter_low_b",rx->channel);
  sprintf(value,"%d",rx->filter_low_b);
  setProperty(name,value);
  sprintf(name,"receiver[%d].filter_high_b",rx->channel);
  sprintf(value,"%d",rx->filter_high_b);
  setProperty(name,value);

  sprintf(name,"receiver[%d].ctun",rx->channel);
  sprintf(value,"%d",rx->ctun);
  setProperty(name,value);
  sprintf(name,"receiver[%d].ctun_offset",rx->channel);
  sprintf(value,"%ld",rx->ctun_offset);
  setProperty(name,value);
  sprintf(name,"receiver[%d].ctun_frequency",rx->channel);
  sprintf(value,"%ld",rx->ctun_frequency);
  setProperty(name,value);
  sprintf(name,"receiver[%d].ctun_min",rx->channel);
  sprintf(value,"%ld",rx->ctun_min);
  setProperty(name,value);
  sprintf(name,"receiver[%d].ctun_max",rx->channel);
  sprintf(value,"%ld",rx->ctun_max);
  setProperty(name,value);

  sprintf(name,"receiver[%d].qo100_beacon",rx->channel);
  sprintf(value,"%d",rx->qo100_beacon);
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
  sprintf(name,"receiver[%d].output_index",rx->channel);
  sprintf(value,"%d",rx->output_index);
  setProperty(name,value);
  sprintf(name,"receiver[%d].mute_when_not_active",rx->channel);
  sprintf(value,"%d",rx->mute_when_not_active);
  setProperty(name,value);
  sprintf(name,"receiver[%d].local_audio_buffer_size",rx->channel);
  sprintf(value,"%d",rx->local_audio_buffer_size);
  setProperty(name,value);
  sprintf(name,"receiver[%d].local_audio_latency",rx->channel);
  sprintf(value,"%d",rx->local_audio_latency);
  setProperty(name,value);
  sprintf(name,"receiver[%d].audio_channels",rx->channel);
  sprintf(value,"%d",rx->audio_channels);
  setProperty(name,value);  
  

  sprintf(name,"receiver[%d].step",rx->channel);
  sprintf(value,"%ld",rx->step);
  setProperty(name,value);
  sprintf(name,"receiver[%d].zoom",rx->channel);
  sprintf(value,"%d",rx->zoom);
  setProperty(name,value);
  sprintf(name,"receiver[%d].pan",rx->channel);
  sprintf(value,"%d",rx->pan);
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

  sprintf(name,"receiver[%d].bpsk_enable",rx->channel);
  sprintf(value,"%d",rx->bpsk_enable);
  setProperty(name,value);

  sprintf(name,"receiver[%d].split",rx->channel);
  sprintf(value,"%d",rx->split);
  setProperty(name,value);
  sprintf(name,"receiver[%d].duplex",rx->channel);
  sprintf(value,"%d",rx->duplex);
  setProperty(name,value);
  sprintf(name,"receiver[%d].mute_while_transmitting",rx->channel);
  sprintf(value,"%d",rx->mute_while_transmitting);
  setProperty(name,value);

  sprintf(name,"receiver[%d].rigctl_port",rx->channel);
  sprintf(value,"%d",rx->rigctl_port);
  setProperty(name,value);
  sprintf(name,"receiver[%d].rigctl_enable",rx->channel);
  sprintf(value,"%d",rx->rigctl_enable);
  setProperty(name,value);
  sprintf(name,"receiver[%d].rigctl_serial_enable",rx->channel);
  sprintf(value,"%d",rx->rigctl_serial_enable);
  setProperty(name,value);
  sprintf(name,"receiver[%d].rigctl_serial_port",rx->channel);
  setProperty(name,rx->rigctl_serial_port);
  sprintf(name,"receiver[%d].rigctl_debug",rx->channel);
  sprintf(value,"%d",rx->rigctl_debug);
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
  sprintf(name,"receiver[%d].error_tx",rx->channel);
  value=getProperty(name);
  if(value) rx->error_tx=atol(value);
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
  sprintf(name,"receiver[%d].filter_low_a",rx->channel);
  value=getProperty(name);
  if(value) rx->filter_low_a=atoi(value);
  sprintf(name,"receiver[%d].filter_high_a",rx->channel);
  value=getProperty(name);
  if(value) rx->filter_high_a=atoi(value);
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
  sprintf(name,"receiver[%d].filter_low_b",rx->channel);
  value=getProperty(name);
  if(value) rx->filter_low_b=atoi(value);
  sprintf(name,"receiver[%d].filter_high_b",rx->channel);
  value=getProperty(name);
  if(value) rx->filter_high_b=atoi(value);

  sprintf(name,"receiver[%d].ctun",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun=atoi(value);
  sprintf(name,"receiver[%d].ctun_offset",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun_offset=atol(value);
  sprintf(name,"receiver[%d].ctun_frequency",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun_frequency=atol(value);
  sprintf(name,"receiver[%d].ctun_min",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun_min=atol(value);
  sprintf(name,"receiver[%d].ctun_max",rx->channel);
  value=getProperty(name);
  if(value) rx->ctun_max=atol(value);

  sprintf(name,"receiver[%d].qo100_beacon",rx->channel);
  value=getProperty(name);
  if(value) rx->qo100_beacon=atoi(value);

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
  sprintf(name,"receiver[%d].output_index",rx->channel);
  value=getProperty(name);
  if(value) rx->output_index=atoi(value);
  sprintf(name,"receiver[%d].mute_when_not_active",rx->channel);
  value=getProperty(name);
  if(value) rx->mute_when_not_active=atoi(value);
  sprintf(name,"receiver[%d].local_audio_buffer_size",rx->channel);
  value=getProperty(name);
  if(value) rx->local_audio_buffer_size=atoi(value);
  sprintf(name,"receiver[%d].local_audio_latency",rx->channel);
  value=getProperty(name);
  if(value) rx->local_audio_latency=atoi(value);
  sprintf(name,"receiver[%d].audio_channels",rx->channel);
  value=getProperty(name);  
  if(value) rx->audio_channels=atoi(value);  

  sprintf(name,"receiver[%d].step",rx->channel);
  value=getProperty(name);
  if(value) rx->step=atol(value);
  sprintf(name,"receiver[%d].zoom",rx->channel);
  value=getProperty(name);
  if(value) rx->zoom=atoi(value);
  sprintf(name,"receiver[%d].pan",rx->channel);
  value=getProperty(name);
  if(value) rx->pan=atoi(value);

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
 
  sprintf(name,"receiver[%d].bpsk_enable",rx->channel);
  value=getProperty(name);
  if(value) rx->bpsk_enable=atoi(value);

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
  sprintf(name,"receiver[%d].panadapter_step",rx->channel);
  value=getProperty(name);
  if(value) rx->panadapter_step=atoi(value);
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

  sprintf(name,"receiver[%d].split",rx->channel);
  value=getProperty(name);
  if(value) rx->split=atoi(value);
  sprintf(name,"receiver[%d].duplex",rx->channel);
  value=getProperty(name);
  if(value) rx->duplex=atoi(value);
  sprintf(name,"receiver[%d].mute_while_transmitting",rx->channel);
  value=getProperty(name);
  if(value) rx->mute_while_transmitting=atoi(value);

  sprintf(name,"receiver[%d].rigctl_port",rx->channel);
  value=getProperty(name);
  if(value) rx->rigctl_port=atoi(value);
  sprintf(name,"receiver[%d].rigctl_enable",rx->channel);
  value=getProperty(name);
  if(value) rx->rigctl_enable=atoi(value);
  sprintf(name,"receiver[%d].rigctl_serial_enable",rx->channel);
  value=getProperty(name);
  if(value) rx->rigctl_serial_enable=atoi(value);
  sprintf(name,"receiver[%d].rigctl_serial_port",rx->channel);
  value=getProperty(name);
  if(value) strcpy(rx->rigctl_serial_port,value);
  sprintf(name,"receiver[%d].rigctl_debug",rx->channel);
  value=getProperty(name);
  if(value) rx->rigctl_debug=atoi(value);

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
g_print("receiver_change_sample_rate: from %d to %d radio=%d\n",rx->sample_rate,sample_rate,radio->sample_rate);
  g_mutex_lock(&rx->mutex);
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

#ifdef SOAPYSDR
  if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
    rx->resample_step=radio->sample_rate/rx->sample_rate;
g_print("receiver_change_sample_rate: resample_step=%d\n",rx->resample_step);
  }
#endif

  SetChannelState(rx->channel,1,0);
  g_mutex_unlock(&rx->mutex);
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
  switch(event->button) {
    case 1: // left button
      //if(!rx->locked) {
        rx->last_x=(int)event->x;
        rx->has_moved=FALSE;
        if(rx->zoom>1 && event->y>=rx->panadapter_height-20) {
          rx->is_panning=TRUE;
        }
      //}
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
        update_receiver_dialog(rx);
      }
      break;
  }
  return TRUE;
}

void update_frequency(RECEIVER *rx) {
  if(!rx->locked) {
    if(rx->vfo!=NULL) {
      update_vfo(rx);
    }
    if(radio->transmitter!=NULL) {
      if(radio->transmitter->rx==rx) {
        update_tx_panadapter(radio);
      }
    }
  }
}

long long receiver_move_a(RECEIVER *rx,long long hz,gboolean round) {
  long long delta=0LL;
  if(!rx->locked) {
    if(rx->ctun) {
      delta=rx->ctun_frequency;
      rx->ctun_frequency=rx->ctun_frequency+hz;
      if(round && (rx->mode_a!=CWL || rx->mode_a!=CWU)) {
        rx->ctun_frequency=(rx->ctun_frequency/rx->step)*rx->step;
      }
      delta=rx->ctun_frequency-delta;
    } else {
      delta=rx->frequency_a;
      rx->frequency_a=rx->frequency_a-hz; // DEBUG was +
      if(round && (rx->mode_a!=CWL || rx->mode_a!=CWU)) {
        rx->frequency_a=(rx->frequency_a/rx->step)*rx->step;
      }
      delta=rx->frequency_a-delta;
    }
  }
  return delta;
}

void receiver_move_b(RECEIVER *rx,long long hz,gboolean b_only,gboolean round) {
  if(!rx->locked) {
    switch(rx->split) {
      case SPLIT_OFF:
        if(round) {
          rx->frequency_b=(rx->frequency_b+hz)/rx->step*rx->step;
        } else {
          rx->frequency_b=rx->frequency_b+hz;
        }
        break;
      case SPLIT_ON:
        if(round) {
          rx->frequency_b=(rx->frequency_b+hz)/rx->step*rx->step;
        } else {
          rx->frequency_b=rx->frequency_b+hz;
        }
        break;
      case SPLIT_SAT:
        if(round) {
          rx->frequency_b=(rx->frequency_b+hz)/rx->step*rx->step;
        } else {
          rx->frequency_b=rx->frequency_b+hz;
        }
        if(!b_only) receiver_move_a(rx,hz,round);
        break;
      case SPLIT_RSAT:
        if(round) {
          rx->frequency_b=(rx->frequency_b-hz)/rx->step*rx->step;
        } else {
          rx->frequency_b=rx->frequency_b-hz;
        }
        if(!b_only) receiver_move_a(rx,-hz,round);
        break;
    }
  
    if(rx->subrx_enable) {
      // dont allow frequency outside of passband
    }
  }
}

void receiver_move(RECEIVER *rx,long long hz,gboolean round) {
  if(!rx->locked) {
    long long delta=receiver_move_a(rx,hz,round);
    switch(rx->split) {
      case SPLIT_OFF:
        break;
      case SPLIT_ON:
        break;
      case SPLIT_SAT:
      case SPLIT_RSAT:
        receiver_move_b(rx,delta,TRUE,round);
        break;
    }

    frequency_changed(rx);
    update_frequency(rx);
  }
}

void receiver_move_to(RECEIVER *rx,long long hz) {
  long long delta;
  long long start=(long long)rx->frequency_a-(long long)(rx->sample_rate/2);
  long long offset=hz;
  long long f;

  if(!rx->locked) {
    offset=hz;
  
    f=start+offset+(long long)((double)rx->pan*rx->hz_per_pixel);
    f=f/rx->step*rx->step;
    if(rx->ctun) {
      delta=rx->ctun_frequency;
      rx->ctun_frequency=f;
      delta=rx->ctun_frequency-delta;
    } else {
      if(rx->split==SPLIT_ON && (rx->mode_a==CWL || rx->mode_a==CWU)) {
        if(rx->mode_a==CWU) {
          f=f-radio->cw_keyer_sidetone_frequency;
        } else {
          f=f+radio->cw_keyer_sidetone_frequency;
        }
        rx->frequency_b=f;
      } else {
        delta=rx->frequency_a;
        rx->frequency_a=f;
        delta=rx->frequency_a-delta;
      }
    }
  
    switch(rx->split) {
      case SPLIT_OFF:
        break;
      case SPLIT_ON:
        break;
      case SPLIT_SAT:
        receiver_move_b(rx,delta,TRUE,TRUE);
        break;
      case SPLIT_RSAT:
        receiver_move_b(rx,-delta,TRUE,TRUE);
        break;
    }

    frequency_changed(rx);
    update_frequency(rx);
  }
}

gboolean receiver_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  gint64 hz;
  RECEIVER *rx=(RECEIVER *)data;
  int x;
  int y;
  GdkModifierType state;
  x=event->x;
  y=event->y;
  state=event->state;
  int moved=x-rx->last_x;
  switch(event->button) {
    case 1: // left button
      if(rx->is_panning) {
        int pan=rx->pan+(moved*rx->zoom);
        if(pan<0) {
          pan=0;
        } else if(pan>(rx->pixels-rx->panadapter_width)) {
          pan=rx->pixels-rx->panadapter_width;
        }
        rx->pan=pan;
        rx->last_x=x;
        rx->has_moved=FALSE;
        rx->is_panning=FALSE;
      } else if(!rx->locked) {
        if(rx->has_moved) {
          // drag
          receiver_move(rx,(long long)((double)(moved*rx->hz_per_pixel)),TRUE);
        } else {
          // move to this frequency
          receiver_move_to(rx,(long long)((float)x*rx->hz_per_pixel));
        }
        rx->last_x=x;
        rx->has_moved=FALSE;
    }
  }

  return TRUE;
}

gboolean receiver_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  gint x,y;
  GdkModifierType state;
  RECEIVER *rx=(RECEIVER *)data;
  long long delta;

  x=event->x;
  y=event->y;
  state=event->state;
  int moved=x-rx->last_x;
  if(rx->is_panning) {
    int pan=rx->pan+(moved*rx->zoom);
    if(pan<0) {
      pan=0;
    } else if(pan>(rx->pixels-rx->panadapter_width)) {
      pan=rx->pixels-rx->panadapter_width;
    }
    rx->pan=pan;
    rx->last_x=x;
  } else if(!rx->locked) {
    if((state & GDK_BUTTON1_MASK) == GDK_BUTTON1_MASK) {
      //receiver_move(rx,(long long)((double)(moved*rx->hz_per_pixel)),FALSE);
      receiver_move(rx,(long long)((double)(moved*rx->hz_per_pixel)),TRUE);
      rx->last_x=x;
      rx->has_moved=TRUE;
    }
  }
  return TRUE;
}

gboolean receiver_scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  int x=(int)event->x;
  int y=(int)event->y;
  int half=rx->panadapter_height/2;

  if(rx->zoom>1 && y>=rx->panadapter_height-20) {
    int pan;
    if(event->direction==GDK_SCROLL_UP) {
      pan=rx->pan+rx->zoom;
    } else {
      pan=rx->pan-rx->zoom;
    }

    if(pan<0) {
      pan=0;
    } else if(pan>(rx->pixels-rx->panadapter_width)) {
      pan=rx->pixels-rx->panadapter_width;
    }
    rx->pan=pan;
  } else if(!rx->locked) {
    if((x>4 && x<35) && (widget==rx->panadapter)) {
      if(event->direction==GDK_SCROLL_UP) {
        if(y<half) {
          rx->panadapter_high=rx->panadapter_high-5;
        } else {
          rx->panadapter_low=rx->panadapter_low-5;
        }
      } else {
        if(y<half) {
          rx->panadapter_high=rx->panadapter_high+5;
        } else {
          rx->panadapter_low=rx->panadapter_low+5;
        }
      }
    } else if(event->direction==GDK_SCROLL_UP) {
      if(rx->ctun) {
        receiver_move(rx,rx->step,TRUE);
      } else {
        receiver_move(rx,-rx->step,TRUE);
      }
    } else {
      if(rx->ctun) {
        receiver_move(rx,-rx->step,TRUE);
      } else {
        receiver_move(rx,+rx->step,TRUE);
      }
    }
  }
  return TRUE;
}
        
static gboolean update_timer_cb(void *data) {
  int rc;
  RECEIVER *rx=(RECEIVER *)data;
  char *ptr;

  g_mutex_lock(&rx->mutex);
  if(!isTransmitting(radio) || (rx->duplex)) {
    if(rx->panadapter_resize_timer==-1) {
      GetPixels(rx->channel,0,rx->pixel_samples,&rc);
      if(rc) {
        update_rx_panadapter(rx);
        update_waterfall(rx);
      }
    }
    rx->meter_db=GetRXAMeter(rx->channel,rx->smeter) + radio->meter_calibration;
    update_meter(rx);
  }
  update_radio_info(rx);

#ifdef SOAPYSDR
  if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
    if(radio->transmitter!=NULL && radio->discovered->info.soapy.has_temp) {
      radio->transmitter->updated=FALSE;
    }
  }
#endif

  if(radio->transmitter!=NULL && !radio->transmitter->updated) {
    update_tx_panadapter(radio);
  }
  g_mutex_unlock(&rx->mutex);
  return TRUE;
}
 
static void set_mode(RECEIVER *rx,int m) {
  int previous_mode;
  previous_mode=rx->mode_a;
  rx->mode_a=m;
  SetRXAMode(rx->channel, m);
}

void set_filter(RECEIVER *rx,int low,int high) {
//fprintf(stderr,"set_filter: %d %d\n",low,high);
  if(rx->mode_a==CWL) {
    rx->filter_low_a=-radio->cw_keyer_sidetone_frequency-low;
    rx->filter_high_a=-radio->cw_keyer_sidetone_frequency+high;
  } else if(rx->mode_a==CWU) {
    rx->filter_low_a=radio->cw_keyer_sidetone_frequency-low;
    rx->filter_high_a=radio->cw_keyer_sidetone_frequency+high;
  } else {
    rx->filter_low_a=low;
    rx->filter_high_a=high;
  }
  RXASetPassband(rx->channel,(double)rx->filter_low_a,(double)rx->filter_high_a);
  if(radio->transmitter!=NULL && radio->transmitter->rx==rx && radio->transmitter->use_rx_filter) {
    transmitter_set_filter(radio->transmitter,rx->filter_low_a,rx->filter_high_a);
    update_tx_panadapter(radio);
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
  display_average = max(2, (int)fmin(60, (double)rx->fps * t));
  SetDisplayAvBackmult(rx->channel, 0, display_avb);
  SetDisplayNumAverage(rx->channel, 0, display_average);
}

void receiver_fps_changed(RECEIVER *rx) {
  g_source_remove(rx->update_timer_id);
  rx->update_timer_id=g_timeout_add(1000/rx->fps,update_timer_cb,(gpointer)rx);
  calculate_display_average(rx);
}

void receiver_filter_changed(RECEIVER *rx,int filter) {
//fprintf(stderr,"receiver_filter_changed: %d\n",filter);
  rx->filter_a=filter;
  if(rx->mode_a==FMN) {
    switch(rx->deviation) {
      case 2500:
        set_filter(rx,-4000,4000);
        break;
      case 5000:
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
  gdouble left_sample,right_sample;
  short left_audio_sample, right_audio_sample;
  SUBRX *subrx=(SUBRX *)rx->subrx;

  for (int i=0;i<rx->output_samples;i++) {
    // if subrx is enabled left channel is main and right channel is sub
    if(rx->subrx_enable) {
      left_sample=rx->audio_output_buffer[i*2];
      right_sample=subrx->audio_output_buffer[i*2];
    } else {
      // Rx option for left channel only, right only, or both channels
      switch (rx->audio_channels) {
        case AUDIO_STEREO: {
          left_sample = rx->audio_output_buffer[i*2];
          right_sample = rx->audio_output_buffer[(i*2)+1];     
          break;
        }      
        case AUDIO_LEFT_ONLY: {
          left_sample = rx->audio_output_buffer[i*2];
          right_sample = 0;
          break;
        }
        case AUDIO_RIGHT_ONLY: {
          left_sample = 0;
          right_sample = rx->audio_output_buffer[(i*2)+1];
          break;
        }
      }
    }
    left_audio_sample=(short)(left_sample*32767.0);
    right_audio_sample=(short)(right_sample*32767.0);

    if(rx->local_audio) {
      audio_write(rx,(float)left_sample,(float)right_sample);
    }

    if(radio->active_receiver==rx) {

      if ((isTransmitting(radio) || rx->remote_audio==FALSE) &&  (!rx->duplex)) {
        left_audio_sample=0;
        right_audio_sample=0;
      }
      switch(radio->discovered->protocol) {
        case PROTOCOL_1:
          if(radio->discovered->device!=DEVICE_HERMES_LITE2) {
            protocol1_audio_samples(rx,left_audio_sample,right_audio_sample);
          }
          break;
        case PROTOCOL_2:
          protocol2_audio_samples(rx,left_audio_sample,right_audio_sample);
          break;
      }
    }
  }
  if(rx->local_audio && !rx->output_started) {
    audio_start_output(rx);
  }
}

static void full_rx_buffer(RECEIVER *rx) {
  int error;

  if(isTransmitting(radio) && (!rx->duplex)) return;

  // noise blanker works on origianl IQ samples
  if(rx->nb) {
     xanbEXT (rx->channel, rx->iq_input_buffer, rx->iq_input_buffer);
  }
  if(rx->nb2) {
     xnobEXT (rx->channel, rx->iq_input_buffer, rx->iq_input_buffer);
  }

  g_mutex_lock(&rx->mutex);
  fexchange0(rx->channel, rx->iq_input_buffer, rx->audio_output_buffer, &error);
  //if(error!=0 && error!=-2) {
  if(error!=0) {    
    fprintf(stderr,"full_rx_buffer: channel=%d fexchange0: error=%d\n",rx->channel,error);
  }

  if(rx->subrx_enable) {
    subrx_iq_buffer(rx);
  }

  Spectrum0(1, rx->channel, 0, 0, rx->iq_input_buffer);
  
  process_rx_buffer(rx);
  g_mutex_unlock(&rx->mutex);

}

void add_iq_samples(RECEIVER *rx,double i_sample,double q_sample) {
  rx->iq_input_buffer[rx->samples*2]=i_sample;
  rx->iq_input_buffer[(rx->samples*2)+1]=q_sample;
  rx->samples=rx->samples+1;
  if(rx->samples>=rx->buffer_size) {
    full_rx_buffer(rx);
    rx->samples=0;
  }

  if(rx->bpsk_enable && rx->bpsk!=NULL) {
    bpsk_add_iq_samples(rx->bpsk,i_sample,q_sample);
  }
}

static gboolean receiver_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
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
  gchar title[128];
  g_snprintf((gchar *)&title,sizeof(title),"LinHPSDR: %s Rx-%d ADC-%d %d",radio->discovered->name,rx->channel,rx->adc,rx->sample_rate);
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
  gtk_widget_set_name(rx->window,"receiver-window");
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
  gtk_widget_set_size_request(rx->vfo,715,65);
  gtk_table_attach(GTK_TABLE(rx->table), rx->vfo, 0, 3, 0, 1,
      GTK_FILL, GTK_FILL, 0, 0);
  
  //GtkWidget *radio_info;
  //cairo_surface_t *radio_info_surface;    
  
  
  rx->radio_info=create_radio_info_visual(rx);
  gtk_widget_set_size_request(rx->radio_info, 250, 60);        
  gtk_table_attach(GTK_TABLE(rx->table), rx->radio_info, 3, 4, 0, 1,
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
    int overlap = 2048;
    int clip = 0;
    int span_clip_l = 0;
    int span_clip_h = 0;
    int pixels=rx->pixels;
    int stitches = 1;
    int calibration_data_set = 0;
    double span_min_freq = 0.0;
    double span_max_freq = 0.0;

g_print("receiver_init_analyzer: channel=%d zoom=%d pixels=%d pixel_samples=%p pan=%d\n",rx->channel,rx->zoom,rx->pixels,rx->pixel_samples,rx->pan);

  if(rx->pixel_samples!=NULL) {
//g_print("receiver_init_analyzer: g_free: channel=%d pixel_samples=%p\n",rx->channel,rx->pixel_samples);
    g_free(rx->pixel_samples);
    rx->pixel_samples=NULL;
  }
  if(rx->pixels>0) {
    rx->pixel_samples=g_new0(float,rx->pixels);
g_print("receiver_init_analyzer: g_new0: channel=%d pixel_samples=%p\n",rx->channel,rx->pixel_samples);
    rx->hz_per_pixel=(gdouble)rx->sample_rate/(gdouble)rx->pixels;

    int max_w = fft_size + (int) fmin(keep_time * (double) rx->fps, keep_time * (double) fft_size * (double) rx->fps);

    //overlap = (int)max(0.0, ceil(fft_size - (double)rx->sample_rate / (double)rx->fps));

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

void receiver_change_zoom(RECEIVER *rx,int zoom) {
  rx->zoom=zoom;
  rx->pixels=rx->panadapter_width*rx->zoom;
  if(rx->zoom==1) {
    rx->pan=0;
  } else {
    if(rx->ctun) {
      long long min_frequency=rx->frequency_a-(long long)(rx->sample_rate/2);
      rx->pan=(rx->pixels/2)-(rx->panadapter_width/2);
      //rx->pan=((rx->min_frequency)/rx->hz_per_pixel)-(rx->panadapter_width/2);
      if(rx->pan<0) rx->pan=0;
      if(rx->pan>(rx->pixels-rx->panadapter_width)) rx->pan=rx->pixels-rx->panadapter_width;
    } else {
      rx->pan=(rx->pixels/2)-(rx->panadapter_width/2);
    }
  }
  receiver_init_analyzer(rx);
}

RECEIVER *create_receiver(int channel,int sample_rate) {
  RECEIVER *rx=g_new0(RECEIVER,1);
  char name [80];
  char *value;
  gint x=-1;
  gint y=-1;
  gint width;
  gint height;

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

  switch(radio->discovered->protocol) {
    default:
      rx->frequency_a=14200000;
      rx->band_a=band20;
      rx->mode_a=USB;
      rx->bandstack=1;
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      rx->frequency_a=145000000;
      rx->band_a=band144;
      rx->mode_a=USB;
      rx->bandstack=1;
      break;
#endif
  }

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

  rx->bpsk=NULL;
  rx->bpsk_enable=FALSE;
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
  if(radio->discovered->device==DEVICE_SOAPYSDR) {
    rx->buffer_size=1024; //2048;
  } else {
#endif
    rx->buffer_size=1024;
#ifdef SOAPYSDR
  }
#endif
fprintf(stderr,"create_receiver: buffer_size=%d\n",rx->buffer_size);
  rx->iq_input_buffer=g_new0(gdouble,2*rx->buffer_size);

  rx->audio_buffer_size=480;
  rx->audio_buffer=g_new0(guchar,rx->audio_buffer_size);
  rx->audio_sequence=0;

  rx->mixed_audio=0;

  rx->output_started=FALSE;

  rx->fps=10;
  rx->display_average_time=170.0;

#ifdef SOAPYSDR
  if(radio->discovered->device==DEVICE_SOAPYSDR) {
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
  rx->panadapter_step=20;
  rx->panadapter_surface=NULL;
  
  rx->panadapter_filled=TRUE;
  rx->panadapter_gradient=TRUE;
  rx->panadapter_agc_line=TRUE;

  rx->waterfall_automatic=TRUE;
  rx->waterfall_ft8_marker=FALSE;

  rx->vfo_surface=NULL;
  rx->meter_surface=NULL;
  rx->radio_info_surface=NULL;

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
  rx->local_audio_buffer_size=2048;
  //rx->local_audio_buffer_size=rx->output_samples;
  rx->local_audio_buffer_offset=0;
  rx->local_audio_buffer=NULL;
  rx->local_audio_latency=50;
  rx->audio_channels = 0;
  g_mutex_init(&rx->local_audio_mutex);


  rx->audio_name=NULL;
  rx->mute_when_not_active=FALSE;

  rx->zoom=1;
  rx->pan=0;
  rx->is_panning=FALSE;
  rx->enable_equalizer=FALSE;
  rx->equalizer[0]=0;
  rx->equalizer[1]=0;
  rx->equalizer[2]=0;
  rx->equalizer[3]=0;

  rx->bookmark_dialog=NULL;

  rx->filter_frame=NULL;
  rx->filter_grid=NULL;

  rx->rigctl_port=19090+rx->channel;
  rx->rigctl_enable=FALSE;

  strcpy(rx->rigctl_serial_port,"/dev/ttyACM0");
  rx->rigctl_serial_baudrate=B9600;
  rx->rigctl_serial_enable=FALSE;
  rx->rigctl_debug=FALSE;
  rx->rigctl=NULL;

  rx->vpaned=NULL;
  rx->paned_position=-1;
  rx->paned_percent=0.5;

  rx->split=SPLIT_OFF;
  rx->duplex=FALSE;
  rx->mute_while_transmitting=FALSE;

  rx->vfo=NULL;
  rx->subrx_enable=FALSE;
  rx->subrx=NULL;

  receiver_restore_state(rx);

  if(radio->discovered->protocol==PROTOCOL_1) {
    if(rx->sample_rate!=sample_rate) {
      rx->sample_rate=sample_rate;
    }
  }

  rx->output_samples=rx->buffer_size/(rx->sample_rate/48000);
  rx->audio_output_buffer=g_new0(gdouble,2*rx->output_samples);

g_print("create_receiver: OpenChannel: channel=%d buffer_size=%d sample_rate=%d fft_size=%d output_samples=%d\n", rx->channel, rx->buffer_size, rx->sample_rate, rx->fft_size,rx->output_samples);

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
#ifdef SOAPYSDR
  if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
    rx->resample_step=radio->sample_rate/rx->sample_rate;
g_print("receiver_change_sample_rate: resample_step=%d\n",rx->resample_step);
  }
#endif


  frequency_changed(rx);
  receiver_mode_changed(rx,rx->mode_a);

  SetRXAPanelGain1(rx->channel, rx->volume);
  SetRXAPanelSelect(rx->channel, 3);
  SetRXAPanelPan(rx->channel, 0.5);
  SetRXAPanelCopy(rx->channel, 0);
  SetRXAPanelBinaural(rx->channel, 0);
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

  if(rx->bpsk_enable) {
    rx->bpsk=create_bpsk(BPSK_CHANNEL,rx->band_a);
  }


  if(rx->rigctl_enable) {
    launch_rigctl(rx);
  }

  if(rx->rigctl_serial_enable) {
    launch_serial(rx);
  }

  return rx;
}

void receiver_set_volume(RECEIVER *rx) {
  SetRXAPanelGain1(rx->channel, rx->volume);
  if(rx->subrx_enable) {
    subrx_volume_changed(rx);
  }
}

void receiver_set_agc_gain(RECEIVER *rx) {
  SetRXAAGCTop(rx->channel, rx->agc_gain);
}

void receiver_set_ctun(RECEIVER *rx) {
  rx->ctun_offset=0;
  rx->ctun_frequency=rx->frequency_a;
  rx->ctun_min=rx->frequency_a-(rx->sample_rate/2);
  rx->ctun_max=rx->frequency_a+(rx->sample_rate/2);
  if(!rx->ctun) {
    SetRXAShiftRun(rx->channel, 0);
  } else {
    SetRXAShiftRun(rx->channel, 1);
  }
  frequency_changed(rx);
  update_frequency(rx);
}
