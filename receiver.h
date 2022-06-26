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

#ifndef RECEIVER_H
#define RECEIVER_H


#include <soundio/soundio.h>
#ifndef __APPLE__
#include <pulse/simple.h>
#include <alsa/asoundlib.h>
#endif

typedef enum {SPLIT_OFF, SPLIT_ON, SPLIT_SAT, SPLIT_RSAT} split_type;

typedef struct _receiver {
  
  gint channel; // WDSP channel

  gint adc;

  gint sample_rate;
  gint buffer_size;
  gint dsp_rate;
  gint output_rate;

  gint fft_size;
  gboolean low_latency;

  gdouble *buffer;

  guint32 iq_sequence;
  gdouble *iq_input_buffer;
  guint32 audio_sequence;
  gdouble *audio_output_buffer;
  gint audio_buffer_size;
  guchar *audio_buffer;
  gfloat *pixel_samples;

  gint update_timer_id;

  gint samples;
  gint output_samples;
  gint pixels;
  gint fps;
  gdouble display_average_time;
  
  gboolean ctun;
  gint64 ctun_frequency;
  gint64 ctun_offset;
  gint64 ctun_min;
  gint64 ctun_max;

  gint64 frequency_min;
  gint64 frequency_max;

  gboolean qo100_beacon;

  gboolean entering_frequency;
  gint64 entered_frequency;

  gint64 frequency_a;
  gint64 lo_a;
  gint64 error_a;
  gint band_a;
  gint mode_a;
  gint filter_a;
  gint64 offset;
  gint bandstack;

  gint64 lo_tx;
  gint64 error_tx;
  gboolean tx_track_rx;

  gint64 frequency_b;
  gint64 lo_b;
  gint64 error_b;
  gint band_b;
  gint mode_b;
  gint filter_b;

  split_type split;
  gboolean mute_while_transmitting;
  gboolean duplex;

  gint filter_low_a;
  gint filter_high_a;
  gint deviation;

  gint filter_low_b;
  gint filter_high_b;

  gint agc;
  gdouble agc_gain;
  gdouble agc_slope;
  gdouble agc_hang_threshold;

  gboolean rit_enabled;
  gint64 rit;
  gint64 rit_step;

  gint64 step;

  gboolean locked;

  gdouble volume;

  gboolean nb;
  gboolean nb2;
  gboolean nr;
  gboolean nr2;
  gboolean anf;
  gboolean snb;

  gint nr_agc;
  gint nr2_gain_method;
  gint nr2_npe_method;
  gint nr2_ae;


  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *vfo;
  cairo_surface_t *vfo_surface;
  GtkWidget *meter;
  cairo_surface_t *meter_surface;
  GtkWidget *radio_info;
  cairo_surface_t *radio_info_surface;  
  
  gint vfo_a_x;
  gint vfo_a_digits;
  gint vfo_a_width;

  gint vfo_b_x;
  gint vfo_b_digits;
  gint vfo_b_width;

  GtkWidget *bookmark_dialog;

  gint smeter;
  double meter_db;

  gint window_x;
  gint window_y;
  gint window_width;
  gint window_height;

  GtkWidget *vpaned;
  gint paned_position;
  double paned_percent;

  GtkWidget *panadapter;
  gint panadapter_width;
  gint panadapter_height;
  gint panadapter_resize_width;
  gint panadapter_resize_height;
  guint panadapter_resize_timer;
  cairo_surface_t *panadapter_surface;

  gint panadapter_low;
  gint panadapter_high;
  gint panadapter_step;
  gboolean panadapter_filled;
  gboolean panadapter_gradient;
  gboolean panadapter_agc_line;  

  GtkWidget *waterfall;
  gint waterfall_width;
  gint waterfall_height;
  gint waterfall_resize_width;
  gint waterfall_resize_height;
  guint waterfall_resize_timer;
  GdkPixbuf *waterfall_pixbuf;

  gint waterfall_low;
  gint waterfall_high;
  gboolean waterfall_automatic;
  gboolean waterfall_ft8_marker;
  gint64 waterfall_frequency;
  gint waterfall_sample_rate;
  
  gdouble hz_per_pixel;

  gboolean is_panning;
  gboolean has_moved;
  gint last_x;

  gint mixed_audio;
  short mixed_left_audio;
  short mixed_right_audio;

  gboolean remote_audio;
  gboolean local_audio;
  gint local_audio_buffer_size;
  gint local_audio_buffer_offset;
  void *local_audio_buffer;
  GMutex local_audio_mutex;
  gint local_audio_latency;
  gint audio_channels;
  
  gchar *audio_name;
  int output_index;
  gboolean mute_when_not_active;

  struct SoundIoDevice *output_device;
  struct SoundIoOutStream *output_stream;
  struct SoundIoRingBuffer *ring_buffer;
  gboolean output_started;

#ifndef __APPLE__
  pa_simple* playstream;
  snd_pcm_t *playback_handle;
  snd_pcm_format_t local_audio_format;  
#endif

  GtkWidget *toolbar;
  GtkWidget *dialog;
  GtkWidget *filter_frame;
  GtkWidget *filter_grid;
  GtkWidget *mode_grid;
  GtkWidget *band_grid;

  gint zoom;
  gint pan;

  gboolean enable_equalizer;
  gint equalizer[4];

  GMutex mutex;

  gboolean bpsk_enable;
  void *bpsk;

  gboolean subrx_enable;
  void *subrx;

  void *resampler;
  gdouble *resampled_buffer;
  gint resampled_buffer_size;

  GtkWidget *local_audio_b;
  GtkWidget *audio_choice_b;
  GtkWidget *tx_control_b;
  gulong audio_choice_signal_id;
  gulong local_audio_signal_id;
  gulong tx_control_signal_id;

  gint rigctl_port;
  gboolean rigctl_enable;
  gboolean cat_client_connected;

  GtkWidget *serial_port_entry;
  char rigctl_serial_port[80];
  gint rigctl_serial_baudrate;
  gboolean rigctl_serial_enable;
  

  gboolean rigctl_debug;
  void *rigctl;

} RECEIVER;


