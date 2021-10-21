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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <wdsp.h>

#include "alex.h"
#include "button_text.h"
#include "discovered.h"
#include "bpsk.h"
#include "mode.h"
#include "filter.h"
#include "band.h"
#include "receiver.h"
#include "transmitter.h"
#include "receiver.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "tx_panadapter.h"
#include "protocol1.h"
#include "protocol2.h"
#ifdef SOAPYSDR
#include "soapy_protocol.h"
#endif
#include "main.h"
#include "configure_dialog.h"
#include "audio.h"
#include "vfo.h"
#include "mic_level.h"
#include "mic_gain.h"
#include "drive_level.h"
#include "frequency.h"
#include "property.h"
//#include "rigctl.h"
#include "receiver_dialog.h"
#include "subrx.h"

#ifdef MIDI
#include "midi.h"
#include "midi_dialog.h"
// rather than including MIDI.h with all its internal stuff
// (e.g. enum components) we just declare the single bit thereof
// we need here to make a strict compiler happy.
int MIDIstartup(char *filename);
#endif

static GtkWidget *add_receiver_b;
static GtkWidget *add_wideband_b;

static void rxtx(RADIO *r);

int radio_restart(void *data) {
  RADIO *r=(RADIO *)data;
fprintf(stderr,"radio_restart\n");
  switch(r->discovered->protocol) {
    case PROTOCOL_1:
      protocol1_run(r);
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      soapy_protocol_change_sample_rate(r->receiver[0],r->sample_rate);
      break;
#endif
  }
  if(r->transmitter!=NULL) {
    update_tx_panadapter(r);
  }
  return 0;
}

int radio_start(void *data) {
  RADIO *r=(RADIO *)data;
fprintf(stderr,"radio_start\n");
  switch(r->discovered->protocol) {
    case PROTOCOL_1:
      protocol1_run(r);
      break;
    case PROTOCOL_2:
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      break;
#endif
  }
  if(r->transmitter!=NULL) {
    update_tx_panadapter(r);
  }
  return 0;
}

void radio_save_state(RADIO *radio) {
  char name[80];
  char value[80];
  int i;
  gint x,y;
  gint width,height;
  char filename[128];
  switch(radio->discovered->protocol) {
    
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      sprintf(filename,"%s/.local/share/linhpsdr/%s.props",
                        g_get_home_dir(),
                        radio->discovered->name);
      break;
#endif
    default:
      sprintf(filename,"%s/.local/share/linhpsdr/%02X-%02X-%02X-%02X-%02X-%02X.props",
                        g_get_home_dir(),
                        radio->discovered->info.network.mac_address[0],
                        radio->discovered->info.network.mac_address[1],
                        radio->discovered->info.network.mac_address[2],
                        radio->discovered->info.network.mac_address[3],
                        radio->discovered->info.network.mac_address[4],
                        radio->discovered->info.network.mac_address[5]);
      break;
  }

g_print("radio_save_state: %s\n",filename);
  initProperties();

  sprintf(value,"%d",radio->model);
  setProperty("radio.model",value);
  sprintf(value,"%d",radio->sample_rate);
  setProperty("radio.sample_rate",value);
  sprintf(value,"%d",radio->buffer_size);
  setProperty("radio.buffer_size",value);
  sprintf(value,"%d",radio->receivers);
  setProperty("radio.receivers",value);
  sprintf(value,"%f",radio->meter_calibration);
  setProperty("radio.meter_calibration",value);
  sprintf(value,"%f",radio->panadapter_calibration);
  setProperty("radio.panadapter_calibration",value);
  sprintf(value,"%d",radio->cw_keyer_sidetone_frequency);
  setProperty("radio.cw_keyer_sidetone_frequency",value);
  sprintf(value,"%d",radio->cw_keyer_sidetone_volume);
  setProperty("radio.cw_keyer_sidetone_volume",value);
  sprintf(value,"%d",radio->cw_keys_reversed);
  setProperty("radio.cw_keys_reversed",value);
  sprintf(value,"%d",radio->cw_keyer_speed);
  setProperty("radio.cw_keyer_speed",value);
  sprintf(value,"%d",radio->cw_keyer_mode);
  setProperty("radio.cw_keyer_mode",value);
  sprintf(value,"%d",radio->cw_keyer_weight);
  setProperty("radio.cw_keyer_weight",value);
  sprintf(value,"%d",radio->cw_keyer_spacing);
  setProperty("radio.cw_keyer_internal",value);
  sprintf(value,"%d",radio->cw_keyer_internal);
  setProperty("radio.cw_keyer_internal",value);
  sprintf(value,"%d",radio->cw_keyer_ptt_delay);
  setProperty("radio.cw_keyer_ptt_delay",value);
  sprintf(value,"%d",radio->cw_keyer_hang_time);
  setProperty("radio.cw_keyer_hang_time",value);
  sprintf(value,"%d",radio->cw_breakin);
  setProperty("radio.cw_breakin",value);
  sprintf(value,"%d",radio->local_microphone);
  setProperty("radio.local_microphone",value);
  sprintf(value,"%d",radio->mic_boost);
  setProperty("radio.mic_boost",value);
  if(radio->microphone_name!=NULL) {
    setProperty("radio.microphone_name",radio->microphone_name);
  }
  sprintf(value,"%d",radio->mic_ptt_enabled);
  setProperty("radio.mic_ptt_enabled",value);
  sprintf(value,"%d",radio->mic_bias_enabled);
  setProperty("radio.mic_bias_enabled",value);
  sprintf(value,"%d",radio->mic_ptt_tip_bias_ring);
  setProperty("radio.mic_ptt_tip_bias_ring",value);
  sprintf(value,"%d",radio->mic_linein);
  setProperty("radio.mic_linein",value);
  sprintf(value,"%d",radio->linein_gain);
  setProperty("radio.linein_gain",value);

  for(int i=0;i<radio->discovered->adcs;i++) {
    sprintf(name,"radio.adc[%d].filters",i);
    sprintf(value,"%d",radio->adc[i].filters);
    setProperty(name,value);
    sprintf(name,"radio.adc[%d].hpf",i);
    sprintf(value,"%d",radio->adc[i].hpf);
    setProperty(name,value);
    sprintf(name,"radio.adc[%d].lpf",i);
    sprintf(value,"%d",radio->adc[i].lpf);
    setProperty(name,value);
    sprintf(name,"radio.adc[%d].antenna",i);
    sprintf(value,"%d",radio->adc[i].antenna);
    setProperty(name,value);
    sprintf(name,"radio.adc[%d].dither",i);
    sprintf(value,"%d",radio->adc[i].dither);
    setProperty(name,value);
    sprintf(name,"radio.adc[%d].random",i);
    sprintf(value,"%d",radio->adc[i].random);
    setProperty(name,value);
    sprintf(name,"radio.adc[%d].preamp",i);
    sprintf(value,"%d",radio->adc[i].preamp);
    setProperty(name,value);
    sprintf(name,"radio.adc[%d].attenuation",i);
    sprintf(value,"%d",radio->adc[i].attenuation);
    setProperty(name,value);

#ifdef SOAPYSDR
    if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
      sprintf(name,"radio.adc[%d].gain",i);
      sprintf(value,"%f", radio->adc[0].gain);
      setProperty(name,value);
      sprintf(name,"radio.adc[%d].agc",i);
      sprintf(value,"%d", soapy_protocol_get_automatic_gain(&radio->adc[0]));
      setProperty(name,value);
      sprintf(name,"radio.dac[%d].gain",i);
      sprintf(value,"%f", radio->dac[0].gain);
      setProperty(name,value);
      sprintf(name,"radio.dac[%d].antenna",i);
      sprintf(value,"%d", radio->dac[0].antenna);
      setProperty(name,value);
    }
#endif
  }

  sprintf(value,"%d",radio->filter_board);
  setProperty("radio.filter_board",value);

  sprintf(value,"%d",radio->region);
  setProperty("radio.region",value);

  sprintf(value,"%d",radio->classE);
  setProperty("radio.classE",value);

  sprintf(value,"%d",radio->temperature_alarm_value);
  setProperty("radio.temp_alarm",value);
  sprintf(value,"%f",radio->swr_alarm_value);
  setProperty("radio.swr_alarm",value);

