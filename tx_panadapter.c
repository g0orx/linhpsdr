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
#include <stdlib.h>

#include "mode.h"
#include "discovered.h"
#include "bpsk.h"
#include "adc.h"
#include "dac.h"
#include "wideband.h"
#include "receiver.h"
#include "transmitter.h"
#include "radio.h"
#include "vfo.h"
//#include "transmitter_dialog.h"
#include "configure_dialog.h"
#include "main.h"
#ifdef SOAPYSDR
#include "soapy_protocol.h"
#endif

static gboolean transmitter_button_press_event_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  switch(event->button) {
    case 1: // left
      break;
    case 3: // right
      //if(tx->dialog==NULL) {
      if(radio->dialog==NULL) {
        radio->dialog=create_configure_dialog(radio,rx_base+radio->receivers);
      }
      break;
  }
  return TRUE;
}

static gboolean tx_panadapter_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  tx->panadapter_width=gtk_widget_get_allocated_width (widget);
  tx->panadapter_height=gtk_widget_get_allocated_height (widget);
  if(tx->panadapter_width>1 && tx->panadapter_height>1) {
  if(tx->panadapter_surface) {
    cairo_surface_destroy(tx->panadapter_surface);
  }
  g_print("tx_panadapter_configure_event: width=%d height=%d\n",tx->panadapter_width,tx->panadapter_height);
  if(radio->discovered->protocol==PROTOCOL_1) {
    tx->pixels=tx->panadapter_width*3;
  } else {
    tx->pixels=tx->panadapter_width*12;
  }
  if(tx->panadapter!=NULL) {
    tx->panadapter_surface = gdk_window_create_similar_surface (gtk_widget_get_window (tx->panadapter),
                                       CAIRO_CONTENT_COLOR,
                                       tx->panadapter_width,
                                       tx->panadapter_height);
    cairo_t *cr;
    cr = cairo_create (tx->panadapter_surface);
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_paint (cr);
    cairo_destroy(cr);
    transmitter_init_analyzer(tx);
  }
  }
  return TRUE;
}

static gboolean tx_panadapter_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  TRANSMITTER *tx=(TRANSMITTER *)data;
  if(tx->panadapter_surface!=NULL) {
    cairo_set_source_surface (cr, tx->panadapter_surface, 0.0, 0.0);
    cairo_paint (cr);
  }
  return TRUE;
}

GtkWidget *create_tx_panadapter(TRANSMITTER *tx) {

  tx->panadapter_width=0;
  tx->panadapter_height=0;
  tx->panadapter_surface=NULL;
  tx->panadapter=gtk_drawing_area_new();
  gtk_widget_set_size_request(tx->panadapter, 300, 150);

  g_signal_connect(tx->panadapter,"configure-event",G_CALLBACK(tx_panadapter_configure_event_cb),(gpointer)tx);
  g_signal_connect(tx->panadapter,"draw",G_CALLBACK(tx_panadapter_draw_cb),(gpointer)tx);
  g_signal_connect(tx->panadapter,"button-press-event",G_CALLBACK(transmitter_button_press_event_cb),(gpointer)tx);

  gtk_widget_set_events (tx->panadapter, gtk_widget_get_events (tx->panadapter)
                     | GDK_BUTTON_PRESS_MASK);

  return tx->panadapter;
}