enum {
  AUDIO_STEREO = 0,
  AUDIO_LEFT_ONLY = 1,
  AUDIO_RIGHT_ONLY = 2
};

extern RECEIVER *create_receiver(int channel,int sample_rate);
extern void receiver_update_title(RECEIVER *rx);
extern void receiver_init_analyzer(RECEIVER *rx);
extern void add_iq_samples(RECEIVER *r,double left,double right);
extern gboolean receiver_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
extern gboolean receiver_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
extern gboolean receiver_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data);
extern gboolean receiver_scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data);

extern void receiver_filter_changed(RECEIVER *rx,int filter);
extern void receiver_mode_changed(RECEIVER *rx,int mode);
extern void receiver_band_changed(RECEIVER *rx,int band);
extern void receiver_xvtr_changed(RECEIVER *rx);
extern void set_filter(RECEIVER *rx,int low,int high);
extern void set_deviation(RECEIVER *rx);

extern void update_noise(RECEIVER *rx);

extern void receiver_save_state(RECEIVER *rx);
extern void receiver_change_sample_rate(RECEIVER *rx,int sample_rate);
extern void set_agc(RECEIVER *rx);
extern void calculate_display_average(RECEIVER *rx);
extern void receiver_fps_changed(RECEIVER *rx);
extern void receiver_change_zoom(RECEIVER *rx,int zoom);
extern void update_frequency(RECEIVER *rx);
extern void receiver_move(RECEIVER *rx,long long hz,gboolean round);
extern void receiver_move_b(RECEIVER *rx,long long hz,gboolean b_only,gboolean round);
extern void receiver_move_to(RECEIVER *rx,long long hz);
extern void receiver_set_volume(RECEIVER *rx);
extern void receiver_set_agc_gain(RECEIVER *rx);
extern void receiver_set_ctun(RECEIVER *rx);
extern void set_band(RECEIVER *rx,int band,int entry);
#endif
