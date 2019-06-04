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
#include <wdsp.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#include "agc.h"
#include "mode.h"
#include "filter.h"
#include "band.h"
#include "receiver.h"
#include "rx_panadapter.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "vfo.h"


static gboolean resize_timeout(void *data) {
  RECEIVER *rx=(RECEIVER *)data;

  g_mutex_lock(&rx->mutex);
  rx->panadapter_width=rx->panadapter_resize_width;
  rx->panadapter_height=rx->panadapter_resize_height;
  rx->pixels=rx->panadapter_width*rx->zoom;

  receiver_init_analyzer(rx);

  if (rx->panadapter_surface) {
    cairo_surface_destroy (rx->panadapter_surface);
    rx->panadapter_surface=NULL;
  }

  if(rx->panadapter!=NULL) {
    rx->panadapter_surface = gdk_window_create_similar_surface (gtk_widget_get_window (rx->panadapter),
                                       CAIRO_CONTENT_COLOR,
                                       rx->panadapter_width,
                                       rx->panadapter_height);

    /* Initialize the surface to black */
    cairo_t *cr;
    cr = cairo_create (rx->panadapter_surface);

    if(rx->panadapter_gradient) {
      cairo_pattern_t *pat=cairo_pattern_create_linear(0.0,0.0,0.0,rx->panadapter_height);
      cairo_pattern_add_color_stop_rgba(pat,1.0,0.1,0.1,0.1,0.5);
      cairo_pattern_add_color_stop_rgba(pat,0.0,0.5,0.5,0.5,0.5);
      cairo_rectangle(cr, 0,0,rx->panadapter_width,rx->panadapter_height);
      cairo_set_source (cr, pat);
      cairo_fill(cr);
      cairo_pattern_destroy(pat);
    } else {
      cairo_set_source_rgb (cr, 0.2, 0.2, 0.2);
    //cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
      cairo_paint (cr);
    }
    cairo_destroy(cr);
  }
  rx->panadapter_resize_timer=-1;
  update_vfo(rx);
  g_mutex_unlock(&rx->mutex);
  return FALSE;
}

static gboolean rx_panadapter_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  gint width=gtk_widget_get_allocated_width (widget);
  gint height=gtk_widget_get_allocated_height (widget);
  if(width!=rx->panadapter_width || height!=rx->panadapter_height) {
    rx->panadapter_resize_width=width;
    rx->panadapter_resize_height=height;
    if(rx->panadapter_resize_timer!=-1) {
      g_source_remove(rx->panadapter_resize_timer);
    }
    rx->panadapter_resize_timer=g_timeout_add(250,resize_timeout,(gpointer)rx);
  }
  return TRUE;
}


static gboolean rx_panadapter_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(rx->panadapter_surface!=NULL) {
    cairo_set_source_surface (cr, rx->panadapter_surface, 0.0, 0.0);
    cairo_paint (cr);
  }
  return TRUE;
}

GtkWidget *create_rx_panadapter(RECEIVER *rx) {
  GtkWidget *panadapter;

  rx->panadapter_width=0;
  rx->panadapter_height=0;
  rx->panadapter_surface=NULL;
  rx->panadapter_resize_timer=-1;

  panadapter = gtk_drawing_area_new ();

  g_signal_connect(panadapter,"configure-event",G_CALLBACK(rx_panadapter_configure_event_cb),(gpointer)rx);
  g_signal_connect(panadapter,"draw",G_CALLBACK(rx_panadapter_draw_cb),(gpointer)rx);

  g_signal_connect(panadapter,"motion-notify-event",G_CALLBACK(receiver_motion_notify_event_cb),rx);
  g_signal_connect(panadapter,"button-press-event",G_CALLBACK(receiver_button_press_event_cb),rx);
  g_signal_connect(panadapter,"button-release-event",G_CALLBACK(receiver_button_release_event_cb),rx);
  g_signal_connect(panadapter,"scroll_event",G_CALLBACK(receiver_scroll_event_cb),rx);

  gtk_widget_set_events (panadapter, gtk_widget_get_events (panadapter)
                     | GDK_BUTTON_PRESS_MASK
                     | GDK_BUTTON_RELEASE_MASK
                     | GDK_BUTTON1_MOTION_MASK
                     | GDK_SCROLL_MASK
                     | GDK_POINTER_MOTION_MASK
                     | GDK_POINTER_MOTION_HINT_MASK);

  return panadapter;
}

