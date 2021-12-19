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
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <wdsp.h>

#include "button_text.h"
#include "discovered.h"
#include "bpsk.h"
#include "mode.h"
#include "filter.h"
#include "band.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "receiver_dialog.h"
#include "vfo.h"
#include "audio.h"
#include "main.h"
#include "rigctl.h"

#define BAND_COLUMNS 5
#define MODE_COLUMNS 4
#define FILTER_COLUMNS 5

typedef struct _SELECT {
  RECEIVER *rx;
  gint choice;
} SELECT;

static void update_filters(RECEIVER *rx);

/* TO REMOVE
static gboolean close_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->dialog=NULL;
  rx->band_grid=NULL;
  rx->mode_grid=NULL;
  rx->filter_frame=NULL;
  rx->filter_grid=NULL;
  return TRUE;
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->dialog=NULL;
  rx->band_grid=NULL;
  rx->mode_grid=NULL;
  rx->filter_frame=NULL;
  rx->filter_grid=NULL;
  return FALSE;
}
*/

static void sample_rate_cb(GtkWidget *widget,gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  int sample_rate=select->choice;
  receiver_change_sample_rate(rx,sample_rate);
}

static void adc_cb(GtkWidget *widget,gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  rx->adc=select->choice;
  receiver_update_title(rx);
}

/* TO REMOVE
static gboolean band_select_cb(GtkWidget *widget,gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  BAND *b;
  int band=select->choice;
  int mode_a;
  long long frequency_a;
  long long lo_a=0LL;
  long long error_a=0LL;

  switch(band) {
    case band2200:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=LSB;
          frequency_a=136000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWL;
          frequency_a=136000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=LSB;
          frequency_a=136000LL;
          break;
      }
      break;
    case band630:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
        case CWL:
        case CWU:
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=CWL;
          frequency_a=472100LL;
          break;
      }
      break;
    case band160:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=LSB;
          frequency_a=1900000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWL;
          frequency_a=1830000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=LSB;
          frequency_a=1900000LL;
          break;
      }
      break;
    case band80:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=LSB;
          frequency_a=3700000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWL;
          frequency_a=3520000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=LSB;
          frequency_a=3700000LL;
          break;
      }
      break;
    case band60:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=LSB;
          switch(radio->region) {
            case REGION_OTHER:
              frequency_a=5330000LL;
              break;
            case REGION_UK:
              frequency_a=5280000LL;
              break;
          }
          break;
        case CWL:
        case CWU:
          mode_a=CWL;
          switch(radio->region) {
            case REGION_OTHER:
              frequency_a=5330000LL;
              break;
            case REGION_UK:
              frequency_a=5280000LL;
              break;
          }
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=LSB;
          switch(radio->region) {
            case REGION_OTHER:
              frequency_a=5330000LL;
              break;
            case REGION_UK:
              frequency_a=5280000LL;
              break;
          }
          break;
      }
      break;
    case band40:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=LSB;
          frequency_a=7100000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWL;
          frequency_a=7020000LL;
          break;
        case DIGU:
        case SPEC:
        case DIGL:
          mode_a=LSB;
          frequency_a=7070000LL;
          break;
        case FMN:
        case AM:
        case SAM:
        case DRM:
          mode_a=LSB;
          frequency_a=7100000LL;
          break;
      }
      break;
    case band30:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=USB;
          frequency_a=10145000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWU;
          frequency_a=10120000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=USB;
          frequency_a=10145000LL;
          break;
      }
      break;
    case band20:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=USB;
          frequency_a=14150000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWU;
          frequency_a=14020000LL;
          break;
        case DIGU:
        case SPEC:
        case DIGL:
          mode_a=USB;
          frequency_a=14070000LL;
          break;
        case FMN:
        case AM:
        case SAM:
        case DRM:
          mode_a=USB;
          frequency_a=14020000LL;
          break;
      }
      break;
    case band17:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=USB;
          frequency_a=18140000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWU;
          frequency_a=18080000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=USB;
          frequency_a=18140000LL;
          break;
      }
      break;
    case band15:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=USB;
          frequency_a=21200000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWU;
          frequency_a=21080000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=USB;
          frequency_a=21200000LL;
          break;
      }
      break;
    case band12:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=USB;
          frequency_a=24960000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWU;
          frequency_a=24900000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=USB;
          frequency_a=24960000LL;
          break;
      }
      break;
    case band10:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=USB;
          frequency_a=28300000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWU;
          frequency_a=28020000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=USB;
          frequency_a=28300000LL;
          break;
      }
      break;
    case band6:
      switch(rx->mode_a) {
        case LSB:
        case USB:
        case DSB:
          mode_a=USB;
          frequency_a=51000000LL;
          break;
        case CWL:
        case CWU:
          mode_a=CWU;
          frequency_a=50090000LL;
          break;
        case FMN:
        case AM:
        case DIGU:
        case SPEC:
        case DIGL:
        case SAM:
        case DRM:
          mode_a=USB;
          frequency_a=51000000LL;
          break;
      }
      break;
#ifdef SOAPYSDR
    case band70:
      mode_a=USB;
      frequency_a=70200000;
      break;
    case band144:
      mode_a=USB;
      frequency_a=144200000;
      break;
    case band220:
      mode_a=USB;
      frequency_a=220200000;
      break;
    case band430:
      mode_a=USB;
      frequency_a=432200000;
      break;
    case band902:
      mode_a=USB;
      frequency_a=902200000;
      break;
    case band1240:
      mode_a=USB;
      frequency_a=1240200000;
      break;
    case band2300:
      mode_a=USB;
      frequency_a=2300300000;
      break;
    case band3400:
      mode_a=USB;
      frequency_a=3400300000;
      break;
    case bandAIR:
      mode_a=AM;
      frequency_a=118800000;
      break;
#endif
    case bandGen:
      mode_a=AM;
      frequency_a=5975000LL;
      break;
    case bandWWV:
      mode_a=SAM;
      frequency_a=10000000LL;
      break;
    default:
      b=band_get_band(band);
      mode_a=USB;
      frequency_a=b->frequencyMin;
      lo_a=b->frequencyLO;
      error_a=b->errorLO;
      break;
  }
  if(band!=rx->band_a) {
    GtkWidget *grid=gtk_widget_get_ancestor(widget,GTK_TYPE_GRID);
    int x=rx->band_a%BAND_COLUMNS;
    int y=rx->band_a/BAND_COLUMNS;
    GtkWidget *b=gtk_grid_get_child_at(GTK_GRID(grid),x,y);
    set_button_text_color(b,"black");
    set_button_text_color(widget,"orange");
  }
  if(mode_a!=rx->mode_a) {
  }
  rx->band_a=band;
  rx->frequency_a=frequency_a;
  rx->lo_a=lo_a;
  rx->error_a=error_a;
  receiver_band_changed(rx,band);
  update_mode(rx,mode_a);
  if(radio->transmitter->rx==rx) {
    if(radio->transmitter->rx->split) {
      transmitter_set_mode(radio->transmitter,rx->mode_b);
    } else {
      transmitter_set_mode(radio->transmitter,rx->mode_a);
    }
  }
  return TRUE;
}
*/

