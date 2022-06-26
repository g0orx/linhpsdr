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
#include <epoxy/gl.h>
#include <math.h>
#include <stdlib.h>
#include <wdsp.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#include "bpsk.h"
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
#include "subrx.h"

#define LINE_WIDTH 0.5

int signal_vertices_size=-1;
float *signal_vertices=NULL;

static const double dashed2[] = {2.0, 2.0};
static int len2  = sizeof(dashed2) / sizeof(dashed2[0]);        

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
      cairo_pattern_t *pat=cairo_pattern_create_linear(0.2, 0.2, 0.2, rx->panadapter_height);
      cairo_pattern_add_color_stop_rgba(pat,0.0, (48/255), (48/255), (48/255), 1);
      cairo_pattern_add_color_stop_rgba(pat,0.0, (80/255), (80/255), (80/255), 1);
      cairo_rectangle(cr, 0,0,rx->panadapter_width,rx->panadapter_height);
      cairo_set_source (cr, pat);
      cairo_fill(cr);
      cairo_pattern_destroy(pat);
    } else {
      cairo_set_source_rgb (cr, 0.2, 0.2, 0.2);
      cairo_paint (cr);
    }
    cairo_destroy(cr);
  }
  rx->panadapter_resize_timer=-1;
  update_vfo(rx);
  g_mutex_unlock(&rx->mutex);
  return FALSE;
}

#ifdef OPENGL

GLuint gl_program, gl_vao;

const GLchar *vert_src ="\n" \
"#version 330                                  \n" \
"#extension GL_ARB_explicit_attrib_location: enable  \n" \
"                                              \n" \
"layout(location = 0) in vec2 in_position;     \n" \
"                                              \n" \
"void main()                                   \n" \
"{                                             \n" \
"  gl_Position = ftransform;                   \n" \
"}                                             \n";

const GLchar *frag_src ="\n" \
"void main (void)                              \n" \
"{                                             \n" \
"  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);    \n" \
"}                                             \n";

static gboolean rx_panadapter_render(GtkGLArea *area, GdkGLContext *context)
{
  //g_mutex_lock(&rx->mutex);
  // inside this function it's safe to use GL; the given
  // #GdkGLContext has been made current to the drawable
  // surface used by the #GtkGLArea and the viewport has
  // already been set to be the size of the allocation

  // we can start by clearing the buffer
  glClearColor (0, 0, 0, 0);
  glClear (GL_COLOR_BUFFER_BIT);

  // draw the object
  if(signal_vertices_size!=-1) {
    glLineWidth(2.0);
    glColor3f(1.0,1.0,0.0);
    GLuint vbo;
    glGenBuffers(1, &vbo); // Generate 1 buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, signal_vertices_size*sizeof(float)*2, signal_vertices, GL_STREAM_DRAW);

    glUseProgram(gl_program);
    glBindVertexArray(gl_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDrawArrays(GL_LINE_STRIP,0,signal_vertices_size);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); //Unbind
    glBindVertexArray(0);
    glUseProgram(0);
  }

  // we completed our drawing; the draw commands will be
  // flushed at the end of the signal emission chain, and
  // the buffers will be drawn on the window
  //g_mutex_unlock(&rx->mutex);
  return TRUE;
}

static void rx_panadapter_realize (GtkGLArea *area)
{
  // We need to make the context current if we want to
  // call GL API
  gtk_gl_area_make_current (area);

  // If there were errors during the initialization or
  // when trying to make the context current, this
  // function will return a #GError for you to catch
  if (gtk_gl_area_get_error (area) != NULL)
    return;

  GLuint frag_shader, vert_shader;
  frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  vert_shader = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(frag_shader, 1, &frag_src, NULL);
  glShaderSource(vert_shader, 1, &vert_src, NULL);

  glCompileShader(frag_shader);
  glCompileShader(vert_shader);

  gl_program = glCreateProgram();
  glAttachShader(gl_program, frag_shader);
  glAttachShader(gl_program, vert_shader);
  glLinkProgram(gl_program);

  glGenVertexArrays(1, &gl_vao);
  glBindVertexArray(gl_vao);


}
#endif