/*
  sprintf(value,"%d",rigctl_enable);
  setProperty("rigctl_enable",value);
  sprintf(value,"%d",rigctl_port_base);
  setProperty("rigctl_port_base",value);
*/
  sprintf(value,"%d",radio->iqswap);
  setProperty("radio.iqswap",value);

  sprintf(value,"%d",radio->which_audio);
  setProperty("radio.which_audio",value);

  sprintf(value,"%d",radio->which_audio_backend);
  setProperty("radio.which_audio_backend",value);

  filterSaveState();
  bandSaveState();

  for(i=0;i<radio->discovered->supported_receivers;i++) {
    if(radio->receiver[i]!=NULL) {
      receiver_save_state(radio->receiver[i]);
    }
  }

  if(radio->discovered->supported_transmitters!=0) {
    transmitter_save_state(radio->transmitter);
  }

#ifdef MIDI
  setProperty("radio.midi_filename",radio->midi_filename);
  sprintf(value,"%d",radio->midi_enabled);
  setProperty("radio.midi_enabled",value);

  midi_save_state();
#endif

  gtk_window_get_position(GTK_WINDOW(main_window),&x,&y);
  sprintf(value,"%d",x);
  setProperty("radio.x",value);
  sprintf(value,"%d",y);
  setProperty("radio.y",value);

  gtk_window_get_size(GTK_WINDOW(main_window),&width,&height);
  sprintf(value,"%d",width);
  setProperty("radio.width",value);
  sprintf(value,"%d",height);
  setProperty("radio.height",value);

  saveProperties(filename);
}

