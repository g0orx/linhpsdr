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

#include "wideband.h"
#include "wideband_waterfall.h"

static int colorLowR=0; // black
static int colorLowG=0;
static int colorLowB=0;

static int colorHighR=255; // yellow
static int colorHighG=255;
static int colorHighB=0;

static gboolean resize_timeout(void *data) {
  WIDEBAND *w=(WIDEBAND *)data;

  w->waterfall_width=w->waterfall_resize_width;
  w->waterfall_height=w->waterfall_resize_height;

  if(w->waterfall_pixbuf) {
    g_object_unref((gpointer)w->waterfall_pixbuf);
    w->waterfall_pixbuf=NULL;
  }

  if(w->waterfall!=NULL) {
    w->waterfall_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w->waterfall_width, w->waterfall_height);
    guchar *pixels = gdk_pixbuf_get_pixels (w->waterfall_pixbuf);
    memset(pixels, 0, w->waterfall_width*w->waterfall_height*3);
  }
  w->waterfall_frequency=0;
  w->waterfall_sample_rate=0;
  w->waterfall_resize_timer=-1;
  return FALSE;
}

static gboolean waterfall_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  gint width=gtk_widget_get_allocated_width (widget);
  gint height=gtk_widget_get_allocated_height (widget);
  if(width!=w->waterfall_width || height!=w->waterfall_height) {
    w->waterfall_resize_width=width;
    w->waterfall_resize_height=height;
    if(w->waterfall_resize_timer!=-1) {
      g_source_remove(w->waterfall_resize_timer);
    }
    w->waterfall_resize_timer=g_timeout_add(250,resize_timeout,(gpointer)w);
  }
  return TRUE;
}


static gboolean waterfall_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
  if(w->waterfall_pixbuf) {
    gdk_cairo_set_source_pixbuf (cr, w->waterfall_pixbuf, 0, 0);
    cairo_paint (cr);
  }
  return FALSE;
}

GtkWidget *create_wideband_waterfall(WIDEBAND *w) {
  GtkWidget *waterfall;

  w->waterfall_width=0;
  w->waterfall_height=0;
  w->waterfall_resize_timer=-1;
  w->waterfall_pixbuf=NULL;

  waterfall = gtk_drawing_area_new ();
  //gtk_widget_set_size_request (waterfall, w->width, w->height/3);

  g_signal_connect(waterfall,"configure-event",G_CALLBACK (waterfall_configure_event_cb),(gpointer)w);
  g_signal_connect(waterfall,"draw",G_CALLBACK (waterfall_draw_cb),(gpointer)w);

  //g_signal_connect(waterfall,"motion-notify-event",G_CALLBACK(receiver_motion_notify_event_cb),w);
  //g_signal_connect(waterfall,"button-press-event",G_CALLBACK(receiver_button_press_event_cb),w);
  //g_signal_connect(waterfall,"button-release-event",G_CALLBACK(receiver_button_release_event_cb),w);
  //g_signal_connect(waterfall,"scroll_event",G_CALLBACK(receiver_scroll_event_cb),w);

  //gtk_widget_set_events (waterfall, gtk_widget_get_events (waterfall)
  //                   | GDK_BUTTON_PRESS_MASK
  //                   | GDK_BUTTON_RELEASE_MASK
  //                   | GDK_BUTTON1_MOTION_MASK
  //                   | GDK_SCROLL_MASK
  //                   | GDK_POINTER_MOTION_MASK
  //                   | GDK_POINTER_MOTION_HINT_MASK);

  return waterfall;
}

void update_wideband_waterfall(WIDEBAND *w) {
  int i;

  float *samples;
  if(w->waterfall_pixbuf && w->waterfall_height>1) {
    guchar *pixels = gdk_pixbuf_get_pixels (w->waterfall_pixbuf);

    int width=gdk_pixbuf_get_width(w->waterfall_pixbuf);
    int height=gdk_pixbuf_get_height(w->waterfall_pixbuf);
    int rowstride=gdk_pixbuf_get_rowstride(w->waterfall_pixbuf);

    //memset(pixels, 0, width*height*3);

    memmove(&pixels[rowstride],pixels,(height-1)*rowstride);

    float sample;
    int average=0;
    guchar *p;
    p=pixels;
    samples=w->pixel_samples;
    for(i=0;i<w->pixels;i++) {
            sample=samples[i+w->pixels];
            if(i>0 || i<(w->pixels-1)) {
                average+=(int)sample;
            }
            if(sample<(float)w->waterfall_low) {
                *p++=colorLowR;
                *p++=colorLowG;
                *p++=colorLowB;
            } else if(sample>(float)w->waterfall_high) {
                *p++=colorHighR;
                *p++=colorHighG;
                *p++=colorHighB;
            } else {
                float range=(float)w->waterfall_high-(float)w->waterfall_low;
                float offset=sample-(float)w->waterfall_low;
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


    if(w->waterfall_automatic) {
      w->waterfall_low=average/(width-2);
      w->waterfall_high=w->waterfall_low+50;
    }

    gtk_widget_queue_draw (w->waterfall);
  }
}
