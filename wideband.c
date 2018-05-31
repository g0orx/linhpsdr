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
#include "wideband_panadapter.h"
#include "wideband_waterfall.h"
#include "waterfall.h"
#include "protocol1.h"
#include "protocol2.h"
#include "receiver_dialog.h"
#ifdef TOOLBAR
#include "receiver_toolbar.h"
#endif
#include "property.h"

void wideband_save_state(WIDEBAND *w) {
  char name[80];
  char value[80];
  gint x;
  gint y;
  gint width;
  gint height;

  sprintf(name,"wideband.channel");
  sprintf(value,"%d",w->channel);
  setProperty(name,value);
  sprintf(name,"wideband.adc");
  sprintf(value,"%d",w->adc);
  setProperty(name,value);
  sprintf(name,"wideband.buffer_size");
  sprintf(value,"%d",w->buffer_size);
  setProperty(name,value);
  sprintf(name,"wideband.fft_size");
  sprintf(value,"%d",w->fft_size);
  setProperty(name,value);
  sprintf(name,"wideband.fps");
  sprintf(value,"%d",w->fps);
  setProperty(name,value);

  gtk_window_get_position(GTK_WINDOW(w->window),&x,&y);
  sprintf(name,"wideband.x");
  sprintf(value,"%d",x);
  setProperty(name,value);
  sprintf(name,"wideband.y");
  sprintf(value,"%d",y);
  setProperty(name,value);

  gtk_window_get_size(GTK_WINDOW(w->window),&width,&height);
  sprintf(name,"wideband.width");
  sprintf(value,"%d",width);
  setProperty(name,value);
  sprintf(name,"wideband.height");
  sprintf(value,"%d",height);
  setProperty(name,value);
  
}

void wideband_restore_state(WIDEBAND *w) {
  char name[80];
  char *value;

  sprintf(name,"wideband.channel");
  value=getProperty(name);
  if(value) w->channel=atoi(value);
  sprintf(name,"wideband.adc");
  value=getProperty(name);
  if(value) w->adc=atoi(value);

}

static gboolean window_delete(GtkWidget *widget,GdkEvent *event, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  g_source_remove(w->update_timer_id);
  delete_wideband(w);
  return FALSE;
}

static void focus_in_event_cb(GtkWindow *window,GdkEventFocus *event,gpointer data) {
}

gboolean wideband_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  int x=(int)event->x;
  switch(event->button) {
    case 1: // left button
      break;
    case 3: // right button
      break;
  }
  return TRUE;
}

gboolean wideband_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data) {
  gint64 hz;
  WIDEBAND *w=(WIDEBAND *)data;
  int x=(int)event->x;
  switch(event->button) {
    case 1: // left button
      break;
    case 3: // right button
      break;
  }
  return TRUE;
}

gboolean wideband_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  gint x,y;
  GdkModifierType state;
  WIDEBAND *w=(WIDEBAND *)data;
  gdk_window_get_device_position(event->window,event->device,&x,&y,&state);
  if((state & GDK_BUTTON1_MASK) == GDK_BUTTON1_MASK) {
  }
  return TRUE;
}

gboolean wideband_scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  if(event->direction==GDK_SCROLL_UP) {
  } else {
  }
  return TRUE;
}

static gboolean update_timer_cb(void *data) {
  int rc;
  WIDEBAND *w=(WIDEBAND *)data;

  if(w->panadapter_resize_timer==-1) {
    GetPixels(w->channel,0,w->pixel_samples,&rc);
    if(rc) {
      update_wideband_panadapter(w);
      update_wideband_waterfall(w);
    }
  }
  return TRUE;
}
 
static void full_wideband_buffer(WIDEBAND *w) {
  Spectrum0(1, w->channel, 0, 0, w->input_buffer);
}

void reset_wideband_buffer_index(WIDEBAND *w) {
  if(w!=NULL) {
    w->samples=0;
  }
}

void add_wideband_sample(WIDEBAND *w,double sample) {
  w->input_buffer[w->samples*2]=sample;
  w->input_buffer[(w->samples*2)+1]=0.0;
  w->samples=w->samples+1;
  if(w->samples>=w->buffer_size) {
    full_wideband_buffer(w);
    w->samples=0;
  }
}

static gboolean receiver_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  gint width=gtk_widget_get_allocated_width(widget);
  gint height=gtk_widget_get_allocated_height(widget);
  w->window_width=width;
  w->window_height=height;
  return FALSE;
}

void wideband_update_title(WIDEBAND *w) {
  gchar title[32];
  g_snprintf((gchar *)&title,sizeof(title),"Linux HPSDR: Wideband ADC-%d",w->adc);
g_print("create_visual: %s\n",title);
  gtk_window_set_title(GTK_WINDOW(w->window),title);
}