void radio_restore_state(RADIO *radio) {
  char name[80];
  char *value;
  char filename[128];
  switch(radio->discovered->protocol) {
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      sprintf(filename,"%s/.local/share/linhpsdr/%s.props",
                        g_get_home_dir(),
                        radio->discovered->name);
      break;
#endif
    default:
      sprintf(filename,"%s/.local/share/linhpsdr/%02X-%02X-%02X-%02X-%02X-%02X.props",
                        g_get_home_dir(),
                        radio->discovered->info.network.mac_address[0],
                        radio->discovered->info.network.mac_address[1],
                        radio->discovered->info.network.mac_address[2],
                        radio->discovered->info.network.mac_address[3],
                        radio->discovered->info.network.mac_address[4],
                        radio->discovered->info.network.mac_address[5]);
      break;
  }

  loadProperties(filename);

  value=getProperty("radio.model");
  if(value!=NULL) radio->model=atoi(value);

  value=getProperty("radio.sample_rate");
  if(value!=NULL) radio->sample_rate=atoi(value);
  
  value=getProperty("radio.meter_calibration");
  if(value) radio->meter_calibration=atof(value);
  value=getProperty("radio.panadapter_calibration");
  if(value) radio->panadapter_calibration=atof(value);
  value=getProperty("radio.cw_keyer_sidetone_frequency");
  if(value!=NULL) radio->cw_keyer_sidetone_frequency=atoi(value);
  value=getProperty("radio.cw_keyer_sidetone_volume");
  if(value!=NULL) radio->cw_keyer_sidetone_volume=atoi(value);
  value=getProperty("radio.cw_keys_reversed");
  if(value!=NULL) radio->cw_keys_reversed=atoi(value);
  value=getProperty("radio.cw_keyer_speed");
  if(value!=NULL) radio->cw_keyer_speed=atoi(value);
  value=getProperty("radio.cw_keyer_mode");
  if(value!=NULL) radio->cw_keyer_mode=atoi(value);
  value=getProperty("radio.cw_keyer_weight");
  if(value!=NULL) radio->cw_keyer_weight=atoi(value);
  value=getProperty("radio.cw_keyer_internal");
  if(value!=NULL) radio->cw_keyer_internal=atoi(value);
  value=getProperty("radio.cw_keyer_ptt_delay");
  if(value!=NULL) radio->cw_keyer_ptt_delay=atoi(value);
  value=getProperty("radio.cw_keyer_hang_time");
  if(value!=NULL) radio->cw_keyer_hang_time=atoi(value);
  value=getProperty("radio.cw_breakin");
  if(value!=NULL) radio->cw_breakin=atoi(value);

  for(int i=0;i<radio->discovered->adcs;i++) {
    sprintf(name,"radio.adc[%d].filters",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].filters=atoi(value);
    sprintf(name,"radio.adc[%d].hpf",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].hpf=atoi(value);
    sprintf(name,"radio.adc[%d].lpf",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].lpf=atoi(value);
    sprintf(name,"radio.adc[%d].antenna",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].antenna=atoi(value);
    sprintf(name,"radio.adc[%d].dither",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].dither=atoi(value);
    sprintf(name,"radio.adc[%d].random",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].random=atoi(value);
    sprintf(name,"radio.adc[%d].preamp",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].preamp=atoi(value);
    sprintf(name,"radio.adc[%d].attenuation",i);
    value=getProperty(name);
    if(value!=NULL) radio->adc[i].attenuation=atoi(value);

#ifdef SOAPYSDR
    if(radio->discovered->device==DEVICE_SOAPYSDR) {
      sprintf(name,"radio.adc[%d].gain",i);
      value=getProperty(name);
      if(value!=NULL) radio->adc[i].gain=atof(value);
      sprintf(name,"radio.adc[%d].agc",i);
      value=getProperty(name);
      if(value!=NULL) radio->adc[i].agc=atoi(value);
      if(radio->can_transmit) {
        sprintf(name,"radio.dac[%d].gain",i);
        value=getProperty(name);
        if(value!=NULL) radio->dac[i].gain=atof(value);
        sprintf(name,"radio.dac[%d].antenna",i);
        value=getProperty(name);
        if(value!=NULL) radio->dac[i].antenna=atoi(value);
      }
    }
#endif
  }

  value=getProperty("radio.local_microphone");
  if(value!=NULL) radio->local_microphone=atoi(value);
  value=getProperty("radio.microphone_name");
  if(value!=NULL) {
    radio->microphone_name=g_new0(gchar,strlen(value)+1);
    strcpy(radio->microphone_name,value);
  }
  value=getProperty("radio.mic_boost");
  if(value!=NULL) radio->mic_boost=atoi(value);
  value=getProperty("radio.mic_ptt_enabled");
  if(value!=NULL) radio->mic_ptt_enabled=atoi(value);
  value=getProperty("radio.mic_bias_enabled");
  if(value!=NULL) radio->mic_bias_enabled=atoi(value);
  value=getProperty("radio.mic_ptt_tip_bias_ring");
  if(value!=NULL) radio->mic_ptt_tip_bias_ring=atoi(value);
  value=getProperty("radio.mic_linein");
  if(value!=NULL) radio->mic_linein=atoi(value);
  value=getProperty("radio.linein_gain");
  if(value!=NULL) radio->linein_gain=atoi(value);
  value=getProperty("radio.region");
  if(value!=NULL) radio->region=atoi(value);
  value=getProperty("radio.classE");
  if(value!=NULL) radio->classE=atoi(value);

  value=getProperty("radio.temp_alarm");
  if(value!=NULL) radio->temperature_alarm_value=atoi(value);
  value=getProperty("radio.swr_alarm");
  if(value!=NULL) radio->swr_alarm_value=atof(value);

  value=getProperty("radio.iqswap");
  if(value) radio->iqswap=atoi(value);

  value=getProperty("radio.which_audio");
  if(value) radio->which_audio=atoi(value);

  value=getProperty("radio.which_audio_backend");
  if(value) radio->which_audio_backend=atoi(value);

/*
  value=getProperty("rigctl_enable");
  if(value!=NULL) rigctl_enable=atoi(value);
  value=getProperty("rigctl_port_base");
  if(value!=NULL) rigctl_port_base=atoi(value);
*/

#ifdef MIDI
  midi_restore_state();
  value=getProperty("radio.midi_filename");
  if(value) strcpy(radio->midi_filename,value);
  value=getProperty("radio.midi_enabled");
  if(value) radio->midi_enabled=atoi(value);
#endif

  filterRestoreState();
  bandRestoreState();
}

gboolean radio_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  g_print("%s\n",__FUNCTION__);
  switch(event->button) {
    case 1: // left button
      break;
    case 3: // right button
      if(radio->dialog==NULL) {
        //radio->dialog=create_radio_dialog(radio);
        radio->dialog=create_configure_dialog(radio,0);
      }
      break;
  }
  return TRUE;
}

void radio_change_region(RADIO *r) {
  if(r->region==REGION_UK) {
    channel_entries=UK_CHANNEL_ENTRIES;
    band_channels_60m=&band_channels_60m_UK[0];
    bandstack60.entries=UK_CHANNEL_ENTRIES;
    bandstack60.current_entry=0;
    bandstack60.entry=bandstack_entries60_UK;
  } else {
    channel_entries=OTHER_CHANNEL_ENTRIES;
    band_channels_60m=&band_channels_60m_OTHER[0];
    bandstack60.entries=OTHER_CHANNEL_ENTRIES;
    bandstack60.current_entry=0;
    bandstack60.entry=bandstack_entries60_OTHER;
  }
}