static gboolean filter_select_cb(GtkWidget *widget,gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  gint f=select->choice;
  GtkWidget *grid=gtk_widget_get_ancestor(widget, GTK_TYPE_GRID);
  int x=rx->filter_a%FILTER_COLUMNS;
  int y=rx->filter_a/FILTER_COLUMNS;
  if(rx->filter_a>=FVar1) {
    y=1+((rx->filter_a+4)/5);
    x=0;
  }
  GtkWidget *b=gtk_grid_get_child_at(GTK_GRID(grid),x,y);
  set_button_text_color(b,"black");
  set_button_text_color(widget,"orange");
  receiver_filter_changed(rx,f);
  return TRUE;
}

static gboolean deviation_select_cb(GtkWidget *widget,gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  rx->deviation=select->choice;
  //transmitter->deviation=select->choice;
  if(rx->deviation==2500) {
    set_filter(rx,-4000,4000);
    if(radio->transmitter) transmitter_set_filter(radio->transmitter,-4000,4000);
  } else {
    set_filter(rx,-8000,8000);
    if(radio->transmitter) transmitter_set_filter(radio->transmitter,-8000,8000);
  }
  set_deviation(rx);
  if(radio->transmitter) transmitter_set_deviation(radio->transmitter);
  update_vfo(rx);
  return TRUE;
}

static void var_spin_low_cb (GtkWidget *widget, gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  gint f=select->choice;

  FILTER *mode_filters=filters[rx->mode_a];
  FILTER *filter=&mode_filters[f];

  filter->low=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  if(rx->mode_a==CWL || rx->mode_a==CWU) {
    filter->high=filter->low;
  }
  if(f==rx->filter_a) {
    receiver_filter_changed(rx,f);
  }
}

static void var_spin_high_cb (GtkWidget *widget, gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  gint f=select->choice;

  FILTER *mode_filters=filters[rx->mode_a];
  FILTER *filter=&mode_filters[f];

  filter->high=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  if(f==rx->filter_a) {
    receiver_filter_changed(rx,f);
  }
}