void update_rx_panadapter(RECEIVER *rx) {
  int i;
  int x1,x2;
  int result;
  float *samples;
  float saved_max;
  float saved_min;
  cairo_text_extents_t extents;
  char temp[32];

  int display_width=gtk_widget_get_allocated_width (rx->panadapter);
  int display_height=gtk_widget_get_allocated_height (rx->panadapter);


  if(rx->panadapter_surface==NULL) {
    return;
  }

#ifdef FREEDV
  if(rx->freedv) {
    display_height=display_height-20;
  }
#endif

  if(display_height<=1) return;
    
  samples=rx->pixel_samples;

  //clear_panadater_surface();
  cairo_t *cr;
  cr = cairo_create (rx->panadapter_surface);
  cairo_select_font_face(cr, "FreeMono", CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 12);
  cairo_set_line_width(cr, 1.0);

  if(rx->panadapter_gradient) {
    cairo_pattern_t *pat=cairo_pattern_create_linear(0.0,0.0,0.0,rx->panadapter_height);
    cairo_pattern_add_color_stop_rgba(pat,1.0,0.1,0.1,0.1,0.5);
    cairo_pattern_add_color_stop_rgba(pat,0.0,0.5,0.5,0.5,0.5);
    cairo_rectangle(cr, 0,0,rx->panadapter_width,rx->panadapter_height);
    cairo_set_source (cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
  } else {

    cairo_set_source_rgb (cr, 0.2, 0.2, 0.2);
    //cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_rectangle(cr,0,0,display_width,display_height);
    cairo_fill(cr);
  }


  long long frequency=rx->frequency_a;
  long long half=(long long)rx->sample_rate/2LL;
  long long min_display=frequency-(half/(long long)rx->zoom);
  long long max_display=frequency+(half/(long long)rx->zoom);
  BAND *band=band_get_band(rx->band_a);

  if(rx->band_a==band60) {
    for(i=0;i<channel_entries;i++) {
      long long low_freq=band_channels_60m[i].frequency-(band_channels_60m[i].width/(long long)2);
      long long hi_freq=band_channels_60m[i].frequency+(band_channels_60m[i].width/(long long)2);
      x1=(low_freq-min_display)/(long long)rx->hz_per_pixel;
      x2=(hi_freq-min_display)/(long long)rx->hz_per_pixel;
      cairo_set_source_rgb (cr, 0.6, 0.3, 0.3);
      cairo_rectangle(cr, x1, 0.0, x2-x1, (double)display_height);
      cairo_fill(cr);
/*
      cairo_set_source_rgba (cr, 0.5, 1.0, 0.0, 1.0);
      cairo_move_to(cr,(double)x1,0.0);
      cairo_line_to(cr,(double)x1,(double)display_height);
      cairo_stroke(cr);
      cairo_move_to(cr,(double)x2,0.0);
      cairo_line_to(cr,(double)x2,(double)display_height);
      cairo_stroke(cr);
*/
    }
  }

  // filter
  //cairo_set_source_rgba (cr, 0.25, 0.25, 0.25, 0.75);
  cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.75);
  double filter_left=(double)display_width/2.0+(((double)rx->filter_low+rx->ctun_offset)/rx->hz_per_pixel);
  double filter_right=(double)display_width/2.0+(((double)rx->filter_high+rx->ctun_offset)/rx->hz_per_pixel);
  cairo_rectangle(cr, filter_left, 0.0, filter_right-filter_left, (double)display_height);
  cairo_fill(cr);

  if(rx->mode_a==CWU || rx->mode_a==CWL) {
    cairo_set_source_rgb (cr, 1.0, 1.0, 0.0);
    double cw_frequency=filter_left+((filter_right-filter_left)/2.0);
    cairo_move_to(cr,cw_frequency,10.0);
    cairo_line_to(cr,cw_frequency,(double)display_height);
    cairo_stroke(cr);
  }

  // plot the levels
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

  double dbm_per_line=(double)display_height/((double)rx->panadapter_high-(double)rx->panadapter_low);

  for(i=rx->panadapter_high;i>=rx->panadapter_low;i--) {
    int mod=abs(i)%20;
    if(mod==0) {
      double y = (double)(rx->panadapter_high-i)*dbm_per_line;
      cairo_move_to(cr,0.0,y);
      cairo_line_to(cr,(double)display_width,y);

      sprintf(temp,"%d dBm",i);
      cairo_move_to(cr, 1, y-1);  
      cairo_show_text(cr, temp);
    }
  }
  cairo_stroke(cr);

  // plot frequency markers
  long long f1;
  long long f2;
  long long divisor1=20000;
  long long divisor2=5000;
  switch(rx->sample_rate) {
    case 48000:
      switch(rx->zoom) {
        case 1:
        case 3:
          divisor1=5000LL;
          divisor2=1000LL;
          break;
        case 5:
          divisor1=1000LL;
          divisor2=500LL;
          break;
        case 7:
          divisor1=1000LL;
          divisor2=200LL;
          break;
      }
      break;
    case 96000:
    case 100000:
      switch(rx->zoom) {
        case 1:
          divisor1=10000LL;
          divisor2=5000LL;
          break;
        case 3:
        case 5:
        case 7:
          divisor1=2000LL;
          divisor2=1000LL;
          break;
      }
      break;
    case 192000:
      switch(rx->zoom) {
        case 1:
          divisor1=20000LL;
          divisor2=5000LL;
          break;
        case 3:
          divisor1=10000LL;
          divisor2=5000LL;
          break;
        case 5:
        case 7:
          divisor1=5000LL;
          divisor2=1000LL;
          break;
      }
      break;
    case 384000:
      switch(rx->zoom) {
        case 1:
          divisor1=50000LL;
          divisor2=10000LL;
          break;
        case 3:
          divisor1=20000LL;
          divisor2=5000LL;
          break;
        case 5:
          divisor1=10000LL;
          divisor2=5000LL;
          break;
        case 7:
          divisor1=10000LL;
          divisor2=2000LL;
          break;
      }
      break;
    case 512000:
    case 524288:
    case 768000:
      switch(rx->zoom) {
        case 1:
          divisor1=50000LL;
          divisor2=25000LL;
          break;
        case 3:
          divisor1=50000LL;
          divisor2=10000LL;
          break;
        case 5:
          divisor1=50000LL;
          divisor2=10000LL;
          break;
        case 7:
          divisor1=20000LL;
          divisor2=10000LL;
          break;
      }
      break;
    case 1048576:
    case 1536000:
    case 2048000:
      switch(rx->zoom) {
        case 1: 
          divisor1=100000LL;
          divisor2=50000LL;
          break;
        case 3: 
          divisor1=100000LL;
          divisor2=20000LL;
          break;
        case 5: 
          divisor1=50000LL;
          divisor2=10000LL;
          break;
        case 7: 
          divisor1=20000LL;
          divisor2=5000LL;
          break;
      }
      break;
  }
  int offset=((rx->zoom-1)/2)*rx->panadapter_width;
  cairo_set_line_width(cr, 1.0);

  f1=frequency-half+(long long)(rx->hz_per_pixel*offset);
  f2=(f1/divisor2)*divisor2;

  int x=0;
  do {
    x=(int)(f2-f1)/rx->hz_per_pixel;
    if(x>70) {
      if((f2%divisor1)==0LL) {
        cairo_set_source_rgb (cr, 1.0, 1.0, 0.0);
        cairo_move_to(cr,(double)x,10.0);
        cairo_line_to(cr,(double)x,(double)display_height);
        cairo_stroke(cr);
        cairo_select_font_face(cr, "FreeMono",
                            CAIRO_FONT_SLANT_NORMAL,
                            CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12);
        sprintf(temp,"%0lld.%03lld",f2/1000000,(f2%1000000)/1000);
        cairo_text_extents(cr, temp, &extents);
        cairo_move_to(cr, (double)x-(extents.width/2.0), 10.0);
        cairo_show_text(cr, temp);
      } else if((f2%divisor2)==00LL) {
        cairo_set_source_rgb (cr, 0.60, 0.60, 0.0);
        cairo_move_to(cr,(double)x,10.0);
        cairo_line_to(cr,(double)x,(double)display_height);
        cairo_stroke(cr);
      }
    }
    f2=f2+divisor2;
  } while(x<display_width);
  

  if(rx->band_a!=band60) {
    // band edges
    if(band->frequencyMin!=0LL) {
      cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
      cairo_set_line_width(cr, 2.0);
      if((min_display<band->frequencyMin)&&(max_display>band->frequencyMin)) {
        i=(int)(((double)band->frequencyMin-(double)min_display)/rx->hz_per_pixel);
        cairo_move_to(cr,(double)i,0.0);
        cairo_line_to(cr,(double)i,(double)display_height);
        cairo_stroke(cr);
      }
      if((min_display<band->frequencyMax)&&(max_display>band->frequencyMax)) {
        i=(int)(((double)band->frequencyMax-(double)min_display)/rx->hz_per_pixel);
        cairo_move_to(cr,(double)i,0.0);
        cairo_line_to(cr,(double)i,(double)display_height);
        cairo_stroke(cr);
      }
      cairo_set_line_width(cr, 1.0);
    }
  }

  double attenuation=radio->adc[rx->adc].attenuation;
  if(radio->discovered->device==DEVICE_HERMES_LITE) {
    if(!radio->adc[rx->adc].dither) {
      attenuation-=20;
    }
  }
            
  // agc
  if(rx->agc!=AGC_OFF) {
    double hang=0.0;
    double thresh=0.0;
    double x=80.0;

    GetRXAAGCHangLevel(rx->channel, &hang);
    GetRXAAGCThresh(rx->channel, &thresh, 4096.0, (double)rx->sample_rate);
    
    if(rx->panadapter_agc_line) {
      double knee_y=thresh+attenuation+radio->panadapter_calibration;
      knee_y = floor((rx->panadapter_high - knee_y)*dbm_per_line);

      double hang_y=hang+attenuation+radio->panadapter_calibration;
      hang_y = floor((rx->panadapter_high - hang_y)*dbm_per_line);

      if(rx->agc!=AGC_MEDIUM && rx->agc!=AGC_FAST) {
        cairo_set_source_rgb (cr, 1.0, 1.0, 0.0);
        cairo_move_to(cr,x,hang_y-8.0);
        cairo_rectangle(cr, x, hang_y-8.0,8.0,8.0);
        cairo_fill(cr);
        cairo_move_to(cr,x,hang_y);
        cairo_line_to(cr,(double)display_width-x,hang_y);
        cairo_stroke(cr);
        cairo_move_to(cr,x+8.0,hang_y);
        cairo_show_text(cr, "-H");
      }

      cairo_set_source_rgb (cr, 0.0, 1.0, 0.0);
      cairo_move_to(cr,x,knee_y-8.0);
      cairo_rectangle(cr, x, knee_y-8.0,8.0,8.0);
      cairo_fill(cr);
      cairo_move_to(cr,x,knee_y);
      cairo_line_to(cr,(double)display_width-x,knee_y);
      cairo_stroke(cr);
      cairo_move_to(cr,x+8.0,knee_y);
      cairo_show_text(cr, "-G");      
    }
  }


  // cursor
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
  cairo_move_to(cr,(double)(display_width/2.0)+(rx->ctun_offset/rx->hz_per_pixel),0.0);
  cairo_line_to(cr,(double)(display_width/2.0)+(rx->ctun_offset/rx->hz_per_pixel),(double)display_height);
  cairo_stroke(cr);

  // signal
  double s2;
  
  samples[display_width-1+offset]=-200;
  cairo_move_to(cr, 0.0, display_height);

  for(i=1;i<display_width;i++) {
    s2=(double)samples[i+offset]+attenuation+radio->panadapter_calibration;
    s2 = floor((rx->panadapter_high - s2) *dbm_per_line);
    cairo_line_to(cr, (double)i, s2);
  }

  if(rx->panadapter_filled) {
    cairo_close_path (cr);
//#ifdef GRADIANT
    cairo_pattern_t *pat=cairo_pattern_create_linear(0.0,0.0,0.0,rx->panadapter_height);
    cairo_pattern_add_color_stop_rgba(pat,0.0,0.0,0.0,1.0,0.5);
    cairo_pattern_add_color_stop_rgba(pat,1.0,1.0,1.0,1.0,0.5);
    cairo_set_source (cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);
//#else
//    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0,0.5);
//    cairo_fill_preserve (cr);
//#endif
  }
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_stroke(cr);

#ifdef FREEDV
  if(rx->freedv) {
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_rectangle(cr,0,display_height,display_width,display_height+20);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    cairo_set_font_size(cr, 18);
    cairo_move_to(cr, 0.0, (double)display_height+20.0-2.0);
    cairo_show_text(cr, rx->freedv_text_data);
  }
#endif

#ifdef GPIO
  if(active) {
    cairo_set_source_rgb(cr,1.0,1.0,0.0);
    cairo_set_font_size(cr,16);
    if(ENABLE_E1_ENCODER) {
      cairo_move_to(cr, display_width-150,30);
      cairo_show_text(cr, encoder_string[e1_encoder_action]);
    }

    if(ENABLE_E2_ENCODER) {
      cairo_move_to(cr, display_width-150,50);
      cairo_show_text(cr, encoder_string[e2_encoder_action]);
    }

    if(ENABLE_E3_ENCODER) {
      cairo_move_to(cr, display_width-150,70);
      cairo_show_text(cr, encoder_string[e3_encoder_action]);
    }
  }
#endif

  cairo_destroy (cr);
  gtk_widget_queue_draw (rx->panadapter);

}