void radio_change_audio(RADIO *r,int selected) {
  int i;
  g_print("%s: %dn",__FUNCTION__,selected);
  if(r->which_audio!=selected) {
    if(r->local_microphone) {
      radio->local_microphone=FALSE;
      audio_close_input(r);
    }
    for(i=0;i<radio->discovered->supported_receivers;i++) {
      if(radio->receiver[i]!=NULL) {
        if(radio->receiver[i]->local_audio) {
          radio->receiver[i]->local_audio=FALSE;
          audio_close_output(radio->receiver[i]);
        }
      }
    }
  }

  r->which_audio=selected;
  create_audio(r->which_audio_backend,r->which_audio==USE_SOUNDIO?audio_get_backend_name(r->which_audio_backend):NULL);
}

void radio_change_audio_backend(RADIO *r,int selected) {
  int i;
  g_print("%s: %d\n",__FUNCTION__,selected);
  if(r->which_audio_backend!=selected) {
    if(r->local_microphone) {
      radio->local_microphone=FALSE;
      audio_close_input(r);
    }
    for(i=0;i<radio->discovered->supported_receivers;i++) {
      if(radio->receiver[i]!=NULL) {
        if(radio->receiver[i]->local_audio) {
          radio->receiver[i]->local_audio=FALSE;
          audio_close_output(radio->receiver[i]);
        }
      }
    }
  }

  r->which_audio_backend=selected;
  create_audio(r->which_audio_backend,r->which_audio==USE_SOUNDIO?audio_get_backend_name(r->which_audio_backend):NULL);
}


void vox_changed(RADIO *r) {
  rxtx(radio);
}

void frequency_changed(RECEIVER *rx) {
  if(rx->ctun) {
    gint64 offset;
    rx->ctun_offset=rx->ctun_frequency-rx->frequency_a;
    offset=rx->ctun_offset;

    //g_print("%s: offset=%ld\n",__FUNCTION__,offset);
    if(rx->mode_a==CWU) {
      offset+=(gint64)radio->cw_keyer_sidetone_frequency;
    } else if(rx->mode_a==CWL) {
      offset-=(gint64)radio->cw_keyer_sidetone_frequency;
    }
    if(rx->rit_enabled) {
      offset+=rx->rit;
    }
    SetRXAShiftFreq(rx->channel, (double)offset);
    RXANBPSetShiftFrequency(rx->channel, (double)offset);
#ifdef SOAPYSDR
    if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
      // delay setting tx frequency until transmit
    }
#endif
  } else {
    if(radio->discovered->protocol==PROTOCOL_2) {
      protocol2_high_priority();
#ifdef SOAPYSDR
    } else if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {
      soapy_protocol_set_rx_frequency(rx);
#endif
    }
    rx->band_a=get_band_from_frequency(rx->frequency_a);
  }

  if(rx->subrx_enable) {
    // make sure VFO B frequency is in the passband
    gint64 min_frequency=rx->frequency_a-(gint64)(rx->sample_rate/2);
    gint64 max_frequency=rx->frequency_a+(gint64)(rx->sample_rate/2);

    gint64 filter_low_frequency=rx->frequency_b+(gint64)rx->filter_low_b;
    gint64 filter_high_frequency=rx->frequency_b+(gint64)rx->filter_high_b;

    if(filter_low_frequency<min_frequency) {
      rx->frequency_b=min_frequency+(gint64)rx->filter_low_b;
    } else if(filter_high_frequency>max_frequency) {
      rx->frequency_b=max_frequency-(gint64)rx->filter_high_b;
    }
    subrx_frequency_changed(rx);
  }
}

gboolean isTransmitting(RADIO *r) {
  return (r->ptt | r->mox | r->vox | r->tune);
}

void delete_wideband(WIDEBAND *w) {
  if(radio->wideband==w) {
    radio->wideband=NULL;
  }
  if(radio->wideband==NULL) {
    gtk_widget_set_sensitive(add_wideband_b,TRUE);
  }
  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_stop_wideband();
  }
  if(radio->dialog) {
    gtk_widget_destroy(radio->dialog);
    radio->dialog=NULL;
  }
}

void delete_receiver(RECEIVER *rx) {
  int i;
  for(i=0;i<radio->discovered->supported_receivers;i++) {
    if(radio->receiver[i]==rx) {
      if(radio->discovered->protocol==PROTOCOL_1) {
        protocol1_stop();
      }
      if(radio->transmitter!=NULL && radio->transmitter->rx==rx) {
        radio->transmitter->rx=NULL;
      }
      radio->receiver[i]=NULL;
      radio->receivers--;
      if(radio->discovered->protocol==PROTOCOL_1) {
        protocol1_run();
      }
g_print("delete_receiver: receivers now %d\n",radio->receivers);
      break;
    }
  }

  if(radio->transmitter!=NULL && radio->transmitter->rx==NULL) {
    if(radio->receivers>0) {
      for(i=0;i<radio->discovered->supported_receivers;i++) {
        if(radio->receiver[i]!=NULL) {
          radio->transmitter->rx=radio->receiver[i];
          update_vfo(radio->receiver[i]);
          break;
        }
      }
    } else {
      // no more receivers
    }
  }

  gtk_widget_set_sensitive(add_receiver_b,radio->receivers<radio->discovered->supported_receivers);
  if(radio->dialog) {
    gtk_widget_destroy(radio->dialog);
    radio->dialog=NULL;
  }
}

static void rxtx(RADIO *r) {
  int i;
g_print("%s: isTransmitting=%d\n",__FUNCTION__,isTransmitting(r));
  if(isTransmitting(r)) {
    for(i=0;i<r->discovered->supported_receivers;i++) {
      if(r->receiver[i]!=NULL) {
        if(!r->receiver[i]->duplex) {
          SetChannelState(r->receiver[i]->channel,0,1);        
        }
      }
    }
    SetChannelState(r->transmitter->channel,1,0);
    switch(r->discovered->protocol) {
      case PROTOCOL_1:
        break;
      case PROTOCOL_2:
        protocol2_high_priority();
        protocol2_receive_specific();
        break;
#ifdef SOAPYSDR
      case PROTOCOL_SOAPYSDR:
        soapy_protocol_set_tx_frequency(r->transmitter);
        break;
#endif
    }
  } else {
    SetChannelState(r->transmitter->channel,0,1);
    for(i=0;i<r->discovered->supported_receivers;i++) {
      if(r->receiver[i]!=NULL) {
        if(!r->receiver[i]->duplex) {
          SetChannelState(r->receiver[i]->channel,1,0);
        }
      }
    }
    switch(r->discovered->protocol) {
      case PROTOCOL_1:
        break;
      case PROTOCOL_2:
        protocol2_high_priority();
        protocol2_receive_specific();
        break;
#ifdef SOAPYSDR
      case PROTOCOL_SOAPYSDR:
        //soapy_protocol_stop_transmitter(r->transmitter);
        break;
#endif
    }
    update_tx_panadapter(r);
  }
  update_vfo(r->transmitter->rx);
}