static void update_filters(RECEIVER *rx) {
  int i;
  int row;
  SELECT *select;

  FILTER* band_filters=filters[rx->mode_a];

  if(rx->filter_frame!=NULL && rx->filter_grid!=NULL) {
    gtk_container_remove(GTK_CONTAINER(rx->filter_frame),rx->filter_grid);
  }

  rx->filter_grid=gtk_grid_new();
g_print("update_filters: new filter grid %p\n",rx->filter_grid);
  gtk_grid_set_row_homogeneous(GTK_GRID(rx->filter_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(rx->filter_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(rx->filter_frame),rx->filter_grid);
  switch(rx->mode_a) {
    case FMN:
      {
      GtkWidget *l=gtk_label_new("Deviation:");
      gtk_grid_attach(GTK_GRID(rx->filter_grid),l,0,1,1,1);

      GtkWidget *b=gtk_button_new_with_label("2.5K");
      if(rx->deviation==2500) {
        set_button_text_color(b,"orange");
        //last_filter=b;
      } else {
        set_button_text_color(b,"black");
      }
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=2500;
      g_signal_connect(b,"pressed",G_CALLBACK(deviation_select_cb),(gpointer)select);
      gtk_grid_attach(GTK_GRID(rx->filter_grid),b,1,1,1,1);

      b=gtk_button_new_with_label("5.0K");
      if(rx->deviation==5000) {
        set_button_text_color(b,"orange");
        //last_filter=b;
      } else {
        set_button_text_color(b,"black");
      }
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=5000;
      g_signal_connect(b,"pressed",G_CALLBACK(deviation_select_cb),(gpointer)select);
      gtk_grid_attach(GTK_GRID(rx->filter_grid),b,2,1,1,1);
      }
      break;

    default:
      /*
      for(i=0;i<FILTERS-2;i++) {
        FILTER* band_filter=&band_filters[i];
        GtkWidget *b=gtk_button_new_with_label(band_filters[i].title);
        if(i==rx->filter_a) {
          set_button_text_color(b,"orange");
          //last_filter=b;
        } else {
          set_button_text_color(b,"black");
        }
        select=g_new0(SELECT,1);
        select->rx=rx;
        select->choice=i;
        g_signal_connect(b,"pressed",G_CALLBACK(filter_select_cb),(gpointer)select);
        gtk_grid_attach(GTK_GRID(rx->filter_grid),b,i%FILTER_COLUMNS,i/FILTER_COLUMNS,1,1);
      }
      */

  // last 2 are var1 and var2
      i=FILTERS-2;
      row=1+((i+4)/5);
      FILTER* band_filter=&band_filters[i];
      GtkWidget *b=gtk_button_new_with_label(band_filters[i].title);
      if(i==rx->filter_a) {
        set_button_text_color(b,"orange");
        //last_filter=b;
      } else {
        set_button_text_color(b,"black");
      }
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=i;
      g_signal_connect(b,"pressed",G_CALLBACK(filter_select_cb),(gpointer)select);
      gtk_grid_attach(GTK_GRID(rx->filter_grid),b,0,row,1,1);

      GtkWidget *var1_spin_low=gtk_spin_button_new_with_range(-8000.0,+8000.0,1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(var1_spin_low),(double)band_filter->low);
      gtk_grid_attach(GTK_GRID(rx->filter_grid),var1_spin_low,1,row,2,1);
      g_signal_connect(var1_spin_low,"value-changed",G_CALLBACK(var_spin_low_cb),(gpointer)select);

      if(rx->mode_a!=CWL && rx->mode_a!=CWU) {
        GtkWidget *var1_spin_high=gtk_spin_button_new_with_range(-8000.0,+8000.0,1.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(var1_spin_high),(double)band_filter->high);
        gtk_grid_attach(GTK_GRID(rx->filter_grid),var1_spin_high,3,row,2,1);
        g_signal_connect(var1_spin_high,"value-changed",G_CALLBACK(var_spin_high_cb),(gpointer)select);
      }

      row++;

      i++;
      band_filter=&band_filters[i];
      b=gtk_button_new_with_label(band_filters[i].title);
      if(i==rx->filter_a) {
        set_button_text_color(b,"orange");
        //last_filter=b;
      } else {
        set_button_text_color(b,"black");
      }
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=i;
      gtk_grid_attach(GTK_GRID(rx->filter_grid),b,0,row,1,1);
      g_signal_connect(b,"pressed",G_CALLBACK(filter_select_cb),(gpointer)select);

     GtkWidget *var2_spin_low=gtk_spin_button_new_with_range(-8000.0,+8000.0,1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(var2_spin_low),(double)band_filter->low);
      gtk_grid_attach(GTK_GRID(rx->filter_grid),var2_spin_low,1,row,2,1);
      g_signal_connect(var2_spin_low,"value-changed",G_CALLBACK(var_spin_low_cb),(gpointer)select);

     if(rx->mode_a!=CWL && rx->mode_a!=CWU) {
        GtkWidget *var2_spin_high=gtk_spin_button_new_with_range(-8000.0,+8000.0,1.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(var2_spin_high),(double)band_filter->high);
        gtk_grid_attach(GTK_GRID(rx->filter_grid),var2_spin_high,3,row,2,1);
        g_signal_connect(var2_spin_high,"value-changed",G_CALLBACK(var_spin_high_cb),(gpointer)select);
      }
    gtk_widget_show_all(rx->filter_frame);
  }
}

/* TO REMOVE
static void update_mode(RECEIVER *rx,int mode) {
  int x=rx->mode_a%MODE_COLUMNS;
  int y=rx->mode_a/MODE_COLUMNS;
  GtkWidget *b=gtk_grid_get_child_at(GTK_GRID(rx->mode_grid),x,y);
  set_button_text_color(b,"black");
  receiver_mode_changed(rx,mode);
  x=rx->mode_a%MODE_COLUMNS;
  y=rx->mode_a/MODE_COLUMNS;
  b=gtk_grid_get_child_at(GTK_GRID(rx->mode_grid),x,y);
  set_button_text_color(b,"orange");
  update_filters(rx);
  if(radio->transmitter->rx==rx) {
    if(rx->split) {
      transmitter_set_mode(radio->transmitter,rx->mode_b);
    } else {
      transmitter_set_mode(radio->transmitter,rx->mode_a);
    }
  }
}


static gboolean mode_select_cb(GtkWidget *widget,gpointer data) {
  SELECT *select=(SELECT *)data;
  RECEIVER *rx=select->rx;
  int m=select->choice;
  update_mode(rx,m);
  return TRUE;
}
*/

static void tx_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  RECEIVER *temp=radio->transmitter->rx;
  if(radio->transmitter->rx==rx) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), radio->transmitter->rx==rx);
  } else {
    radio->transmitter->rx=rx;
  }
  update_vfo(temp);
  update_vfo(rx);
}

static void fps_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->fps=gtk_range_get_value(GTK_RANGE(widget));
  receiver_fps_changed(rx);
}


static void panadapter_average_time_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->display_average_time=gtk_range_get_value(GTK_RANGE(widget));
  calculate_display_average(rx);
}

static void panadapter_high_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->panadapter_high=gtk_range_get_value(GTK_RANGE(widget));
}

static void panadapter_low_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->panadapter_low=gtk_range_get_value(GTK_RANGE(widget));
}

static void panadapter_step_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->panadapter_step=gtk_range_get_value(GTK_RANGE(widget));
}

static void panadapter_filled_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->panadapter_filled=rx->panadapter_filled==TRUE?FALSE:TRUE;
}

static void panadapter_gradient_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->panadapter_gradient=rx->panadapter_gradient==TRUE?FALSE:TRUE;
}

static void panadapter_agc_line_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->panadapter_agc_line=rx->panadapter_agc_line==TRUE?FALSE:TRUE;
}

static void waterfall_high_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->waterfall_high=gtk_range_get_value(GTK_RANGE(widget));
}

static void waterfall_low_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->waterfall_low=gtk_range_get_value(GTK_RANGE(widget));
}

static void waterfall_automatic_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->waterfall_automatic=rx->waterfall_automatic==1?0:1;
}

