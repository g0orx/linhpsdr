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

#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#define CTCSS_FREQUENCIES 38
extern double ctcss_frequencies[CTCSS_FREQUENCIES];

typedef struct _transmitter {
  gint channel; // WDSP channel

  gint dac;
  
  GMutex queue_mutex;  

  gint alex_antenna;
  gdouble mic_gain;
  gint linein_gain;

  gint exciter_power;
  gint alex_forward_power;
  gint alex_reverse_power;

  gint alc_meter;
  gdouble fwd;
  gdouble exciter;
  gdouble rev;
  gdouble alc;
  gdouble swr;

  GtkWidget *window;
  gint window_width;
  gint window_height;

  RECEIVER *rx;

  gdouble drive;
  gdouble tune_percent;
  gboolean tune_use_drive;
  gint attenuation;

  gboolean eer;
  gint eer_amiq;
  gdouble eer_pgain;
  gdouble eer_pdelay;
  gdouble eer_mgain;
  gdouble eer_mdelay;
  gboolean eer_enable_delays;
  gint eer_pwm_min;
  gint eer_pwm_max;

  gboolean use_rx_filter;
  gint filter_low;
  gint filter_high;

  gint actual_filter_low;
  gint actual_filter_high;

  gint fft_size;
  gboolean low_latency;

  gint buffer_size;
  gint mic_samples;
  gdouble *mic_input_buffer;
  gdouble *iq_output_buffer;
  //
  guint packet_counter;
  
  gfloat *inI, *inQ, *outMI, *outMQ; // for EER
  gint mic_sample_rate;
  gint mic_dsp_rate;
  gint iq_output_rate;
  gint p1_packet_size;
  gint output_samples;
  
  gdouble temperature;

  gboolean pre_emphasize;
  gboolean enable_equalizer;
  gint equalizer[4];
  gboolean leveler;

  gboolean ctcss_enabled;
  gint ctcss;
  gdouble tone_level;

  gdouble deviation;
  gboolean compressor;
  gdouble compressor_level;

  gdouble am_carrier_level;

  gint fps;
  gint pixels;
  gfloat *pixel_samples;
  gint update_timer_id;

  gint panadapter_low;
  gint panadapter_high;

  gint panadapter_width;
  gint panadapter_height;
  GtkWidget *panadapter;
  cairo_surface_t *panadapter_surface;

  GtkWidget *dialog;

  gboolean puresignal;
  gboolean ps_twotone;
  gboolean ps_feedback;
  gboolean ps_auto;
  gboolean ps_single;
  GtkWidget *ps;
  cairo_surface_t *ps_surface;
  gint ps_timer_id;

  GtkWidget *local_audio_b;
  GtkWidget *audio_choice_b;
  GtkWidget *tx_control_b;
  gulong audio_choice_signal_id;
  gulong local_audio_signal_id;
  gulong tx_control_signal_id;

  GtkWidget *local_microphone_b;
  GtkWidget *microphone_choice_b;
  gulong microphone_choice_signal_id;
  gulong local_microphone_signal_id;

  gboolean xit_enabled;
  gint64 xit;
  gint64 xit_step;

  gboolean updated;

} TRANSMITTER;

extern TRANSMITTER *create_transmitter(int channel);
extern void transmitter_init_analyzer(TRANSMITTER *tx);
extern void transmitter_save_state(TRANSMITTER *tx);
extern void transmitter_restore_state(TRANSMITTER *tx);
extern void add_mic_sample(TRANSMITTER *tx,float sample);
extern void transmitter_set_filter(TRANSMITTER *tx,int low,int high);
extern void transmitter_set_pre_emphasize(TRANSMITTER *tx,int state);
extern void transmitter_set_mode(TRANSMITTER *tx,int mode);
extern void transmitter_set_deviation(TRANSMITTER *tx);
extern void transmitter_set_am_carrier_level(TRANSMITTER *tx);
extern void transmitter_fps_changed(TRANSMITTER *tx);
extern void transmitter_set_ctcss(TRANSMITTER *tx,gboolean run,int f);

extern void transmitter_set_ps(TRANSMITTER *tx,gboolean state);
extern void transmitter_set_twotone(TRANSMITTER *tx,gboolean state);
extern void transmitter_set_ps_sample_rate(TRANSMITTER *tx,int rate);

extern void QueueInit(void);
extern void full_tx_buffer(TRANSMITTER *tx);

extern void transmitter_enable_eer(TRANSMITTER *tx,gboolean state);
extern void transmitter_set_eer_mode_amiq(TRANSMITTER *tx,gboolean state);
extern void transmitter_enable_eer_delays(TRANSMITTER *tx,gboolean state);
extern void transmitter_set_eer_pgain(TRANSMITTER *tx,gdouble gain);
extern void transmitter_set_eer_pdelay(TRANSMITTER *tx,gdouble delay);
extern void transmitter_set_eer_mgain(TRANSMITTER *tx,gdouble gain);
extern void transmitter_set_eer_mdelay(TRANSMITTER *tx,gdouble delay);

#endif