void set_mox(RADIO *r,gboolean state) {
g_print("%s: state=%d\n",__FUNCTION__,state);
  if(r->tune) {
    r->tune=FALSE;
    SetTXAPostGenRun(radio->transmitter->channel,0);
    switch(radio->transmitter->rx->mode_a) {
      case CWL:
      case CWU:
        SetTXAMode(radio->transmitter->channel, radio->transmitter->rx->mode_a);
        break;
    }
  }
  r->mox=state;
  rxtx(r);
  update_radio(r);
}

void ptt_changed(RADIO *r) {
g_print("ptt_changed\n");
  set_mox(r,r->local_ptt);
  update_vfo(r->transmitter->rx);
}

static void mox_cb(GtkToggleButton *widget,gpointer data) {
  RADIO *r=(RADIO *)data;
g_print("mox_cb: mox=%d\n",r->mox);
  if(r->mox) {
    set_mox(r,FALSE);
  } else {
    set_mox(r,TRUE);
  }
}

static void vox_cb(GtkToggleButton *widget,gpointer data) {
  RADIO *r=(RADIO *)data;
  r->vox_enabled=!r->vox_enabled;
  update_radio(r);
}

void set_tune(RADIO *r,gboolean state) {
  if(r->mox) {
    r->mox=FALSE;
    set_button_text_color(r->mox_button,r->mox?"red":"black");
  }
  r->tune=state;
  if(r->tune) {
    //SM4VEY
    struct timeval te;
    gettimeofday(&te,NULL);
    long long now=te.tv_sec*1000LL+te.tv_usec/1000 + r->OCfull_tune_time;
    r->tune_timeout = now;
    
    switch(r->transmitter->rx->mode_a) {
      case CWL:
        SetTXAMode(r->transmitter->channel, LSB);
        SetTXAPostGenToneFreq(r->transmitter->channel, -(double)r->cw_keyer_sidetone_frequency);
        r->cw_keyer_internal=FALSE;
        break;
      case LSB:
      case DIGL:
        SetTXAPostGenToneFreq(r->transmitter->channel, (double)(-r->transmitter->filter_low-((r->transmitter->filter_high-r->transmitter->filter_low)/2)));
        break;
      case CWU:
        SetTXAMode(r->transmitter->channel, USB);
        SetTXAPostGenToneFreq(r->transmitter->channel, (double)r->cw_keyer_sidetone_frequency);
        r->cw_keyer_internal=FALSE;
        break;
      default:
        SetTXAPostGenToneFreq(r->transmitter->channel, (double)(r->transmitter->filter_low+((r->transmitter->filter_high-r->transmitter->filter_low)/2)));
        break;
    }
    SetTXAPostGenToneMag(r->transmitter->channel,0.99999);
    SetTXAPostGenMode(r->transmitter->channel,0);
    SetTXAPostGenRun(r->transmitter->channel,1);
    rxtx(r);
  } else {
    SetTXAPostGenRun(r->transmitter->channel,0);
    switch(r->transmitter->rx->mode_a) {
      case CWL:
      case CWU:
        SetTXAMode(r->transmitter->channel, r->transmitter->rx->mode_a);
        r->cw_keyer_internal=TRUE;
        break;
    }
    rxtx(r);
  }
  update_radio(r);
}

static void tune_cb(GtkToggleButton *widget,gpointer data) {
  RADIO *r=(RADIO *)data;
  if(r->tune) {
    set_tune(r,FALSE);
  } else {
    set_tune(r,TRUE);
  }
}

int add_receiver(void *data) {
  RADIO *r=(RADIO *)data;
  int i;
  for(i=0;i<r->discovered->supported_receivers;i++) {
    if(r->receiver[i]==NULL) {
      break;
    }
  }
  if(i<r->discovered->supported_receivers) {
g_print("add_receiver: using receiver %d\n",i);
    r->receiver[i]=create_receiver(i,r->sample_rate);
    r->receivers++;
g_print("add_receiver: receivers now %d\n",r->receivers);
    switch(r->discovered->protocol) {
      case PROTOCOL_2:
        protocol2_start_receiver(r->receiver[i]);
        break;
#ifdef SOAPYSDR
      case PROTOCOL_SOAPYSDR:
        soapy_protocol_create_receiver(r->receiver[i]);
        RECEIVER *rx=r->receiver[i];
        int adc=rx->adc;
        soapy_protocol_set_rx_antenna(radio->receiver[i],radio->adc[adc].antenna);
        double f=(double)(rx->frequency_a-rx->lo_a+rx->error_a);
        soapy_protocol_set_rx_frequency(radio->receiver[i]);
        soapy_protocol_set_automatic_gain(radio->receiver[i],radio->adc[adc].agc);
        for(int i=0;i<radio->discovered->info.soapy.rx_gains;i++) {
          soapy_protocol_set_gain(&radio->adc[adc]);
        } 
        soapy_protocol_start_receiver(rx);
        break;
#endif
      default:
        break;
    }
  } else {
g_print("add_receiver: no receivers available\n");
  }

  gtk_widget_set_sensitive(add_receiver_b,r->receivers<r->discovered->supported_receivers);

  if(radio->dialog) {
    gtk_widget_destroy(radio->dialog);
    radio->dialog=NULL;
  }
  return 0;
}

