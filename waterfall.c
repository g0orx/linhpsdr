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
#include <string.h>

#include "discovered.h"
#include "bpsk.h"
#include "adc.h"
#include "dac.h"
#include "wideband.h"
#include "receiver.h"
#include "transmitter.h"
#include "radio.h"
#include "waterfall.h"
#include "main.h"

static int colorLowR=0; // black
static int colorLowG=0;
static int colorLowB=0;

static int colorHighR=255; // yellow
static int colorHighG=255;
static int colorHighB=0;

static gboolean resize_timeout(void *data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->waterfall_width=rx->waterfall_resize_width;
  rx->waterfall_height=rx->waterfall_resize_height;
  if(rx->waterfall_pixbuf) {
    g_object_unref((gpointer)rx->waterfall_pixbuf);
    rx->waterfall_pixbuf=NULL;
  }
  if(rx->waterfall!=NULL) {
    rx->waterfall_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, rx->waterfall_width, rx->waterfall_height);
    guchar *pixels = gdk_pixbuf_get_pixels (rx->waterfall_pixbuf);
    memset(pixels, 0, rx->waterfall_width*rx->waterfall_height*3);
  }
  rx->waterfall_frequency=0;
  rx->waterfall_sample_rate=0;
  rx->waterfall_resize_timer=-1;
  return FALSE;
}

static gboolean waterfall_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  gint width=gtk_widget_get_allocated_width (widget);
  gint height=gtk_widget_get_allocated_height (widget);
  if(width!=rx->waterfall_width || height!=rx->waterfall_height) {
    rx->waterfall_resize_width=width;
    rx->waterfall_resize_height=height;
    if(rx->waterfall_resize_timer!=-1) {
      g_source_remove(rx->waterfall_resize_timer);
    }
    rx->waterfall_resize_timer=g_timeout_add(250,resize_timeout,(gpointer)rx);
  }
  return TRUE;
}

static gboolean waterfall_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(rx->waterfall_pixbuf) {
    gdk_cairo_set_source_pixbuf (cr, rx->waterfall_pixbuf, 0, 0);
    cairo_paint (cr);
  }
  return FALSE;
}


GtkWidget *create_waterfall(RECEIVER *rx) {
  GtkWidget *waterfall;

  rx->waterfall_width=0;
  rx->waterfall_height=0;
  rx->waterfall_resize_timer=-1;
  rx->waterfall_pixbuf=NULL;

  waterfall = gtk_drawing_area_new ();
  //gtk_widget_set_size_request (waterfall, rx->width, rx->height/3);

  g_signal_connect(waterfall,"configure-event",G_CALLBACK (waterfall_configure_event_cb),(gpointer)rx);
  g_signal_connect(waterfall,"draw",G_CALLBACK (waterfall_draw_cb),(gpointer)rx);

  g_signal_connect(waterfall,"motion-notify-event",G_CALLBACK(receiver_motion_notify_event_cb),rx);
  g_signal_connect(waterfall,"button-press-event",G_CALLBACK(receiver_button_press_event_cb),rx);
  g_signal_connect(waterfall,"button-release-event",G_CALLBACK(receiver_button_release_event_cb),rx);
  g_signal_connect(waterfall,"scroll_event",G_CALLBACK(receiver_scroll_event_cb),rx);

  gtk_widget_set_events (waterfall, gtk_widget_get_events (waterfall)
                     | GDK_BUTTON_PRESS_MASK
                     | GDK_BUTTON_RELEASE_MASK
                     | GDK_BUTTON1_MOTION_MASK
                     | GDK_SCROLL_MASK
                     | GDK_POINTER_MOTION_MASK
                     | GDK_POINTER_MOTION_HINT_MASK);

  return waterfall;
}