void rx_panadapter_resize(GtkGLArea *area, gint width, gint height, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(width!=rx->panadapter_width || height!=rx->panadapter_height) {
    rx->panadapter_resize_width=width;
    rx->panadapter_resize_height=height;
    if(rx->panadapter_resize_timer!=-1) {
      g_source_remove(rx->panadapter_resize_timer);
    }
    rx->panadapter_resize_timer=g_timeout_add(250,resize_timeout,(gpointer)rx);
  }
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

  panadapter=NULL;
#ifdef OPENGL
  if(opengl) {
    panadapter=gtk_gl_area_new();
  }

  if(panadapter!=NULL) {
    g_print("rx_panadapter: using opengl\n");
    g_signal_connect (panadapter,"render",G_CALLBACK(rx_panadapter_render),rx);
    g_signal_connect (panadapter,"realize",G_CALLBACK(rx_panadapter_realize),rx);
    g_signal_connect (panadapter,"resize",G_CALLBACK(rx_panadapter_resize),rx);
  } else {
#endif
    panadapter = gtk_drawing_area_new ();

    g_signal_connect(panadapter,"configure-event",G_CALLBACK(rx_panadapter_configure_event_cb),(gpointer)rx);
    g_signal_connect(panadapter,"draw",G_CALLBACK(rx_panadapter_draw_cb),(gpointer)rx);

#ifdef OPENGL
  }
#endif

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

static gboolean first_time=TRUE;

void update_rx_panadapter(RECEIVER *rx,gboolean running) {
  int i;
  int x1,x2;
  float *samples;
  cairo_text_extents_t extents;
  char temp[32];

  int display_width=gtk_widget_get_allocated_width (rx->panadapter);
  int display_height=gtk_widget_get_allocated_height (rx->panadapter);
  //int offset=((rx->zoom-1)/2)*display_width;
  int offset=rx->pan;
  samples=rx->pixel_samples;
  samples[display_width-1+offset]=-200;
  double dbm_per_line=(double)display_height/((double)rx->panadapter_high-(double)rx->panadapter_low);
  double attenuation=radio->adc[rx->adc].attenuation;
  
  
  if(radio->discovered->device==DEVICE_HERMES_LITE2) {
      attenuation = attenuation * -1;
  }
  

  if(display_height<=1) return;

  if(opengl) {
    if(signal_vertices_size!=display_width) {
      if(signal_vertices!=NULL) {
        g_free(signal_vertices);
      }
      signal_vertices=g_new(float,display_width*2);
      signal_vertices_size=display_width;
    }
    float h_half=(float)display_width/2.0;
    float v_half=(float)rx->panadapter_low+(((float)rx->panadapter_high-(float)rx->panadapter_low)/2.0);
    for(i=0;i<display_width;i++) {
      float x=((float)i-h_half)/h_half;
      double s2=(double)samples[i+offset]+attenuation+radio->panadapter_calibration;
      float y=((float)s2-v_half)/v_half;
      if(y>1.0) y=1.0;
      if(y<-1.0) y=-1.0;
      signal_vertices[i*2]=x;
      signal_vertices[(i*2)+1]=y;
      if(first_time) {
        g_print("i=%d x=%f y=%f\n",i,x,y);
      }
    }
    first_time=FALSE;
    gtk_widget_queue_draw (rx->panadapter);
  } else {

    if(rx->panadapter_surface==NULL) {
      return;
    }

    //clear_panadater_surface();
    cairo_t *cr;
    cr = cairo_create (rx->panadapter_surface);
    cairo_select_font_face(cr, "Noto Sans", CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    cairo_set_line_width(cr, LINE_WIDTH);

    if(!running) {
      SetColour(cr, WARNING);
      cairo_set_font_size(cr, 18);
      cairo_move_to(cr, display_width/2, display_height/2);  
      cairo_show_text(cr, "No data - receiver thread exited");
      cairo_destroy (cr);
      gtk_widget_queue_draw (rx->panadapter);
      return;
    }

    if(rx->panadapter_gradient) {
      // Set fill      
      cairo_pattern_t *pat = cairo_pattern_create_radial((rx->panadapter_width / 2),
                             rx->panadapter_height + 300,
                             5,
                             (rx->panadapter_width / 2),
                             rx->panadapter_height + 300,
                             rx->panadapter_width/2);
      
      cairo_pattern_add_color_stop_rgba(pat, 1, 0.1, 0.1, 0.1, 1);
      cairo_pattern_add_color_stop_rgba(pat, 0, 0.25, 0.25, 0.25, 1);      
      
      cairo_rectangle(cr, 0,0,rx->panadapter_width,rx->panadapter_height);
      cairo_set_source (cr, pat);
      cairo_fill(cr);
      cairo_pattern_destroy(pat);
    } else {
      cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
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
        x1=(low_freq-min_display)/rx->hz_per_pixel;
        x2=(hi_freq-min_display)/rx->hz_per_pixel;
        cairo_set_source_rgb (cr, 0.6, 0.3, 0.3);
        cairo_rectangle(cr, x1, 0.0, x2-x1, (double)display_height);
        cairo_fill(cr);
      }
    }

    // filter
    cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.75);
    double filter_left=((double)rx->pixels/2.0)-(double)rx->pan+(((double)rx->filter_low_a+rx->ctun_offset)/rx->hz_per_pixel);
    double filter_right=((double)rx->pixels/2.0)-(double)rx->pan+(((double)rx->filter_high_a+rx->ctun_offset)/rx->hz_per_pixel);
    cairo_rectangle(cr, filter_left, 10.0, filter_right-filter_left, (double)display_height);
    cairo_fill(cr);

    double cursor;
    // draw cursor for cw mode
    if(rx->mode_a==CWU || rx->mode_a==CWL) {
      SetColour(cr, TEXT_B);
      cursor=filter_left+((filter_right-filter_left)/2.0);
      cairo_move_to(cr,cursor,10.0);
      cairo_line_to(cr,cursor,(double)display_height-20);
      cairo_stroke(cr);
    } else {
      SetColour(cr, TEXT_A);
      cursor=(double)(rx->pixels/2.0)-(double)rx->pan+(rx->ctun_offset/rx->hz_per_pixel);
      cairo_move_to(cr,cursor,10.0);
      cairo_line_to(cr,cursor,(double)display_height-20.0);
      cairo_stroke(cr);
    }

    // draw the frequency
    char temp[32];
    long long af=rx->ctun?rx->ctun_frequency:rx->frequency_a;
    sprintf(temp,"%5lld.%03lld.%03lld",af/(long long)1000000,(af%(long long)1000000)/(long long)1000,af%(long long)1000);
    cairo_set_font_size(cr, 12);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_text_extents(cr, temp, &extents);
    cairo_move_to(cr, (double)cursor-(extents.width/2.0), 10);
    cairo_show_text(cr, temp);

    
    // Show VFO B (tx) for split mode or if subrx enabled
    if(rx->split==SPLIT_ON || rx->subrx_enable) {    
      double diff = (double)(rx->frequency_b - rx->frequency_a)/rx->hz_per_pixel;       
      if(rx->mode_a==CWU || rx->mode_a==CWL) {
        SetColour(cr, WARNING);
        double cw_frequency=filter_left+((filter_right-filter_left)/2.0);
	cursor=cw_frequency+diff;
        cairo_move_to(cr,cursor,20.0);
        cairo_line_to(cr,cursor,(double)display_height-20);
        cairo_stroke(cr);
      } else if(rx->subrx_enable) {
        // VFO B cursor
        SetColour(cr, WARNING);
	cursor=(double)(rx->pixels/2.0)-(double)rx->pan+diff;
        cairo_move_to(cr,cursor,20.0);
        cairo_line_to(cr,cursor,(double)display_height-20);
        cairo_stroke(cr);

        cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 0.75);
#ifdef USE_VFO_B_MODE_AND_FILTER
        double filter_left=((double)rx->pixels/2.0)-(double)rx->pan+(((double)rx->filter_low_b)/rx->hz_per_pixel)+diff;
        double filter_right=((double)rx->pixels/2.0)-(double)rx->pan+(((double)rx->filter_high_b)/rx->hz_per_pixel)+diff;
#else
        double filter_left=((double)rx->pixels/2.0)-(double)rx->pan+(((double)rx->filter_low_a)/rx->hz_per_pixel)+diff;
        double filter_right=((double)rx->pixels/2.0)-(double)rx->pan+(((double)rx->filter_high_a)/rx->hz_per_pixel)+diff;
#endif
        cairo_rectangle(cr, filter_left, 20.0, filter_right-filter_left, (double)display_height-20);
        cairo_fill(cr);
      }

      // draw the frequency
      char temp[32];
      sprintf(temp,"%5lld.%03lld.%03lld",rx->frequency_b/(long long)1000000,(rx->frequency_b%(long long)1000000)/(long long)1000,rx->frequency_b%(long long)1000);
      cairo_set_font_size(cr, 12);
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_text_extents(cr, temp, &extents);
      cairo_move_to(cr, (double)cursor-(extents.width/2.0), 20);
      cairo_show_text(cr, temp);
    }
    
    cairo_set_line_width (cr, LINE_WIDTH);
    // plot the levels
    
    cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
    //cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_rectangle(cr,0,0,40,display_height);
    cairo_fill(cr);    
    
    cairo_set_dash(cr, dashed2, len2, 0);
    for(i=rx->panadapter_high;i>=rx->panadapter_low;i--) {
      SetColour(cr, DARK_LINES);
      int mod=abs(i)%rx->panadapter_step;
      if(mod==0) {
        double y = (double)(rx->panadapter_high-i)*dbm_per_line;
        cairo_move_to(cr,0.0,y);
        cairo_line_to(cr,(double)display_width,y);
        if(rx->panadapter_gradient) SetColour(cr, TEXT_B);
        sprintf(temp," %d",i);
        cairo_move_to(cr, 5, y-1);  
        cairo_show_text(cr, temp);
      }
    }
    cairo_stroke(cr);



    // plot frequency markers
    
    cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr,0, (rx->panadapter_height-20), display_width, (rx->panadapter_height));
    cairo_fill(cr);        

    // if zoom > 1 - show the pan position
    if(rx->zoom!=1) {
      int pan_x=(int)((double)rx->pan/(double)rx->zoom);
      int pan_width=(int)((double)rx->panadapter_width/(double)rx->zoom);
      cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
      cairo_rectangle(cr,pan_x, (rx->panadapter_height-4), pan_width, (rx->panadapter_height));
      cairo_fill(cr);        
    }

    long long f1;
    long long f2;
    long long divisor1=20000;
    long long divisor2=5000;
    long long factor=(long long)(rx->sample_rate/48000);
    if(factor>10LL) factor=10LL;
    switch(rx->zoom) {
      case 1:
      case 2:
      case 3:
        divisor1=5000LL*factor;
        divisor2=1000LL*factor;
        break;
      case 4:
      case 5:
      case 6:
        divisor1=1000LL*factor;
        divisor2=500LL*factor;
        break;
      case 7:
      case 8:
        divisor1=1000LL*factor;
        divisor2=200LL*factor;
        break;
    }
    cairo_set_line_width(cr, LINE_WIDTH);

    f1=frequency-half+(long long)(rx->hz_per_pixel*offset);
    if (rx->mode_a==CWU) {
      f1 -= radio->cw_keyer_sidetone_frequency;
    }
    else if (rx->mode_a==CWL) {
      f1 += radio->cw_keyer_sidetone_frequency;
    }    
    f2=(f1/divisor2)*divisor2;

    int x=0;
    do {
      x=(int)(f2-f1)/rx->hz_per_pixel;
      if(x>70) {
        if((f2%divisor1)==0LL) {
          SetColour(cr, DARK_LINES);
          cairo_set_dash(cr, 0, 0, 0);
          cairo_move_to(cr,(double)x,0);
          cairo_line_to(cr,(double)x,(double)display_height-20);
          SetColour(cr, TEXT_B);
          cairo_line_to(cr,(double)x,(double)display_height);
          cairo_stroke(cr);
          SetColour(cr, TEXT_B);
          cairo_select_font_face(cr, "Noto Sans",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_NORMAL);
          cairo_set_font_size(cr, 12);
          sprintf(temp,"%0lld.%03lld",f2/1000000,(f2%1000000)/1000);
          cairo_text_extents(cr, temp, &extents);
          cairo_move_to(cr, (double)x-(extents.width/2.0), (rx->panadapter_height - 6));
          cairo_show_text(cr, temp);
        } else if((f2%divisor2)==00LL) {
          SetColour(cr, DARK_LINES);
          cairo_set_dash(cr, dashed2, len2, 0);
          cairo_move_to(cr,(double)x,0);
          cairo_line_to(cr,(double)x,(double)display_height-20);
          cairo_stroke(cr);
        }
      }
      f2=f2+divisor2;
    } while(x<display_width);
    


    if(rx->band_a!=band60) {
      // band edges
      if(band->frequencyMin!=0LL) {
        SetColour(cr, WARNING);
        cairo_set_line_width(cr, 2.0);
        if((min_display<band->frequencyMin)&&(max_display>band->frequencyMin)) {
          i=(int)(((double)band->frequencyMin-(double)min_display)/rx->hz_per_pixel);
          cairo_move_to(cr,(double)i,0.0);
          cairo_line_to(cr,(double)i,(double)display_height-20);
          cairo_stroke(cr);
        }
        if((min_display<band->frequencyMax)&&(max_display>band->frequencyMax)) {
          i=(int)(((double)band->frequencyMax-(double)min_display)/rx->hz_per_pixel);
          cairo_move_to(cr,(double)i,0.0);
          cairo_line_to(cr,(double)i,(double)display_height-20);
          cairo_stroke(cr);
        }
        cairo_set_line_width(cr, LINE_WIDTH);
      }
    }
    
    cairo_set_dash(cr, 0, 0, 0);
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
          SetColour(cr, TEXT_A);
          cairo_move_to(cr,x,hang_y-8.0);
          cairo_rectangle(cr, x, hang_y-8.0,8.0,8.0);
          cairo_fill(cr);
          cairo_move_to(cr,x,hang_y);
          cairo_line_to(cr,(double)display_width-x,hang_y);
          cairo_stroke(cr);
          cairo_move_to(cr,x+8.0,hang_y);
          cairo_show_text(cr, "-H");
        }
  
        SetColour(cr, TEXT_C);
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

    // signal
    double s2;
    
    samples[display_width-1+offset]=-200;
    
    cairo_move_to(cr, 0.0, display_height-20);

    for(i=1;i<display_width;i++) {
      s2=(double)samples[i+offset]+attenuation+radio->panadapter_calibration;
      s2 = floor((rx->panadapter_high - s2) *dbm_per_line);
      if (s2 >= rx->panadapter_height-20) {
        s2 = rx->panadapter_height-20;
      }
      cairo_line_to(cr, (double)i, s2);
    }
  

    cairo_pattern_t *gradient;
    if(rx->panadapter_gradient) {
      gradient = cairo_pattern_create_linear(0.0, rx->panadapter_height-20, 0.0, 0.0);
      // calculate where S9 is
      double S9=-73;
      if(rx->frequency_a>30000000LL) {
        S9=-93;
      }
      S9 = floor((rx->panadapter_high - S9)
                              * (double)(rx->panadapter_height-20)
                            / (rx->panadapter_high - rx->panadapter_low));
      S9 = 1.0-(S9/(double)(rx->panadapter_height-20));
  
      cairo_pattern_add_color_stop_rgb (gradient,0.0,0.0,1.0,0.0); // Green
      cairo_pattern_add_color_stop_rgb (gradient,S9/3.0,1.0,0.65,0.0); // Orange
      cairo_pattern_add_color_stop_rgb (gradient,(S9/3.0)*2.0,1.0,1.0,0.0); // Yellow
      cairo_pattern_add_color_stop_rgb (gradient,S9,1.0,0.0,0.0); // Red
      cairo_set_source(cr, gradient);
    } else {
      cairo_set_source_rgba(cr, 1.0, 1.0, 1.0,0.5);
    }

      
    if(rx->panadapter_filled) {
      cairo_close_path (cr);
      /*
      cairo_pattern_t *pat=cairo_pattern_create_linear(0.624,	0.427,	0.690,(rx->panadapter_height-20));     
      cairo_pattern_add_color_stop_rgba(pat,0.0,0.804,	0.635,	0.859,0.5);
      cairo_pattern_add_color_stop_rgba(pat,1.0,0.804,	0.635,	0.859,0.5);
      cairo_set_source (cr, pat);
      */
      cairo_fill_preserve(cr);
      //cairo_pattern_destroy(pat);
    }
    //cairo_set_source_rgb(cr, 0.804,	0.635,	0.859);
    cairo_stroke(cr);
    if(rx->panadapter_gradient) {
      cairo_pattern_destroy(gradient);
    }

    //SetColour(cr, BACKGROUND);
    //cairo_rectangle(cr,0,(rx->panadapter_height - 18),display_width,18);
    //cairo_fill(cr);

    cairo_destroy (cr);
    gtk_widget_queue_draw (rx->panadapter);
  }
}