void add_receivers(RADIO *r) {
  char name[80];
  char *value;
  int receivers;
  int i;

  receivers=0;
  value=getProperty("radio.receivers");
  if(value!=NULL) receivers=atoi(value);

  // always add receiver 0
  if(receivers==0) {
    r->receiver[0]=create_receiver(0,r->sample_rate);
    r->receivers++;
    switch(r->discovered->protocol) {
      case PROTOCOL_2:
        protocol2_start_receiver(r->receiver[0]);
        break;
#ifdef SOAPYSDR
      case PROTOCOL_SOAPYSDR:
        soapy_protocol_create_receiver(r->receiver[0]);
        break;
#endif
      default:
        break;
    }
  } else {
    for(i=0;i<r->discovered->supported_receivers;i++) {
      sprintf(name,"receiver[%d].channel",i);
      value=getProperty(name);
      if(value!=NULL) {
        r->receiver[i]=create_receiver(i,r->sample_rate);
        r->receivers++;
        switch(r->discovered->protocol) {
          case PROTOCOL_2:
            protocol2_start_receiver(r->receiver[i]);
            break;
#ifdef SOAPYSDR
          case PROTOCOL_SOAPYSDR:
            soapy_protocol_create_receiver(r->receiver[i]);
            break;
#endif
          default:
            break;
        }
      }
    }
  }

  for(i=0;i<r->discovered->supported_receivers;i++) {
    if(r->receiver[i]!=NULL) {
      r->active_receiver=r->receiver[i];
      break;
    }
  }

  if(radio->discovered->protocol==PROTOCOL_2) {
    protocol2_general();
    protocol2_high_priority();
    protocol2_receive_specific();
  }

}

void add_transmitter(RADIO *r) {
  r->transmitter=create_transmitter(TRANSMITTER_CHANNEL);
  r->transmitter->rx=r->receiver[0];
  if(r->transmitter->rx->split) {
    transmitter_set_mode(r->transmitter,r->transmitter->rx->mode_b);
  } else {
    transmitter_set_mode(r->transmitter,r->transmitter->rx->mode_a);
  }
  update_vfo(r->transmitter->rx);
}

int add_wideband(void *data) {
  RADIO *r=(RADIO *)data;
  r->wideband=create_wideband(WIDEBAND_CHANNEL);
  if(r->discovered->protocol==PROTOCOL_2) {
    protocol2_start_wideband(r->wideband);
  }
  if(radio->dialog) {
    gtk_widget_destroy(radio->dialog);
    radio->dialog=NULL;
  }
  return 0;
}

static gboolean add_receiver_cb(GtkWidget *widget,gpointer data) {
  RADIO *r=(RADIO *)data;
  add_receiver(r);
  return TRUE;
}

static gboolean add_wideband_cb(GtkWidget *widget,gpointer data) {
  RADIO *r=(RADIO *)data;
  add_wideband(r);
  if(r->wideband !=NULL) {
    gtk_widget_set_sensitive(add_wideband_b,FALSE);
  }
  return TRUE;
}

static gboolean configure_cb(GtkWidget *widget,gpointer data) {
  RADIO *radio=(RADIO *)data;
  if(radio->dialog==NULL) {
    radio->dialog=create_configure_dialog(radio,0);
  }
  return TRUE;
}

static void create_visual(RADIO *r) {
  r->visual=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(r->visual),TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(r->visual),FALSE);
  gtk_grid_set_row_spacing(GTK_GRID(r->visual),5);
  gtk_grid_set_column_spacing(GTK_GRID(r->visual),5);


  int row=0;
  int col=0;

  if(r->can_transmit) {
    gtk_grid_attach(GTK_GRID(r->visual),r->transmitter->panadapter,col,row,1,5);
    col++;
    row=0;

    r->mox_button=gtk_toggle_button_new_with_label("MOX");
    gtk_widget_set_name(r->mox_button,"transmit-warning");
    //gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(r->mox_button)),"circular");
    g_signal_connect(r->mox_button,"toggled",G_CALLBACK(mox_cb),(gpointer)r);
    gtk_grid_attach(GTK_GRID(r->visual),r->mox_button,col,row,1,1);
    row++;

    r->vox_button=gtk_toggle_button_new_with_label("VOX");
    gtk_widget_set_name(r->vox_button,"transmit-warning");
    //gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(r->vox_button)),"circular");
    g_signal_connect(r->vox_button,"toggled",G_CALLBACK(vox_cb),(gpointer)r);
    gtk_grid_attach(GTK_GRID(r->visual),r->vox_button,col,row,1,1);
    row++;

    r->mic_level=create_mic_level(radio->transmitter);
    gtk_grid_attach(GTK_GRID(r->visual),r->mic_level,col,row,3,1);
    row++;
  
    r->mic_gain=create_mic_gain(radio->transmitter);
    gtk_grid_attach(GTK_GRID(r->visual),r->mic_gain,col,row,3,1);
    row++;

    r->drive_level=create_drive_level(radio->transmitter);
    gtk_grid_attach(GTK_GRID(r->visual),r->drive_level,col,row,3,1);

    col++;
    row=0;
  
    r->tune_button=gtk_toggle_button_new_with_label("Tune");
    gtk_widget_set_name(r->tune_button,"transmit-warning");
    //gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(r->tune_button)),"circular");
    g_signal_connect(r->tune_button,"toggled",G_CALLBACK(tune_cb),(gpointer)r);
    gtk_grid_attach(GTK_GRID(r->visual),r->tune_button,col,row,1,1);
    row++;

  }

  GtkWidget *configure=gtk_button_new_with_label("Configure");
  gtk_widget_set_name(configure,"vfo-button");
  //gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(configure)),"circular");
  g_signal_connect(configure,"clicked",G_CALLBACK(configure_cb),(gpointer)r);
  gtk_grid_attach(GTK_GRID(r->visual),configure,col,row,1,1);

  col++;
  row=0;

  if(r->discovered->supported_receivers>1) {
    add_receiver_b=gtk_button_new_with_label("Add Receiver");
    gtk_widget_set_name(add_receiver_b,"vfo-button");
    //gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(add_receiver_b)),"circular");
    g_signal_connect(add_receiver_b,"clicked",G_CALLBACK(add_receiver_cb),(gpointer)r);
    gtk_grid_attach(GTK_GRID(r->visual),add_receiver_b,col,row,1,1);
    gtk_widget_set_sensitive(add_receiver_b,r->receivers<r->discovered->supported_receivers);
    row++;
  }