static void create_visual(WIDEBAND *w) {

  w->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(w->window,"configure-event",G_CALLBACK(receiver_configure_event_cb),w);
  g_signal_connect(w->window,"focus_in_event",G_CALLBACK(focus_in_event_cb),w);
  g_signal_connect(w->window,"delete-event",G_CALLBACK (window_delete), w);

  wideband_update_title(w);

  gtk_widget_set_size_request(w->window,w->window_width,w->window_height);

  w->table=gtk_table_new(4,4,FALSE);

  GtkWidget *vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_table_attach(GTK_TABLE(w->table), vpaned, 0, 4, 0, 4,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

  w->panadapter=create_wideband_panadapter(w);
  gtk_paned_pack1 (GTK_PANED(vpaned), w->panadapter,TRUE,TRUE);

  w->waterfall=create_wideband_waterfall(w);
  gtk_paned_pack2 (GTK_PANED(vpaned), w->waterfall,TRUE,TRUE);

  gtk_container_add(GTK_CONTAINER(w->window),w->table);

  gtk_widget_set_events(w->window,gtk_widget_get_events(w->window)
                     | GDK_FOCUS_CHANGE_MASK
                     | GDK_BUTTON_PRESS_MASK
                     | GDK_BUTTON_RELEASE_MASK);
}

void wideband_init_analyzer(WIDEBAND *w) {
    int flp[] = {0};
    double keep_time = 0.1;
    int n_pixout=1;
    int spur_elimination_ffts = 1;
    int data_type = 1;
    int fft_size = w->fft_size;
    int window_type = 4;
    double kaiser_pi = 14.0;
    int overlap = 0; //1024; //4096;
    int clip = 0;
    int span_clip_l = 0;
    int span_clip_h = 0;
    int pixels=w->pixels;
    int stitches = 1;
    int avm = 0;
    double tau = 0.001 * 120.0;
    int calibration_data_set = 0;
    double span_min_freq = 0.0;
    double span_max_freq = 0.0;


  if(w->pixel_samples!=NULL) {
    g_free(w->pixel_samples);
    w->pixel_samples=NULL;
  }
  if(w->pixels>0) {
    w->pixel_samples=g_new0(float,w->pixels*2);
    int max_w = fft_size + (int) min(keep_time * (double) w->fps, keep_time * (double) fft_size * (double) w->fps);

    //overlap = (int)max(0.0, ceil(fft_size - (double)w->sample_rate / (double)w->fps));

    SetAnalyzer(w->channel,
            n_pixout,
            spur_elimination_ffts, //number of LO frequencies = number of ffts used in elimination
            data_type, //0 for real input data (I only); 1 for complex input data (I & Q)
            flp, //vector with one elt for each LO frequency, 1 if high-side LO, 0 otherwise
            fft_size, //size of the fft, i.e., number of input samples
            w->buffer_size, //number of samples transferred for each OpenBuffer()/CloseBuffer()
            window_type, //integer specifying which window function to use
            kaiser_pi, //PiAlpha parameter for Kaiser window
            overlap, //number of samples each fft (other than the first) is to re-use from the previous
            clip, //number of fft output bins to be clipped from EACH side of each sub-span
            span_clip_l, //number of bins to clip from low end of entire span
            span_clip_h, //number of bins to clip from high end of entire span
            pixels*2, //number of pixel values to return.  may be either <= or > number of bins
            stitches, //number of sub-spans to concatenate to form a complete span
            calibration_data_set, //identifier of which set of calibration data to use
            span_min_freq, //frequency at first pixel value8192
            span_max_freq, //frequency at last pixel value
            max_w //max samples to hold in input ring buffers
    );
  }

}


WIDEBAND *create_wideband(int channel) {
  WIDEBAND *w=g_new0(WIDEBAND,1);
  char name [80];
  char *value;
  gint x=-1;
  gint y=-1;
  gint width;
  gint height;

g_print("create_wideband: channel=%d\n",channel);
  w->channel=channel;
  w->adc=0;

  w->pixels=0;
  w->pixel_samples=NULL;
  w->sequence=0;
  w->buffer_size=16384;
  w->input_buffer=g_new0(gdouble,w->buffer_size*2);

  w->fps=10;

  w->fft_size=w->buffer_size;

  // set default location and sizes
  w->window_x=channel*30;
  w->window_y=channel*30;
  w->window_width=512;
  w->window_height=180;
  w->window=NULL;
  w->panadapter_high=0;
  w->panadapter_low=-140;

  w->waterfall_high=0;
  w->waterfall_low=-140;
  w->waterfall_automatic=TRUE;

  wideband_restore_state(w);

  int result;
  XCreateAnalyzer(w->channel, &result, 262144, 1, 1, "");
  if(result != 0) {
    g_print("XCreateAnalyzer channel=%d failed: %d\n", w->channel, result);
  } else {
    wideband_init_analyzer(w);
  }

  SetDisplayDetectorMode(w->channel, 0, DETECTOR_MODE_AVERAGE/*display_detector_mode*/);
  SetDisplayAverageMode(w->channel, 0,  AVERAGE_MODE_LOG_RECURSIVE/*display_average_mode*/);


  create_visual(w);
  if(w->window!=NULL) {
    gtk_widget_show_all(w->window);
    sprintf(name,"wideband.x");
    value=getProperty(name);
    if(value) x=atoi(value);
    sprintf(name,"wideband.y");
    value=getProperty(name);
    if(value) y=atoi(value);
    if(x!=-1 && y!=-1) {
      gtk_window_move(GTK_WINDOW(w->window),x,y);
    
      sprintf(name,"wideband.width");
      value=getProperty(name);
      if(value) width=atoi(value);
      sprintf(name,"wideband.height");
      value=getProperty(name);
      if(value) height=atoi(value);
      gtk_window_resize(GTK_WINDOW(w->window),width,height);
    }
  }

g_print("create_widband: update_timer: %d\n",1000/w->fps);
  w->update_timer_id=g_timeout_add(1000/w->fps,update_timer_cb,(gpointer)w);
  return w;
}