void update_waterfall(RECEIVER *rx) {
  int i;
  float *samples;

  if(rx->waterfall_pixbuf && rx->waterfall_height>1) {
    guchar *pixels = gdk_pixbuf_get_pixels (rx->waterfall_pixbuf);

    int width=gdk_pixbuf_get_width(rx->waterfall_pixbuf);
    int height=gdk_pixbuf_get_height(rx->waterfall_pixbuf);
    int rowstride=gdk_pixbuf_get_rowstride(rx->waterfall_pixbuf);
    
    if(rx->waterfall_frequency!=0 && (rx->sample_rate==rx->waterfall_sample_rate)) {
      if(rx->waterfall_frequency!=rx->frequency_a) {
        // scrolled or band change
        long long half=((long long)(rx->sample_rate/2))/(rx->zoom);      
        if(rx->waterfall_frequency<(rx->frequency_a-half) || rx->waterfall_frequency>(rx->frequency_a+half)) {
          // outside of the range - blank waterfall
          memset(pixels, 0, width*height*3);
        } else {
          // rotate waterfall
          gint64 diff=rx->waterfall_frequency-rx->frequency_a;
          int rotate_pixels=(int)((double)diff/rx->hz_per_pixel);
          if(rotate_pixels<0) {
            memmove(pixels,&pixels[-rotate_pixels*3],((width*height)+rotate_pixels)*3);
            //now clear the right hand side
            for(i=0;i<height;i++) {
              memset(&pixels[((i*width)+(width+rotate_pixels))*3], 0, -rotate_pixels*3);
            }
          } else {
            memmove(&pixels[rotate_pixels*3],pixels,((width*height)-rotate_pixels)*3);
            //now clear the left hand side
            for(i=0;i<height;i++) {
              memset(&pixels[(i*width)*3], 0, rotate_pixels*3);
            }
          }
        }
      }
    } else {
      memset(pixels, 0, width*height*3);
    }

    rx->waterfall_frequency=rx->frequency_a;
    rx->waterfall_sample_rate=rx->sample_rate;

    memmove(&pixels[rowstride],pixels,(height-1)*rowstride);

    float sample;
    int average=0;
    guchar *p;
    p=pixels;
    samples=rx->pixel_samples;
    //int offset=((rx->zoom-1)/2)*rx->panadapter_width;
    int offset=rx->pan;
    for(i=0;i<width;i++) {
            sample=samples[i+offset]+radio->adc[rx->adc].attenuation;
            if(i>1 || i<(width-1)) {
              average+=(int)sample;
            }
            if(sample<(float)rx->waterfall_low) {
                *p++=colorLowR;
                *p++=colorLowG;
                *p++=colorLowB;
            } else if(sample>(float)rx->waterfall_high) {
                *p++=colorHighR;
                *p++=colorHighG;
                *p++=colorHighB;
            } else {
                float range=(float)rx->waterfall_high-(float)rx->waterfall_low;
                float offset=sample-(float)rx->waterfall_low;
                float percent=offset/range;
                if(percent<(2.0f/9.0f)) {
                    float local_percent = percent / (2.0f/9.0f);
                    *p++ = (int)((1.0f-local_percent)*colorLowR);
                    *p++ = (int)((1.0f-local_percent)*colorLowG);
                    *p++ = (int)(colorLowB + local_percent*(255-colorLowB));
                } else if(percent<(3.0f/9.0f)) {
                    float local_percent = (percent - 2.0f/9.0f) / (1.0f/9.0f);
                    *p++ = 0;
                    *p++ = (int)(local_percent*255);
                    *p++ = (char)255;
                } else if(percent<(4.0f/9.0f)) {
                     float local_percent = (percent - 3.0f/9.0f) / (1.0f/9.0f);
                     *p++ = 0;
                     *p++ = (char)255;
                     *p++ = (int)((1.0f-local_percent)*255);
                } else if(percent<(5.0f/9.0f)) {
                     float local_percent = (percent - 4.0f/9.0f) / (1.0f/9.0f);
                     *p++ = (int)(local_percent*255);
                     *p++ = (char)255;
                     *p++ = 0;
                } else if(percent<(7.0f/9.0f)) {
                     float local_percent = (percent - 5.0f/9.0f) / (2.0f/9.0f);
                     *p++ = (char)255;
                     *p++ = (int)((1.0f-local_percent)*255);
                     *p++ = 0;
                } else if(percent<(8.0f/9.0f)) {
                     float local_percent = (percent - 7.0f/9.0f) / (1.0f/9.0f);
                     *p++ = (char)255;
                     *p++ = 0;
                     *p++ = (int)(local_percent*255);
                } else {
                     float local_percent = (percent - 8.0f/9.0f) / (1.0f/9.0f);
                     *p++ = (int)((0.75f + 0.25f*(1.0f-local_percent))*255.0f);
                     *p++ = (int)(local_percent*255.0f*0.5f);
                     *p++ = (char)255;
                }
            }
    }

    if(rx->waterfall_ft8_marker) {
        static int tim0=0;
        int tim=time(NULL);
        if(tim%15==0) {
          if(tim0==0) {
            p=pixels;
            for(i=0;i<width;i++) {
              *p++=(char)255;
              *p++=0;
              *p++=0;
            }
            tim0=1;
          }
        } else {
          tim0=0;
        }
    } 

    if(rx->waterfall_automatic) {
      rx->waterfall_low=(average/(width-2))-14;
      rx->waterfall_high=rx->waterfall_low+80;
    }

    gtk_widget_queue_draw (rx->waterfall);
  }
}