#ifdef SOAPYSDR
  if(r->discovered->protocol!=PROTOCOL_SOAPYSDR) {
#endif
    add_wideband_b=gtk_button_new_with_label("Add Wideband");
    gtk_widget_set_name(add_wideband_b,"vfo-button");
    //gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(add_wideband_b)),"circular");
    g_signal_connect(add_wideband_b,"clicked",G_CALLBACK(add_wideband_cb),(gpointer)r);
    gtk_grid_attach(GTK_GRID(r->visual),add_wideband_b,col,row,1,1);
    col++;
#ifdef SOAPYSDR
  }
#endif

  row=0;

  gtk_widget_show_all(r->visual);
  
}

RADIO *create_radio(DISCOVERED *d) {
  RADIO *r;
  int i;

g_print("create_radio for %s %d\n",d->name,d->device);

  radio=g_new0(RADIO,1);
  r=radio;
  r->discovered=d;
  switch(d->device) {
    case DEVICE_METIS:
      r->model=ATLAS;
      break;
    case DEVICE_HERMES:
      r->model=ANAN_100;
      break;
    case DEVICE_HERMES2:
      r->model=HERMES_2;
      break;
    case DEVICE_ANGELIA:
      r->model=ANAN_100D;
      break;
    case DEVICE_ORION:
      r->model=ANAN_200D;
      break;
    case DEVICE_ORION2:
      r->model=ANAN_8000DLE;
      break;
    case DEVICE_HERMES_LITE:
      r->model=HERMES_LITE;
      break;
    case DEVICE_HERMES_LITE2:
      r->model=HERMES_LITE_2;
      break;      
      
      
#ifdef SOAPYSDR
    case DEVICE_SOAPYSDR:
      r->model=SOAPYSDR;
      break;
#endif
    default:
      r->model=HERMES;
      break;
  }

  switch(r->model) {
#ifdef SOAPYSDR
    case SOAPYSDR:
      r->sample_rate=r->discovered->info.soapy.sample_rate;
      r->sample_rate=768000;
      if(strcmp(r->discovered->name,"rtlsdr")==0) {
        r->sample_rate=1536000;
      }
      r->buffer_size=2048;
      if(strcmp(r->discovered->name,"lime")==0) {
        r->alex_rx_antenna=3; // LNAW
      } else {
        r->alex_rx_antenna=0; // ANT 0
      }
      r->alex_tx_antenna=0; // ANT 1
      break;
#endif
    default:
      r->sample_rate=192000;
      r->buffer_size=2048;
      r->alex_rx_antenna=0; // ANT 1
      r->alex_tx_antenna=0; // ANT 1
      break;
  }
  r->receivers=0;
  switch(d->device) {
#ifdef SOAPYSDR
    case DEVICE_SOAPYSDR:
      r->meter_calibration=-20.0;
      r->panadapter_calibration=-20.0;
      break;
#endif
    default:
      r->meter_calibration=0.0;
      r->panadapter_calibration=0.0;
      break;
  }
  
  for(i=0;i<r->receivers;i++) {
    r->receiver[i]=NULL;
  }
  r->active_receiver=NULL;
  r->transmitter=NULL;

  r->can_transmit=TRUE;
#ifdef SOAPYSDR
  if(r->discovered->protocol==PROTOCOL_SOAPYSDR) {
    r->can_transmit=r->discovered->info.soapy.tx_channels>0;
  }
#endif
  
  r->mox=FALSE;
  r->tune=FALSE;
  r->vox=FALSE;
  r->ptt=FALSE;
  r->dot=FALSE;
  r->dash=FALSE;

  r->atlas_clock_source_128mhz=FALSE;
  r->atlas_clock_source_10mhz=0;
  r->classE=FALSE;

  r->cw_keyer_internal=TRUE;
  r->cw_keyer_sidetone_frequency=650;
  r->cw_keyer_sidetone_volume=20;
  r->cw_keyer_speed=12;
  r->cw_keyer_mode=KEYER_STRAIGHT;
  r->cw_keyer_weight=30;
  r->cw_keyer_spacing=0;
  r->cw_keyer_ptt_delay=20;
  r->cw_keyer_hang_time=300;
  r->cw_keys_reversed=FALSE;
  r->cw_keys_reversed=FALSE;
  r->cw_breakin=FALSE;
  
  r->cwdaemon=FALSE;
  
  #ifdef CWDAEMON
  r->cwdaemon_running=FALSE;
  r->cwd_port = 51000;
  #endif
  
  r->display_filled=TRUE;

  r->mic_boost=FALSE;
  r->mic_ptt_enabled=FALSE;
  r->mic_bias_enabled=FALSE;
  r->mic_ptt_tip_bias_ring=FALSE;

  r->microphone_name=NULL;
  r->local_microphone=FALSE;
  r->local_microphone_buffer_size=256;
  r->local_microphone_buffer_offset=0;
  r->local_microphone_buffer=NULL;
#ifndef __APPLE__
  r->record_handle=NULL;
#endif

  g_mutex_init(&r->local_microphone_mutex);

  g_mutex_init(&r->ring_buffer_mutex);
  g_cond_init(&r->ring_buffer_cond);

  r->filter_board=ALEX;
  
  // Hermes lite 2
  r->enable_pa = TRUE;
  r->psu_clk = TRUE;

  r->adc[0].id=0;
  r->adc[0].antenna=ANTENNA_1;
  r->adc[0].filters=AUTOMATIC;
  r->adc[0].hpf=HPF_13;
  r->adc[0].lpf=LPF_30_20;
  r->adc[0].dither=FALSE;
  r->adc[0].random=FALSE;
  r->adc[0].preamp=FALSE;
  r->adc[0].attenuation=0;
  r->adc_overload = 0;
  
#ifdef SOAPYSDR
  if(r->discovered->device==DEVICE_SOAPYSDR) {
    r->adc[0].gain=20;
    r->adc[0].agc=FALSE;
    if(r->can_transmit) {
      r->dac[0].antenna=radio->discovered->info.soapy.rx_antennas-1;
      r->dac[0].gain=20;
    }
  }
#endif
  r->adc[1].id=1;
  r->adc[1].antenna=ANTENNA_1;
  r->adc[1].filters=AUTOMATIC;
  r->adc[1].hpf=HPF_9_5;
  r->adc[1].lpf=LPF_60_40;
  r->adc[1].dither=FALSE;
  r->adc[1].random=FALSE;
  r->adc[1].preamp=FALSE;
  r->adc[1].attenuation=0;
#ifdef SOAPYSDR
  if(r->discovered->device==DEVICE_SOAPYSDR) {
    r->adc[1].gain=20;
    r->adc[1].agc=FALSE;
    if(r->can_transmit) {
      r->dac[0].antenna=radio->discovered->info.soapy.rx_antennas-1;
      r->dac[1].gain=20;
    }
  }
#endif

  r->wideband=NULL;

  r->vox_enabled=FALSE;
  r->vox_threshold=0.10;
  r->vox_hang=250;

  r->region=REGION_OTHER;


  #ifdef SOAPYSDR
  if(r->discovered->device==DEVICE_SOAPYSDR) {
    r->iqswap=TRUE;
  } else {
#endif
    r->iqswap=FALSE;
#ifdef SOAPYSDR
  }
#endif

  r->which_audio=USE_SOUNDIO;
  r->which_audio_backend=0;

  r->swr_alarm_value = 2.0;
  r->temperature_alarm_value = 42;  
  r->midi_enabled = FALSE;
  
  r->dialog=NULL;


  radio_restore_state(r);

  radio_change_region(r);

#ifdef SOAPYSDR
  if(r->discovered->protocol==PROTOCOL_SOAPYSDR) {
    soapy_protocol_init(r,0);
  }
#endif

  create_audio(r->which_audio_backend,r->which_audio==USE_SOUNDIO?audio_get_backend_name(r->which_audio_backend):NULL);

  add_receivers(r);

  switch(r->discovered->protocol) {
    case PROTOCOL_1:
    case PROTOCOL_2:
      add_transmitter(r);
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      if(r->discovered->info.soapy.tx_channels>0) {
        add_transmitter(r);
      }
      break;
#endif
  }


  create_visual(r);

  switch(r->discovered->protocol) {
    case PROTOCOL_1:
      protocol1_init(r);
      break;
    case PROTOCOL_2:
      protocol2_init(r);
      break;
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      soapy_protocol_set_rx_antenna(radio->receiver[0],radio->adc[0].antenna);
      for(int i=0;i<radio->discovered->info.soapy.rx_gains;i++) {
        soapy_protocol_set_gain(&radio->adc[0]);
      } 
      RECEIVER *rx=r->receiver[0];
      double f=(double)(rx->frequency_a-rx->lo_a+rx->error_a);
      soapy_protocol_set_rx_frequency(radio->receiver[0]);
      soapy_protocol_set_automatic_gain(radio->receiver[0],radio->adc[0].agc);
      for(int i=0;i<radio->discovered->info.soapy.rx_gains;i++) {
        soapy_protocol_set_gain(&radio->adc[0]);
      } 
      soapy_protocol_start_receiver(rx);
      if(r->can_transmit) {
        if(r->transmitter!=NULL && r->transmitter->rx==rx) {
          soapy_protocol_set_tx_antenna(r->transmitter,radio->dac[0].antenna);
          soapy_protocol_set_tx_frequency(r->transmitter);
        }
      }
      if(radio->can_transmit) {
        soapy_protocol_set_tx_gain(&radio->dac[0]);
      }
      break;
#endif
  }

#ifdef SOAPYSDR
  soapy_protocol_set_mic_sample_rate(r->sample_rate);
#endif
  /*
  if(radio->local_microphone) {
    if(audio_open_input(r)!=0) {
      radio->local_microphone=FALSE;
    }
  }
  */
  
  //
  // MIDIstartup must not be called before the radio is completely set up, since
  // then MIDI can asynchronously trigger actions which require the radio already
  // running. So this is the last thing we do when starting the radio.
  //
#ifdef MIDI
//  if(r->midi_enabled) {
//    r->midi_enabled=(MIDIstartup(r->midi_filename)==0);
//  }
  if(r->midi_enabled && midi_device_name!=NULL) {
    if(register_midi_device(midi_device_name)<0) {
      r->midi_enabled=false;
    }
  } else {
    r->midi_enabled=false;
  }
#endif  
  
  g_idle_add(radio_start,(gpointer)r);


  return r;
}

void update_radio(RADIO *r) {
  // update MOX button
  g_signal_handlers_block_by_func(r->mox_button,G_CALLBACK(mox_cb),r);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->mox_button),r->mox);
  g_signal_handlers_unblock_by_func(r->mox_button,G_CALLBACK(mox_cb),r);

  // update TUNE button
  g_signal_handlers_block_by_func(r->tune_button,G_CALLBACK(tune_cb),r);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->tune_button),r->tune);
  g_signal_handlers_unblock_by_func(r->tune_button,G_CALLBACK(tune_cb),r);

  // update VOX button
  g_signal_handlers_block_by_func(r->vox_button,G_CALLBACK(vox_cb),r);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->vox_button),r->vox_enabled);
  g_signal_handlers_unblock_by_func(r->vox_button,G_CALLBACK(vox_cb),r);
}