static void waterfall_ft8_marker_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->waterfall_ft8_marker=rx->waterfall_ft8_marker==TRUE?FALSE:TRUE;
}

static void remote_audio_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->remote_audio=rx->remote_audio==TRUE?FALSE:TRUE;
}

static void local_audio_cb(GtkWidget *widget,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->local_audio=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
  if(rx->local_audio) {
    if(audio_open_output(rx)<0) {
      rx->local_audio=FALSE;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget),FALSE);
    }
  } else {
    audio_close_output(rx);
  }
}

static void audio_channels_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->audio_channels = gtk_combo_box_get_active(GTK_COMBO_BOX (widget));
}

static void audio_choice_cb(GtkComboBox *widget,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  int i;
  if(rx->local_audio) {
    audio_close_output(rx);
    i=gtk_combo_box_get_active(widget);
    if(rx->audio_name!=NULL) {
      g_free(rx->audio_name);
      //rx->audio_name=NULL;
    }
    if(i>=0) {
      rx->audio_name=g_new0(gchar,strlen(output_devices[i].name)+1);
      rx->output_index=output_devices[i].index;
      strcpy(rx->audio_name,output_devices[i].name);
      if(audio_open_output(rx)<0) {
        rx->local_audio=FALSE;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (rx->local_audio_b),FALSE);
      }
    }
  } else {
    i=gtk_combo_box_get_active(widget);
    if(rx->audio_name!=NULL) {
      g_free(rx->audio_name);
      rx->audio_name=NULL;
    }
    if(i>=0) {
      rx->audio_name=g_new0(gchar,strlen(output_devices[i].name)+1);
      strcpy(rx->audio_name,output_devices[i].name);
    }
  }
  if(gtk_combo_box_get_active(GTK_COMBO_BOX(rx->audio_choice_b))==-1) {
    gtk_widget_set_sensitive(rx->local_audio_b, FALSE);
  } else {
    gtk_widget_set_sensitive(rx->local_audio_b, TRUE);
  }
  if(i>=0) {
    g_print("Output device changed: %d: %s (%s)\n",i,output_devices[i].name,output_devices[i].description);
  } else {
    g_print("Output device changed: %d\n",i);
  }
}

/* TO REMOVE
static void buffer_size_spin_cb(GtkWidget *widget,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->local_audio_buffer_size=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
}

static void latency_spin_cb(GtkWidget *widget,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->local_audio_latency=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
}
*/

static void enable_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->enable_equalizer=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  SetRXAEQRun(rx->channel, rx->enable_equalizer);
}

static void preamp_value_changed_cb (GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->equalizer[0]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetRXAGrphEQ(rx->channel, rx->equalizer);
}

static void low_value_changed_cb (GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->equalizer[1]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetRXAGrphEQ(rx->channel, rx->equalizer);
}

static void mid_value_changed_cb (GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->equalizer[2]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetRXAGrphEQ(rx->channel, rx->equalizer);
}

static void high_value_changed_cb (GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->equalizer[3]=(int)gtk_range_get_value(GTK_RANGE(widget));
  SetRXAGrphEQ(rx->channel, rx->equalizer);
}


static void cat_debug_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->rigctl_debug=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
  if(rx->rigctl!=NULL) {
    rigctl_set_debug(rx);
  }
}

static void cat_enable_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->rigctl_enable=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
  if(rx->rigctl_enable) {
    launch_rigctl(rx);
  } else {
    disable_rigctl(rx);
  }
}

static void rigctl_value_changed_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->rigctl_port = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
}

static void cat_serial_enable_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  strcpy(rx->rigctl_serial_port,gtk_entry_get_text(GTK_ENTRY(rx->serial_port_entry)));
  rx->rigctl_serial_enable=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
  if(rx->rigctl_serial_enable) {
    launch_serial(rx);
  } else {
    disable_serial(rx);
  }
}

static void cat_serial_port_cb(GtkWidget *widget, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  strcpy(rx->rigctl_serial_port,gtk_entry_get_text(GTK_ENTRY(widget)));
}

static void cat_baudrate_cb(GtkWidget *widget,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  int selected=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  switch(selected) {
    case 0:
      rx->rigctl_serial_baudrate=B4800;
      break;
    case 1:
      rx->rigctl_serial_baudrate=B9600;
      break;
    case 2:
      rx->rigctl_serial_baudrate=B19200;
      break;
    case 3:
      rx->rigctl_serial_baudrate=B38400;
      break;
  }
}

void update_receiver_dialog(RECEIVER *rx) {
  int i;

  // update audio
  g_signal_handler_block(G_OBJECT(rx->audio_choice_b),rx->audio_choice_signal_id);
  g_signal_handler_block(G_OBJECT(rx->local_audio_b),rx->local_audio_signal_id);
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(rx->audio_choice_b));
  for(i=0;i<n_output_devices;i++) {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(rx->audio_choice_b),NULL,output_devices[i].description);
    if(rx->audio_name!=NULL) {
      if(strcmp(output_devices[i].name,rx->audio_name)==0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(rx->audio_choice_b),i);
      }
    }
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rx->local_audio_b), rx->local_audio);

  if(gtk_combo_box_get_active(GTK_COMBO_BOX(rx->audio_choice_b))==-1) {
    gtk_widget_set_sensitive(rx->local_audio_b, FALSE);
  } else {
    gtk_widget_set_sensitive(rx->local_audio_b, TRUE);
  }

  g_signal_handler_unblock(G_OBJECT(rx->local_audio_b),rx->local_audio_signal_id);
  g_signal_handler_unblock(G_OBJECT(rx->audio_choice_b),rx->audio_choice_signal_id);

  if(radio->transmitter) {
    // update TX Frequency
    g_signal_handler_block(G_OBJECT(rx->tx_control_b),rx->tx_control_signal_id);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rx->tx_control_b), radio->transmitter->rx==rx);
    g_signal_handler_unblock(G_OBJECT(rx->tx_control_b),rx->tx_control_signal_id);
  }

}

