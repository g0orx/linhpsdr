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

#ifndef RADIO_H
#define RADIO_H

#define MAX_RECEIVERS 8

#define MAX_BUFFER_SIZE 2048

#define TRANSMITTER_CHANNEL 8
#define WIDEBAND_CHANNEL 9

enum {
  ANAN_10=0,
  ANAN_10E,
  ANAN_100,
  ANAN_100D,
  ANAN_200D,
  ANAN_7000DLE,
  ANAN_8000DLE,
  ATLAS,
  HERMES,
  HERMES_2,
  ANGELIA,
  ORION_1,
  ORION_2,
  HERMES_LITE
#ifdef SOAPYSDR
  ,SOAPYSDR_USB
#endif
};

enum {
  ALEX=0,
  APOLLO,
  NONE
};

enum {
  KEYER_STRAIGHT=0,
  KEYER_MODE_A,
  KEYER_MODE_B
};


enum {
  REGION_OTHER=0,
  REGION_UK
};

typedef struct _radio {
  DISCOVERED *discovered;
  gboolean can_transmit;
  gint model;
  gint sample_rate;
  gint buffer_size;
  gint receivers;
  RECEIVER *receiver[MAX_RECEIVERS];
  TRANSMITTER *transmitter;
  RECEIVER *active_receiver;
  gint alex_rx_antenna;
  gint alex_tx_antenna;
  gdouble meter_calibration;
  gdouble panadapter_calibration;
  
  gint cw_keyer_sidetone_frequency;
  gint cw_keyer_sidetone_volume;
  gboolean cw_keys_reversed;
  gint cw_keyer_speed;
  gint cw_keyer_mode;
  gint cw_keyer_weight;
  gint cw_keyer_spacing;
  gint cw_keyer_internal;
  gint cw_keyer_ptt_delay;
  gint cw_keyer_hang_time;
  gboolean cw_breakin;

  gboolean local_microphone;
  gchar *microphone_name;

  struct SoundIoDevice *input_device;
  struct SoundIoInStream *input_stream;
  struct SoundIoRingBuffer *ring_buffer;
  gboolean input_started;
  GMutex ring_buffer_mutex;
  GCond ring_buffer_cond;

#ifndef __APPLE__
  pa_simple* microphone_stream;
#endif

  gint local_microphone_buffer_size;
  gint local_microphone_buffer_offset;
  float *local_microphone_buffer;
  GMutex local_microphone_mutex;

  gboolean mic_boost;
  gboolean mic_ptt_enabled;
  gboolean mic_bias_enabled;
  gboolean mic_ptt_tip_bias_ring;

  gboolean mic_linein;
  gint linein_gain;

  gboolean mox;
  gboolean tune;
  gboolean vox;
  gboolean ptt;
  gboolean dot;
  gboolean dash;
  gboolean local_ptt;

  gboolean adc_overload;
  gboolean IO1;
  gboolean IO2;
  gboolean IO3;
  gint AIN3;
  gint AIN4;
  gint AIN6;

  gboolean pll_locked;
  gint supply_volts;

  gint mercury_software_version;
  gint penelope_software_version;
  gint ozy_software_version;

  gboolean atlas_mic_source;
  gint atlas_clock_source_10mhz;
  gboolean atlas_clock_source_128mhz;

  gboolean classE;

  guchar oc_tune;

  gint OCfull_tune_time;
  gint OCmemory_tune_time;
  long long tune_timeout;

  gint filter_board;

  gboolean display_filled;

  GtkWidget *visual;
  GtkWidget *mox_button;
  GtkWidget *vox_button;
  GtkWidget *tune_button;
  GtkWidget *mic_level;
  cairo_surface_t *mic_level_surface;
  GtkWidget *mic_gain;
  GtkWidget *drive_level;

  GtkWidget *dialog;

  ADC adc[2];
  DAC dac[2];

  WIDEBAND *wideband;

  gboolean vox_enabled;
  double vox_threshold;
  double vox_hang;
  double vox_peak;
  guint vox_timeout;

  int region;

  gboolean duplex;

  gboolean iqswap;

  gint which_audio;
  gint which_audio_backend;

} RADIO;

extern int radio_start(void *data);
extern gboolean isTransmitting(RADIO *r);
extern RADIO *create_radio(DISCOVERED *d);
extern void delete_receiver(RECEIVER *rx);
extern void frequency_changed(RECEIVER *rx);
extern void add_receivers(RADIO *r);
extern void add_transmitter(RADIO *r);
extern void radio_save_state(RADIO *radio);
extern void radio_restore_state(RADIO *radio);
extern void delete_wideband(WIDEBAND *w);
extern void vox_changed(RADIO *r);
extern void ptt_changed(RADIO *r);
extern gboolean radio_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
extern void set_mox(RADIO *r,gboolean state);
extern void radio_change_region(RADIO *r);
extern void radio_change_audio(RADIO *r,int selected);
extern void radio_change_audio_backend(RADIO *r,int selected);
#endif
