/* Copyright (C)
* 2018 - Claudio Girardi, IN3OTD/DK1CG
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wdsp.h>

#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#endif

#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"

static void enable_cb(GtkWidget *widget, gpointer data) {
  RADIO *radio=(RADIO *)data;
  radio->classE=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
  transmitter_enable_eer(radio->transmitter,radio->classE);
}

static void amiq_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  transmitter_set_eer_mode_amiq(tx,gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)));
}

static void delays_en_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  transmitter_enable_eer_delays(tx,gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget)));
}

static void eer_pgain_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  transmitter_set_eer_pgain(tx,gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
}

static void eer_pdelay_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  transmitter_set_eer_pdelay(tx,gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
}

static void eer_mgain_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  transmitter_set_eer_mgain(tx,gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
}

static void eer_mdelay_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  transmitter_set_eer_mdelay(tx,gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
}

static void eer_pwm_max_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->eer_pwm_max=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void eer_pwm_min_cb(GtkWidget *widget, gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->eer_pwm_min=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

GtkWidget *create_eer_dialog(RADIO *r) {
  TRANSMITTER *tx=(TRANSMITTER *)r->transmitter;

  GtkWidget *eer_frame=gtk_frame_new("EER");
  GtkWidget *eer_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(eer_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(eer_grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(eer_grid),5);
  gtk_grid_set_row_spacing(GTK_GRID(eer_grid),5);
  gtk_container_add(GTK_CONTAINER(eer_frame),eer_grid);

  GtkWidget *enable_b=gtk_check_button_new_with_label("Transmit in EER mode");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (enable_b), tx->eer);
  g_signal_connect(enable_b,"toggled",G_CALLBACK(enable_cb),r);
  gtk_grid_attach(GTK_GRID(eer_grid),enable_b,0,0,1,1);

  GtkWidget *amiq_b=gtk_check_button_new_with_label("Amplitude Modulate IQ");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (amiq_b), tx->eer_amiq);
  g_signal_connect(amiq_b,"toggled",G_CALLBACK(amiq_cb),tx);
  gtk_grid_attach(GTK_GRID(eer_grid),amiq_b,0,1,1,1);

  GtkWidget *delays_en_b=gtk_check_button_new_with_label("Use Delays");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (delays_en_b), tx->eer_enable_delays);
  g_signal_connect(delays_en_b,"toggled",G_CALLBACK(delays_en_cb),tx);
  gtk_grid_attach(GTK_GRID(eer_grid),delays_en_b,0,2,1,1);

  GtkWidget *eer_mgain_label=gtk_label_new("Env Gain");
  gtk_widget_show(eer_mgain_label);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_mgain_label,0,3,1,1);
  GtkWidget *eer_mgain=gtk_spin_button_new_with_range(0,1,0.001);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(eer_mgain),tx->eer_mgain);
  gtk_widget_show(eer_mgain);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_mgain,1,3,1,1);
  g_signal_connect(eer_mgain,"value-changed",G_CALLBACK(eer_mgain_cb),tx);

  GtkWidget *eer_pdelay_label=gtk_label_new("Env Delay (us)");
  gtk_widget_show(eer_pdelay_label);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_pdelay_label,0,4,1,1);
  GtkWidget *eer_pdelay=gtk_spin_button_new_with_range(0,1000,0.01);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(eer_pdelay),tx->eer_pdelay);
  gtk_widget_show(eer_pdelay);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_pdelay,1,4,1,1);
  g_signal_connect(eer_pdelay,"value-changed",G_CALLBACK(eer_pdelay_cb),tx);

  GtkWidget *eer_pgain_label=gtk_label_new("Phase Gain");
  gtk_widget_show(eer_pgain_label);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_pgain_label,0,5,1,1);
  GtkWidget *eer_pgain=gtk_spin_button_new_with_range(0,1,0.001);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(eer_pgain),tx->eer_pgain);
  gtk_widget_show(eer_pgain);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_pgain,1,5,1,1);
  g_signal_connect(eer_pgain,"value-changed",G_CALLBACK(eer_pgain_cb),tx);

  GtkWidget *eer_mdelay_label=gtk_label_new("Phase Delay (us)");
  gtk_widget_show(eer_mdelay_label);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_mdelay_label,0,6,1,1);
  GtkWidget *eer_mdelay=gtk_spin_button_new_with_range(0,1000,0.01);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(eer_mdelay),tx->eer_mdelay);
  gtk_widget_show(eer_mdelay);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_mdelay,1,6,1,1);
  g_signal_connect(eer_mdelay,"value-changed",G_CALLBACK(eer_mdelay_cb),tx);

  GtkWidget *eer_pwm_frame=gtk_frame_new("PWM Control");
  GtkWidget *eer_pwm_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(eer_pwm_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(eer_pwm_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(eer_pwm_frame),eer_pwm_grid);
  gtk_grid_attach(GTK_GRID(eer_grid),eer_pwm_frame,2,0,1,1);

  GtkWidget *eer_pwm_max_label=gtk_label_new("Maximum (0 - 1023)");
  gtk_widget_show(eer_pwm_max_label);
  gtk_grid_attach(GTK_GRID(eer_pwm_grid),eer_pwm_max_label,1,0,1,1);
  GtkWidget *eer_pwm_max=gtk_spin_button_new_with_range(0,1023,1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(eer_pwm_max),tx->eer_pwm_max);
  gtk_widget_show(eer_pwm_max);
  gtk_grid_attach(GTK_GRID(eer_pwm_grid),eer_pwm_max,2,0,1,1);
  g_signal_connect(eer_pwm_max,"value-changed",G_CALLBACK(eer_pwm_max_cb),tx);
  GtkWidget *eer_pwm_min_label=gtk_label_new("Minimum (0 - 1023)");
  gtk_widget_show(eer_pwm_min_label);
  gtk_grid_attach(GTK_GRID(eer_pwm_grid),eer_pwm_min_label,1,1,1,1);
  GtkWidget *eer_pwm_min=gtk_spin_button_new_with_range(0,1023,1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(eer_pwm_min),tx->eer_pwm_min);
  gtk_widget_show(eer_pwm_min);
  gtk_grid_attach(GTK_GRID(eer_pwm_grid),eer_pwm_min,2,1,1,1);
  g_signal_connect(eer_pwm_min,"value-changed",G_CALLBACK(eer_pwm_min_cb),tx);

  return eer_frame;
}