GtkWidget *create_receiver_dialog(RECEIVER *rx) {
  int i;
  int col=0;
  int row=0;
  SELECT *select;

  GtkWidget *grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(grid),FALSE);
  gtk_grid_set_column_spacing(GTK_GRID(grid),5);

  row=0;
  col=0;

  if(radio->discovered->adcs>1) {
    GtkWidget *adc_frame=gtk_frame_new("ADC");
    GtkWidget *adc_grid=gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(adc_grid),FALSE);
    gtk_grid_set_column_homogeneous(GTK_GRID(adc_grid),FALSE);
    gtk_container_add(GTK_CONTAINER(adc_frame),adc_grid);
    gtk_grid_attach(GTK_GRID(grid),adc_frame,col,row++,1,1);

    GtkWidget *adc0_b=gtk_radio_button_new_with_label(NULL,"ADC-0");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (adc0_b), rx->adc==0);
    gtk_grid_attach(GTK_GRID(adc_grid),adc0_b,0,0,1,1);
    select=g_new0(SELECT,1);
    select->rx=rx;
    select->choice=0;
    g_signal_connect(adc0_b,"pressed",G_CALLBACK(adc_cb),(gpointer)select);

    GtkWidget *adc1_b=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(adc0_b),"ADC-1");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (adc1_b), rx->adc==1);
    select=g_new0(SELECT,1);
    select->rx=rx;
    select->choice=1;
    g_signal_connect(adc1_b,"pressed",G_CALLBACK(adc_cb),(gpointer)select);
    gtk_grid_attach(GTK_GRID(adc_grid),adc1_b,1,0,1,1);
  }

  switch(radio->discovered->protocol) {
    case PROTOCOL_2:
#ifdef SOAPYSDR
    case PROTOCOL_SOAPYSDR:
      if(strcmp(radio->discovered->name,"sdrplay")!=0)
#endif
      {
      int x=0;
      int y=0;

      GtkWidget *sample_rate_frame=gtk_frame_new("Sample Rate");
      GtkWidget *sample_rate_grid=gtk_grid_new();
      gtk_grid_set_row_homogeneous(GTK_GRID(sample_rate_grid),FALSE);
      gtk_grid_set_column_homogeneous(GTK_GRID(sample_rate_grid),FALSE);
      gtk_container_add(GTK_CONTAINER(sample_rate_frame),sample_rate_grid);
      gtk_grid_attach(GTK_GRID(grid),sample_rate_frame,col,row,1,2);
      row+=2;

      GtkWidget *sample_rate_48=gtk_radio_button_new_with_label(NULL,"48000");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sample_rate_48), rx->sample_rate==48000);
      gtk_grid_attach(GTK_GRID(sample_rate_grid),sample_rate_48,x,y++,1,1);
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=48000;
      g_signal_connect(sample_rate_48,"pressed",G_CALLBACK(sample_rate_cb),(gpointer)select);

      GtkWidget *sample_rate_96=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sample_rate_48),"96000");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sample_rate_96), rx->sample_rate==96000);
      gtk_grid_attach(GTK_GRID(sample_rate_grid),sample_rate_96,x,y++,1,1);
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=96000;
      g_signal_connect(sample_rate_96,"pressed",G_CALLBACK(sample_rate_cb),(gpointer)select);

      GtkWidget *sample_rate_192=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sample_rate_96),"192000");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sample_rate_192), rx->sample_rate==192000);
      gtk_grid_attach(GTK_GRID(sample_rate_grid),sample_rate_192,x,y++,1,1);
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=192000;
      g_signal_connect(sample_rate_192,"pressed",G_CALLBACK(sample_rate_cb),(gpointer)select);

      x++;
      y=0;
      GtkWidget *sample_rate_384=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sample_rate_192),"384000");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sample_rate_384), rx->sample_rate==384000);
      gtk_grid_attach(GTK_GRID(sample_rate_grid),sample_rate_384,x,y++,1,1);
      select=g_new0(SELECT,1);
      select->rx=rx;
      select->choice=384000;
      g_signal_connect(sample_rate_384,"pressed",G_CALLBACK(sample_rate_cb),(gpointer)select);

      if((radio->discovered->protocol==PROTOCOL_2)
#ifdef SOAPYSDR
          || (radio->discovered->protocol==PROTOCOL_SOAPYSDR)
#endif
      ) {
        GtkWidget *sample_rate_768=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sample_rate_384),"768000");
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sample_rate_768), rx->sample_rate==768000);
        gtk_grid_attach(GTK_GRID(sample_rate_grid),sample_rate_768,x,y++,1,1);
        select=g_new0(SELECT,1);
        select->rx=rx;
        select->choice=768000;
        g_signal_connect(sample_rate_768,"pressed",G_CALLBACK(sample_rate_cb),(gpointer)select);

        if(radio->discovered->protocol==PROTOCOL_2) {
          GtkWidget *sample_rate_1536=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(sample_rate_768),"1536000");
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sample_rate_1536), rx->sample_rate==1536000);
          gtk_grid_attach(GTK_GRID(sample_rate_grid),sample_rate_1536,x,y++,1,1);
          select=g_new0(SELECT,1);
          select->rx=rx;
          select->choice=1536000;
          g_signal_connect(sample_rate_1536,"pressed",G_CALLBACK(sample_rate_cb),(gpointer)select);
        }
      }
    }
    break;
  }

  rx->filter_frame=gtk_frame_new("Filter");
  gtk_grid_attach(GTK_GRID(grid),rx->filter_frame,col,row,1,1);
  row++;

  update_filters(rx);

  col=0;

  if(n_output_devices>=0) {
    GtkWidget *audio_frame=gtk_frame_new("Audio");
    GtkWidget *audio_grid=gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(audio_grid),FALSE);
    gtk_grid_set_column_homogeneous(GTK_GRID(audio_grid),FALSE);
    gtk_container_add(GTK_CONTAINER(audio_frame),audio_grid);
    gtk_grid_attach(GTK_GRID(grid),audio_frame,col,row,1,1);
    row++;

    rx->local_audio_b=gtk_check_button_new_with_label("Local Audio");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rx->local_audio_b), rx->local_audio);
    gtk_grid_attach(GTK_GRID(audio_grid),rx->local_audio_b,0,0,1,1);
    rx->local_audio_signal_id=g_signal_connect(rx->local_audio_b,"toggled",G_CALLBACK(local_audio_cb),rx);

    if(radio->discovered->device!=DEVICE_HERMES_LITE2
#ifdef SOAPYSDR
       && radio->discovered->device!=DEVICE_SOAPYSDR
#endif
      ) {

      GtkWidget *remote_audio=gtk_check_button_new_with_label("Remote Audio");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (remote_audio), rx->remote_audio);
      gtk_grid_attach(GTK_GRID(audio_grid),remote_audio,1,0,1,1);
      g_signal_connect(remote_audio,"toggled",G_CALLBACK(remote_audio_cb),rx);
    }

    rx->audio_choice_b=gtk_combo_box_text_new();
    gtk_grid_attach(GTK_GRID(audio_grid),rx->audio_choice_b,0,2,2,1);
    rx->audio_choice_signal_id=g_signal_connect(rx->audio_choice_b,"changed",G_CALLBACK(audio_choice_cb),rx);
    
    // Audio output device options
    // TO REMOVE because the variable n_output_devices is always zero here
    // for(i=0;i<n_output_devices;i++) {
    //   gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(rx->audio_choice_b),NULL,output_devices[i].description);
    //   if(rx->audio_name!=NULL) {
    //     if(strcmp(output_devices[i].name,rx->audio_name)==0) {
    //       gtk_combo_box_set_active(GTK_COMBO_BOX(rx->audio_choice_b),i);
    //     }
    //   }
    // }
    
    // Moved to update_receiver_dialog
    // if(gtk_combo_box_get_active(GTK_COMBO_BOX(rx->audio_choice_b))==-1) {
    //   gtk_widget_set_sensitive(rx->local_audio_b, FALSE);
    // }

    // Stereo, left, right audio
    GtkWidget *audio_channels_combo=gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(audio_channels_combo),NULL,"Stereo");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(audio_channels_combo),NULL,"Left");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(audio_channels_combo),NULL,"Right");    
    gtk_combo_box_set_active(GTK_COMBO_BOX(audio_channels_combo),rx->audio_channels);
    gtk_grid_attach(GTK_GRID(audio_grid),audio_channels_combo,0,1,2,1);
    g_signal_connect(audio_channels_combo,"changed",G_CALLBACK(audio_channels_cb),rx);
  }

  GtkWidget *equalizer_frame=gtk_frame_new("Equalizer");
  GtkWidget *equalizer_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(equalizer_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(equalizer_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(equalizer_frame),equalizer_grid);
  gtk_grid_attach(GTK_GRID(grid),equalizer_frame,col,row,1,4);
  row+=4;

  GtkWidget *enable_b=gtk_check_button_new_with_label("Enable Equalizer");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (enable_b), rx->enable_equalizer);
  g_signal_connect(enable_b,"toggled",G_CALLBACK(enable_cb),rx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),enable_b,0,0,4,1);

  GtkWidget *label=gtk_label_new("Preamp");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,0,1,1,1);

  label=gtk_label_new("Low");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,1,1,1,1);

  label=gtk_label_new("Mid");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,2,1,1,1);

  label=gtk_label_new("High");
  gtk_grid_attach(GTK_GRID(equalizer_grid),label,3,1,1,1);

  GtkWidget *preamp_scale=gtk_scale_new(GTK_ORIENTATION_VERTICAL,gtk_adjustment_new(rx->equalizer[0],-12.0,15.0,1.0,1.0,1.0));
  gtk_range_set_inverted(GTK_RANGE(preamp_scale),TRUE);
  g_signal_connect(preamp_scale,"value-changed",G_CALLBACK(preamp_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),preamp_scale,0,2,1,10);
  gtk_widget_set_size_request(preamp_scale,10,270);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(preamp_scale),15.0,GTK_POS_LEFT,"15dB");

  GtkWidget *low_scale=gtk_scale_new(GTK_ORIENTATION_VERTICAL,gtk_adjustment_new(rx->equalizer[1],-12.0,15.0,1.0,1.0,1.0));
  gtk_range_set_inverted(GTK_RANGE(low_scale),TRUE);
  g_signal_connect(low_scale,"value-changed",G_CALLBACK(low_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),low_scale,1,2,1,10);
  gtk_scale_add_mark(GTK_SCALE(low_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(low_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(low_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(low_scale),15.0,GTK_POS_LEFT,"15dB");

  GtkWidget *mid_scale=gtk_scale_new(GTK_ORIENTATION_VERTICAL,gtk_adjustment_new(rx->equalizer[2],-12.0,15.0,1.0,1.0,1.0));
  gtk_range_set_inverted(GTK_RANGE(mid_scale),TRUE);
  g_signal_connect(mid_scale,"value-changed",G_CALLBACK(mid_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),mid_scale,2,2,1,10);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(mid_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(mid_scale),15.0,GTK_POS_LEFT,"15dB");

  GtkWidget *high_scale=gtk_scale_new(GTK_ORIENTATION_VERTICAL,gtk_adjustment_new(rx->equalizer[3],-12.0,15.0,1.0,1.0,1.0));
  gtk_range_set_inverted(GTK_RANGE(high_scale),TRUE);
  g_signal_connect(high_scale,"value-changed",G_CALLBACK(high_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(equalizer_grid),high_scale,3,2,1,10);
  gtk_scale_add_mark(GTK_SCALE(high_scale),-12.0,GTK_POS_LEFT,"-12dB");
  gtk_scale_add_mark(GTK_SCALE(high_scale),-9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),-6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),-3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),0.0,GTK_POS_LEFT,"0dB");
  gtk_scale_add_mark(GTK_SCALE(high_scale),3.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),6.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),9.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),12.0,GTK_POS_LEFT,NULL);
  gtk_scale_add_mark(GTK_SCALE(high_scale),15.0,GTK_POS_LEFT,"15dB");

  col++;
  row=0;

  if(strcmp(radio->discovered->name,"rtlsdr")!=0) {
    GtkWidget *tx_frame=gtk_frame_new("TX Frequency");
    GtkWidget *tx_grid=gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(tx_grid),FALSE);
    gtk_grid_set_column_homogeneous(GTK_GRID(tx_grid),FALSE);
    gtk_container_add(GTK_CONTAINER(tx_frame),tx_grid);
    gtk_grid_attach(GTK_GRID(grid),tx_frame,col,row,1,1);
    row++;

    rx->tx_control_b=gtk_check_button_new_with_label("Use This Receivers Frequency");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rx->tx_control_b), radio->transmitter!=NULL && radio->transmitter->rx==rx);
    gtk_grid_attach(GTK_GRID(tx_grid),rx->tx_control_b,0,0,1,1);
    rx->tx_control_signal_id=g_signal_connect(rx->tx_control_b,"toggled",G_CALLBACK(tx_cb),rx);
  }

  GtkWidget *panadapter_frame=gtk_frame_new("Panadapter");
  GtkWidget *panadapter_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(panadapter_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(panadapter_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(panadapter_frame),panadapter_grid);
  gtk_grid_attach(GTK_GRID(grid),panadapter_frame,col,row,1,3);
  row+=3;

  
  GtkWidget *fps_label=gtk_label_new("FPS:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),fps_label,0,0,1,1);

  GtkWidget *fps_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(rx->fps, 1.0, 50.0, 1.0, 1.0, 1.0));
  gtk_widget_set_size_request(fps_scale,200,30);
  gtk_widget_show(fps_scale);
  g_signal_connect(G_OBJECT(fps_scale),"value_changed",G_CALLBACK(fps_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),fps_scale,1,0,1,1);
  
  GtkWidget *average_label=gtk_label_new("Average:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),average_label,0,1,1,1);

  GtkWidget *average_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(rx->display_average_time,1.0, 500.0, 1.00, 1.0, 1.0));
  gtk_widget_set_size_request(average_scale,200,30);
  gtk_widget_show(average_scale);
  g_signal_connect(G_OBJECT(average_scale),"value_changed",G_CALLBACK(panadapter_average_time_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),average_scale,1,1,1,1);

  GtkWidget *high_label=gtk_label_new("High:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),high_label,0,2,1,1);

  GtkWidget *panadapter_high_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(rx->panadapter_high,-200.0, 20.0, 1.00, 1.0, 1.0));
  gtk_widget_set_size_request(panadapter_high_scale,200,30);
  gtk_widget_show(panadapter_high_scale);
  g_signal_connect(G_OBJECT(panadapter_high_scale),"value_changed",G_CALLBACK(panadapter_high_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_high_scale,1,2,1,1);

  GtkWidget *low_label=gtk_label_new("Low:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),low_label,0,3,1,1);

  GtkWidget *panadapter_low_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(rx->panadapter_low,-200.0, 20.0, 1.0, 1.0, 1.0));
  gtk_widget_set_size_request(panadapter_low_scale,200,30);
  gtk_widget_show(panadapter_low_scale);
  g_signal_connect(G_OBJECT(panadapter_low_scale),"value_changed",G_CALLBACK(panadapter_low_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_low_scale,1,3,1,1);
  
  GtkWidget *step_label=gtk_label_new("Step:");
  gtk_grid_attach(GTK_GRID(panadapter_grid),step_label,0,4,1,1);

  GtkWidget *panadapter_step_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(rx->panadapter_step,1.0, 40.0, 1.0, 1.0, 1.0));
  gtk_widget_set_size_request(panadapter_step_scale,200,30);
  gtk_widget_show(panadapter_step_scale);
  g_signal_connect(G_OBJECT(panadapter_step_scale),"value_changed",G_CALLBACK(panadapter_step_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_step_scale,1,4,1,1);
  
  GtkWidget *panadapter_filled=gtk_check_button_new_with_label("Panadapter Filled");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panadapter_filled), rx->panadapter_filled);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_filled,0,5,2,1);
  g_signal_connect(panadapter_filled,"toggled",G_CALLBACK(panadapter_filled_changed_cb),rx);

  GtkWidget *panadapter_gradient=gtk_check_button_new_with_label("Panadapter Gradient");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panadapter_gradient), rx->panadapter_gradient);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_gradient,0,6,2,1);
  g_signal_connect(panadapter_gradient,"toggled",G_CALLBACK(panadapter_gradient_changed_cb),rx);

  GtkWidget *panadapter_agc_line=gtk_check_button_new_with_label("AGC Lines");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panadapter_agc_line), rx->panadapter_agc_line);
  gtk_grid_attach(GTK_GRID(panadapter_grid),panadapter_agc_line,0,7,2,1);
  g_signal_connect(panadapter_agc_line,"toggled",G_CALLBACK(panadapter_agc_line_changed_cb),rx);

  GtkWidget *waterfall_frame=gtk_frame_new("Waterfall");
  GtkWidget *waterfall_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(waterfall_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(waterfall_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(waterfall_frame),waterfall_grid);
  gtk_grid_attach(GTK_GRID(grid),waterfall_frame,col,row,1,2);
  row+=2;

  GtkWidget *waterfall_high_label=gtk_label_new("High:");
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_high_label,0,0,1,1);

  GtkWidget *waterfall_high_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(rx->waterfall_high,-200.0, 20.0, 1.0, 1.0, 1.0));
  gtk_widget_set_size_request(waterfall_high_scale,200,30);
  gtk_widget_show(waterfall_high_scale);
  g_signal_connect(G_OBJECT(waterfall_high_scale),"value_changed",G_CALLBACK(waterfall_high_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_high_scale,1,0,1,1);

  GtkWidget *waterfall_low_label=gtk_label_new("Low:");
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_low_label,0,1,1,1);

  GtkWidget *waterfall_low_scale=gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,gtk_adjustment_new(rx->waterfall_low,-200.0, 20.0, 1.0, 1.0, 1.0));
  gtk_widget_set_size_request(waterfall_low_scale,200,30);
  gtk_widget_show(waterfall_low_scale);
  g_signal_connect(G_OBJECT(waterfall_low_scale),"value_changed",G_CALLBACK(waterfall_low_value_changed_cb),rx);
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_low_scale,1,1,1,1);

  GtkWidget *waterfall_automatic=gtk_check_button_new_with_label("Waterfall Automatic");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (waterfall_automatic), rx->waterfall_automatic);
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_automatic,0,2,2,1);
  g_signal_connect(waterfall_automatic,"toggled",G_CALLBACK(waterfall_automatic_cb),rx);

  GtkWidget *waterfall_ft8_marker=gtk_check_button_new_with_label("Waterfall FT8 Marker");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (waterfall_ft8_marker), rx->waterfall_ft8_marker);
  gtk_grid_attach(GTK_GRID(waterfall_grid),waterfall_ft8_marker,0,3,2,1);
  g_signal_connect(waterfall_ft8_marker,"toggled",G_CALLBACK(waterfall_ft8_marker_cb),rx);

  col++;
  row=0;

  GtkWidget *cat_frame=gtk_frame_new("CAT");
  GtkWidget *cat_grid=gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(cat_grid),FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(cat_grid),FALSE);
  gtk_container_add(GTK_CONTAINER(cat_frame),cat_grid);
  gtk_grid_attach(GTK_GRID(grid),cat_frame,col,row,1,3);
  row++;

  GtkWidget *cat_debug_b=gtk_check_button_new_with_label("CAT Debug");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cat_debug_b), rx->rigctl_debug);
  gtk_grid_attach(GTK_GRID(cat_grid),cat_debug_b,0,0,1,1);
  g_signal_connect(cat_debug_b,"toggled",G_CALLBACK(cat_debug_cb),rx);


  GtkWidget *cat_enable_b=gtk_check_button_new_with_label("TCP/IP enable");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cat_enable_b), rx->rigctl_enable);
  gtk_grid_attach(GTK_GRID(cat_grid),cat_enable_b,0,1,1,1);
  g_signal_connect(cat_enable_b,"toggled",G_CALLBACK(cat_enable_cb),rx);

  GtkWidget *rigctl_port_spinner =gtk_spin_button_new_with_range(18000,21000,1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(rigctl_port_spinner),(double)rx->rigctl_port);
  gtk_widget_show(rigctl_port_spinner);
  gtk_grid_attach(GTK_GRID(cat_grid),rigctl_port_spinner,0,2,2,1);
  g_signal_connect(rigctl_port_spinner,"value_changed",G_CALLBACK(rigctl_value_changed_cb),rx);



  GtkWidget *cat_serial_enable_b=gtk_check_button_new_with_label("Serial Port Enable");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cat_serial_enable_b), rx->rigctl_serial_enable);
  gtk_grid_attach(GTK_GRID(cat_grid),cat_serial_enable_b,0,4,1,1);
  g_signal_connect(cat_serial_enable_b,"toggled",G_CALLBACK(cat_serial_enable_cb),rx);

  GtkWidget *serial_text_label=gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(serial_text_label), "<b>Serial Port: </b>");
  gtk_grid_attach(GTK_GRID(cat_grid),serial_text_label,0,5,1,1);

  rx->serial_port_entry=gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(rx->serial_port_entry),rx->rigctl_serial_port);
  gtk_widget_show(rx->serial_port_entry);
  gtk_grid_attach(GTK_GRID(cat_grid),rx->serial_port_entry,1,5,2,1);
  g_signal_connect(rx->serial_port_entry,"activate",G_CALLBACK(cat_serial_port_cb),rx);

  GtkWidget *serial_baudrate_label=gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(serial_baudrate_label), "<b>Baudrate: </b>");
  gtk_grid_attach(GTK_GRID(cat_grid),serial_baudrate_label,0,6,1,1);

  GtkWidget *cat_serial_port_baudrate=gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cat_serial_port_baudrate),NULL,"4800");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cat_serial_port_baudrate),NULL,"9600");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cat_serial_port_baudrate),NULL,"19200");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cat_serial_port_baudrate),NULL,"38400");
  if(rx->rigctl_serial_baudrate==B4800) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(cat_serial_port_baudrate),0);
  } else if(rx->rigctl_serial_baudrate==B9600) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(cat_serial_port_baudrate),1);
  } else if(rx->rigctl_serial_baudrate==B19200) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(cat_serial_port_baudrate),2);
  } else if(rx->rigctl_serial_baudrate==B38400) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(cat_serial_port_baudrate),3);
  }
  gtk_grid_attach(GTK_GRID(cat_grid),cat_serial_port_baudrate,1,6,1,1);
  g_signal_connect(cat_serial_port_baudrate,"changed",G_CALLBACK(cat_baudrate_cb),rx);
  return grid;
}