void update_tx_panadapter(RADIO *r) {
  TRANSMITTER *tx=r->transmitter;
  int width=gtk_widget_get_allocated_width(tx->panadapter);
  int height=gtk_widget_get_allocated_height(tx->panadapter);
  float *samples=tx->pixel_samples;
  double hz_per_pixel=(double)tx->iq_output_rate/(double)tx->pixels;
  char text[32];
  int i;

  if(tx->panadapter_surface!=NULL) {
    cairo_t *cr;
    cr = cairo_create (tx->panadapter_surface);
    cairo_set_line_width(cr, 1.0);

    SetColour(cr, BACKGROUND);
    //cairo_pattern_t *pat=cairo_pattern_create_linear(0.0,0.0,0.0,height);
    //cairo_pattern_add_color_stop_rgba(pat,1.0,0.1,0.1,0.1,0.5);
    //cairo_pattern_add_color_stop_rgba(pat,0.0,0.5,0.5,0.5,0.5);
    cairo_rectangle(cr, 0,0,width,height);
    //cairo_set_source (cr, pat);
    cairo_fill(cr);
    //cairo_pattern_destroy(pat);

    double dbm_per_line=(double)height/((double)tx->panadapter_high-(double)tx->panadapter_low);

    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_select_font_face(cr, "Noto Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);
    char v[32];

    // filter
    cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.75);
    double filter_left=(double)width/2.0+((double)tx->actual_filter_low/hz_per_pixel);
    double filter_right=(double)width/2.0+((double)tx->actual_filter_high/hz_per_pixel);
    cairo_rectangle(cr, filter_left, 20.0, filter_right-filter_left, (double)height-20.0);
    cairo_fill(cr);

    // levels
    for(i=tx->panadapter_high;i>=tx->panadapter_low;i--) {
      int mod=abs(i)%20;
      if(mod==0) {
        double y = (double)(tx->panadapter_high-i)*dbm_per_line;
        cairo_move_to(cr,0.0,y);
        cairo_line_to(cr,(double)width,y);
  
        sprintf(v,"%d dBm",i);
        cairo_move_to(cr, 1, y-1);
        cairo_show_text(cr, v);
      }
    }
    cairo_stroke(cr);

    // cursor
    SetColour(cr, TEXT_A);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr,(double)(width/2.0),20.0);
    cairo_line_to(cr,(double)(width/2.0),(double)height);
    cairo_stroke(cr);

    if(tx->rx->mode_a==CWU || tx->rx->mode_a==CWL) {
      SetColour(cr, TEXT_B);
      double cw_frequency=filter_left+((filter_right-filter_left)/2.0);
      cairo_move_to(cr,cw_frequency,20.0);
      cairo_line_to(cr,cw_frequency,(double)height);
      cairo_stroke(cr);
    }
    
    // signal
    if(isTransmitting(radio)) {
/*
      int offset=tx->pixels/3;
      if(radio->discovered->protocol==PROTOCOL_2) {
        offset=(tx->pixels/24)*11;
      }
*/
      int offset=(tx->pixels/2)-(width/2);
      double s1,s2;
      samples[offset]=-200.0;
      samples[(offset+width)-1]=-200.0;

      s1=(double)samples[offset];
      s1 = floor((tx->panadapter_high - s1)
                            * (double)height
                            / (tx->panadapter_high - tx->panadapter_low));
      cairo_move_to(cr, 0.0, s1);
      for(i=1;i<width;i++) {
        s2=(double)samples[i+offset];
        s2 = floor((tx->panadapter_high - s2)
                                * (double)height
                                / (tx->panadapter_high - tx->panadapter_low));
        cairo_line_to(cr, (double)i, s2);
      }

      /*
      if(radio->display_filled) {
        cairo_close_path (cr);
        cairo_pattern_t *pat=cairo_pattern_create_linear(0.0,0.0,0.0,height);
        //cairo_pattern_add_color_stop_rgba(pat,1.0,0.1,0.0,0.0,0.5);
        //cairo_pattern_add_color_stop_rgba(pat,0.0,0.5,0.0,0.0,0.5);
        
        cairo_set_source (cr, pat);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(pat);
      }
      */
      
      SetColour(cr, BACKGROUND);
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      cairo_set_line_width(cr, 1.0);
      cairo_stroke(cr);
      
      cairo_set_font_size(cr, 12); 
      
      SetColour(cr, TEXT_C);
      sprintf(text,"%.1f W",tx->fwd);
      cairo_move_to(cr, 206, 34);
      cairo_show_text(cr, text);
  
      // Won't show SWR if power out is less than
      // 100 mW, potentially improve this in the future?
      if (tx->fwd > 1E-1) {
         double this_swr;
         double reflection_coefficient = sqrt(tx->rev/tx->fwd);
         if (1 - reflection_coefficient == 0.0) {
           this_swr = 9999.9;
         }
         else {
           this_swr = (1 + reflection_coefficient) / (1 - reflection_coefficient);
         }
        if (this_swr < 0.0) this_swr=1.0;
        
        // Exponential moving average filter
        double alpha = 0.7;
        tx->swr = (alpha * this_swr) + (1 - alpha) * tx->swr;
        
        sprintf(text,"SWR: %1.1f:1", tx->swr);
        cairo_move_to(cr, 206, 56);
        cairo_show_text(cr, text);
      }
  
      sprintf(text,"ALC: %2.1f dB",tx->alc);
      cairo_move_to(cr, 206, 80);
      cairo_show_text(cr, text);
    }

    // frequency
    if(tx->rx!=NULL) {
      long long f=tx->rx->frequency_a+tx->rx->ctun_offset-tx->rx->lo_tx;
      if(tx->rx->split) {
        f=tx->rx->frequency_b;
      }
      char temp[32];
      sprintf(temp,"%5lld.%03lld.%03lld",f/(long long)1000000,(f%(long long)1000000)/(long long)1000,f%(long long)1000);
      if(isTransmitting(radio)) {
        SetColour(cr, WARNING);
      } else {
        SetColour(cr, TEXT_C);
      }
      cairo_set_font_size(cr, 20);
      cairo_move_to(cr,((double)width/2.0)+2.0,18.0);
      cairo_show_text(cr, temp);
    }
    
    if(radio->discovered->device==DEVICE_HERMES_LITE2) {   
      cairo_set_font_size(cr, 12);       
      SetColour(cr, TEXT_C);
      sprintf(text,"%2.0f degC",tx->temperature);
      cairo_move_to(cr, 220, height-8);
      cairo_show_text(cr, text);
    }
#ifdef SOAPYSDR
    if(radio->discovered->protocol==PROTOCOL_SOAPYSDR) {   
      if(radio->discovered->info.soapy.has_temp) {
        cairo_set_font_size(cr, 12);       
        SetColour(cr, TEXT_C);
        int y=height-40;
        for (size_t i = 0; i < radio->discovered->info.soapy.sensors; i++) {
          if(strstr(radio->discovered->info.soapy.sensor[i],"temp")!=NULL) {
            char *value=soapy_protocol_read_sensor(radio->discovered->info.soapy.sensor[i]);
            int v=(int)atof(value);
            if(strcmp(radio->discovered->info.soapy.sensor[i],"xadc_temp0")==0) {
              sprintf(text,"zynq = %dC",v);
            } else if(strcmp(radio->discovered->info.soapy.sensor[i],"ad9361-phy_temp0")==0) {
              sprintf(text,"pluto = %dC",v);
            } else {
              sprintf(text,"%s = %dC",radio->discovered->info.soapy.sensor[i],v);;
            }
            cairo_move_to(cr, width-(width/4), y);
            cairo_show_text(cr, text);
            y+=15;
          } else if(strcmp(radio->discovered->info.soapy.sensor[i],"adm1177_voltage0")==0) {
            char *value=soapy_protocol_read_sensor(radio->discovered->info.soapy.sensor[i]);
            double v=atof(value);
            sprintf(text,"volts = %0.1fv",v);
            cairo_move_to(cr, width-(width/4), y);
            cairo_show_text(cr, text);
            y+=15;
          }
        }
      }
    }
#endif

    cairo_stroke(cr);    
    cairo_destroy(cr);
    gtk_widget_queue_draw(tx->panadapter);

    tx->updated=TRUE;
  }
}
