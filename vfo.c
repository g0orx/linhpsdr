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
#include <wdsp.h>

#include "agc.h"
#include "mode.h"
#include "filter.h"
#include "discovered.h"
#include "receiver.h"
#include "receiver_dialog.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "radio.h"
#include "band.h"
#include "main.h"
#include "vfo.h"
#include "bookmark_dialog.h"
#include "rigctl.h"

enum {
  BUTTON_NONE=-1,
  BUTTON_LOCK=0,
  BUTTON_MODE,
  BUTTON_FILTER,
  BUTTON_NB,
  BUTTON_NR,
  BUTTON_SNB,
  BUTTON_ANF,
  BUTTON_AGC1,
  BUTTON_AGC2,
  BUTTON_BMK,
  BUTTON_CAT,
  BUTTON_RIT,
  VALUE_RIT,
  BUTTON_CTUN,
  BUTTON_ATOB,
  BUTTON_BTOA,
  BUTTON_ASWAPB,
  BUTTON_SPLIT,
  BUTTON_STEP,
  BUTTON_ZOOM,
  BUTTON_VFO,
  SLIDER_AF,
  SLIDER_AGC
};

typedef struct _choice {
  RECEIVER *rx;
  int selection;
} CHOICE;

typedef struct _step {
  gint64 step;
  char *label;
} STEP;

  
static gboolean vfo_configure_event_cb(GtkWidget *widget,GdkEventConfigure *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  gint width=gtk_widget_get_allocated_width(widget);
  gint height=gtk_widget_get_allocated_height(widget);
//g_print("vfo_configure_event_cb: width=%d height=%d\n",width,height);
  if(rx->vfo_surface) {
    cairo_surface_destroy(rx->vfo_surface);
  }
  rx->vfo_surface=gdk_window_create_similar_surface(gtk_widget_get_window(widget),CAIRO_CONTENT_COLOR,width,height);

  /* Initialize the surface to black */
  cairo_t *cr;
  cr = cairo_create (rx->vfo_surface);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_paint (cr);
  cairo_destroy(cr);
  return TRUE;
}


static gboolean vfo_draw_cb(GtkWidget *widget,cairo_t *cr,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(rx->vfo_surface!=NULL) {
    cairo_set_source_surface (cr, rx->vfo_surface, 0.0, 0.0);
    cairo_paint (cr);
  }
  return FALSE;
}

static int next_step(int current_step) {
  int next=100;
  switch(current_step) {
    case 1:
      next=5;
      break;
    case 5:
      next=10;
      break;
    case 10:
      next=25;
      break;
    case 25:
      next=50;
      break;
    case 50:
      next=100;
      break;
    case 100:
      next=250;
      break;
    case 250:
      next=500;
      break;
    case 500:
      next=1000;
      break;
    case 1000:
      next=2500;
      break;
    case 2500:
      next=5000;
      break;
    case 5000:
      next=6250;
      break;
    case 6250:
      next=9000;
      break;
    case 9000:
      next=10000;
      break;
    case 10000:
      next=100000;
      break;
    case 100000:
      next=1;
      break;
  }
  return next;
}

static int previous_step(int current_step) {
  int previous=100;
  switch(current_step) {
    case 1:
      previous=100000;
      break;
    case 5:
      previous=1;
      break;
    case 10:
      previous=5;
      break;
    case 25:
      previous=10;
      break;
    case 50:
      previous=25;
      break;
    case 100:
      previous=50;
      break;
    case 250:
      previous=100;
      break;
    case 500:
      previous=250;
      break;
    case 1000:
      previous=500;
      break;
    case 2500:
      previous=1000;
      break;
    case 5000:
      previous=2500;
      break;
    case 6250:
      previous=5000;
      break;
    case 9000:
      previous=6250;
      break;
    case 10000:
      previous=9000;
      break;
    case 100000:
      previous=10000;
      break;
  }
  return previous;
}

static int which_button(int x,int y) {
  int button=-1;
  if(y<=12) {
    if(x>=355 && x<430) {
      button=BUTTON_STEP;
    } else if(x>285 && x<355) {
      button=BUTTON_ZOOM;
    } else {
      if(x>=70 && x<210) {
        button=(x/35)-2+BUTTON_ATOB;
      }
    }
  } else if(y>=48) {
    button=(x-5)/35;
  } else if(x>=560) {
    if(y<=35) {
      button=SLIDER_AF;
    } else {
      button=SLIDER_AGC;
    }
  }
  if(y>12 && y<48){
    if(x>4 && x<364){
      button=BUTTON_VFO;
    }
  }

  return button;
}

void mode_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  receiver_mode_changed(choice->rx,choice->selection);
  if(radio->transmitter->rx==choice->rx) {
    if(choice->rx->split) {
      transmitter_set_mode(radio->transmitter,choice->rx->mode_b);
    } else {
      transmitter_set_mode(radio->transmitter,choice->rx->mode_a);
    }
  }
  update_vfo(choice->rx);
}

void filter_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  receiver_filter_changed(choice->rx,choice->selection);
  update_vfo(choice->rx);
}

void nb_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  switch(choice->selection) {
    case 0:
      choice->rx->nb=FALSE;
      choice->rx->nb2=FALSE;
      break;
    case 1:
      choice->rx->nb=TRUE;
      choice->rx->nb2=FALSE;
      break;
    case 2:
      choice->rx->nb=FALSE;
      choice->rx->nb2=TRUE;
      break;
  }
  update_noise(choice->rx);
}

void nr_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  switch(choice->selection) {
    case 0:
      choice->rx->nr=FALSE;
      choice->rx->nr2=FALSE;
      break;
    case 1:
      choice->rx->nr=TRUE;
      choice->rx->nr2=FALSE;
      break;
    case 2:
      choice->rx->nr=FALSE;
      choice->rx->nr2=TRUE;
      break;
  }
  update_noise(choice->rx);
}

void agc_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  choice->rx->agc=choice->selection;
  set_agc(choice->rx);
  update_vfo(choice->rx);
}

void zoom_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  choice->rx->zoom=choice->selection;
  choice->rx->pixels=choice->rx->panadapter_width*choice->rx->zoom;
  receiver_init_analyzer(choice->rx);
  update_vfo(choice->rx);
}

void band_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  BAND *b;
  int mode_a;
  long long frequency_a;
  long long lo_a=0LL;

  switch(choice->selection) {
    case band2200:
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      switch(choice->rx->mode_a) {
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
      break;
    case band220:
      break;
    case band430:
      break;
    case band902:
      break;
    case band1240:
      break;
    case band2300:
      break;
    case band3400:
      break;
    case bandAIR:
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
      b=band_get_band(choice->selection);
      mode_a=USB;
      frequency_a=b->frequencyMin;
      lo_a=b->frequencyLO;
      break;
  }
  choice->rx->mode_a=mode_a;
  choice->rx->frequency_a=frequency_a;
  choice->rx->lo_a=lo_a;
  choice->rx->ctun_offset=0;
  receiver_band_changed(choice->rx,band);
  if(radio->transmitter->rx==choice->rx) {
    if(choice->rx->split) {
      transmitter_set_mode(radio->transmitter,choice->rx->mode_b);
    } else {
      transmitter_set_mode(radio->transmitter,choice->rx->mode_a);
    }
  }

}

void step_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  choice->rx->step=choice->selection;
  update_vfo(choice->rx);
}

void rit_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  choice->rx->rit_step=choice->selection;
  receiver_init_analyzer(choice->rx);
  update_vfo(choice->rx);
}


static gboolean vfo_press_event_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  GtkWidget *menu;
  GtkWidget *menu_item;
  RECEIVER *rx=(RECEIVER *)data;
  long long temp_frequency;
  int temp_mode;
  int i;
  CHOICE *choice;
  FILTER *mode_filters;
  FILTER *filter;
  BAND *band;
  int x=(int)event->x;
  int y=(int)event->y;
  char text[32];

  switch(which_button(x,y)) {
    case BUTTON_LOCK:
      switch(event->button) {
        case 1:  // LEFT
            rx->locked=!rx->locked;
            update_vfo(rx);
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_MODE:
      switch(event->button) {
        case 1:  // LEFT
          menu=gtk_menu_new();
          for(i=0;i<MODES;i++) {
            menu_item=gtk_menu_item_new_with_label(mode_string[i]);
            choice=g_new0(CHOICE,1);
            choice->rx=rx;
            choice->selection=i;
            g_signal_connect(menu_item,"activate",G_CALLBACK(mode_cb),choice);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          }
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif

          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_FILTER:
      switch(event->button) {
        case 1:  // LEFT
          if(rx->mode_a==FMN) {
            menu=gtk_menu_new();
            menu_item=gtk_menu_item_new_with_label("2.5k Dev");
            choice=g_new0(CHOICE,1);
            choice->rx=rx;
            choice->selection=0;
            g_signal_connect(menu_item,"activate",G_CALLBACK(filter_cb),choice);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
            menu_item=gtk_menu_item_new_with_label("5.0k Dev");
            choice=g_new0(CHOICE,1);
            choice->rx=rx;
            choice->selection=1;
            g_signal_connect(menu_item,"activate",G_CALLBACK(filter_cb),choice);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          } else {
            mode_filters=filters[rx->mode_a];
            menu=gtk_menu_new();
            for(i=0;i<FILTERS;i++) {
              if(i>=FVar1) {
                sprintf(text,"%s (%d..%d)",mode_filters[i].title,mode_filters[i].low,mode_filters[i].high);
                menu_item=gtk_menu_item_new_with_label(text);
              } else {
                menu_item=gtk_menu_item_new_with_label(mode_filters[i].title);
              }
              choice=g_new0(CHOICE,1);
              choice->rx=rx;
              choice->selection=i;
              g_signal_connect(menu_item,"activate",G_CALLBACK(filter_cb),choice);
              gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
            }
          }
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_NB:
      switch(event->button) {
        case 1:  // LEFT
          menu=gtk_menu_new();
          menu_item=gtk_menu_item_new_with_label("OFF");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=0;
          g_signal_connect(menu_item,"activate",G_CALLBACK(nb_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("NB");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=1;
          g_signal_connect(menu_item,"activate",G_CALLBACK(nb_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("NB2");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=2;
          g_signal_connect(menu_item,"activate",G_CALLBACK(nb_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
          break;
        case 3:  // RIGHT
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=0;
          nb_cb(0, choice);
          break;
      }
      break;
    case BUTTON_NR:
      switch(event->button) {
        case 1:  // LEFT
          menu=gtk_menu_new();
          menu_item=gtk_menu_item_new_with_label("OFF");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=0;
          g_signal_connect(menu_item,"activate",G_CALLBACK(nr_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("NR");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=1;
          g_signal_connect(menu_item,"activate",G_CALLBACK(nr_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("NR2");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=2;
          g_signal_connect(menu_item,"activate",G_CALLBACK(nr_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
          break;
        case 3:  // RIGHT
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=0;
          nr_cb(0, choice);
          break;
      }
      break;
    case BUTTON_SNB:
      switch(event->button) {
        case 1:  // LEFT
          rx->snb=!rx->snb;
          SetRXASNBARun(rx->channel, rx->snb);
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_ANF:
      switch(event->button) {
        case 1:  // LEFT
          rx->anf=!rx->anf;
          SetRXAANFRun(rx->channel, rx->anf);
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_AGC1:
    case BUTTON_AGC2:
      switch(event->button) {
        case 1:  // LEFT
          menu=gtk_menu_new();
          menu_item=gtk_menu_item_new_with_label("OFF");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=AGC_OFF;
          g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("LONG");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=AGC_LONG;
          g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("SLOW");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=AGC_SLOW;
          g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("MEDIUM");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=AGC_MEDIUM;
          g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("FAST");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=AGC_FAST;
          g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
          break;
        case 3:  // RIGHT
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=AGC_OFF;
          agc_cb(0, choice);
          break;
      }
      break;
    case BUTTON_BMK:
      switch(event->button) {
        case 1:  // LEFT
          if(rx->bookmark_dialog==NULL) {
            rx->bookmark_dialog=create_bookmark_dialog(rx,VIEW_BOOKMARKS,NULL);
          }
          break;
        case 3:  // RIGHT
          if(rx->bookmark_dialog==NULL) {
            rx->bookmark_dialog=create_bookmark_dialog(rx,ADD_BOOKMARK,NULL);
          }
          break;
      }
      break;
    case BUTTON_CAT:
      break;
    case BUTTON_RIT:
      switch(event->button) {
        case 1:  // LEFT
          rx->rit_enabled=!rx->rit_enabled;
          break;
        case 3:  // RIGHT
          menu=gtk_menu_new();
          menu_item=gtk_menu_item_new_with_label("1Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=1;
          g_signal_connect(menu_item,"activate",G_CALLBACK(rit_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("5Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=5;
          g_signal_connect(menu_item,"activate",G_CALLBACK(rit_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("10Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=10;
          g_signal_connect(menu_item,"activate",G_CALLBACK(rit_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
          break;
      }
      break;
    case VALUE_RIT:
      switch(event->button) {
        case 1:  // LEFT
          break;
        case 3:  // RIGHT
          rx->rit=0;
          break;
      }
      break;
    case BUTTON_CTUN:
          rx->ctun=!rx->ctun;
          rx->ctun_offset=0;
      break;
    case BUTTON_ATOB:
      switch(event->button) {
        case 1:  // LEFT
          rx->frequency_b=rx->frequency_a;
          rx->mode_b=rx->mode_a;
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_BTOA:
      switch(event->button) {
        case 1:  // LEFT
          rx->frequency_a=rx->frequency_b;
          rx->mode_a=rx->mode_b;
          frequency_changed(rx);
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_ASWAPB:
      switch(event->button) {
        case 1:  // LEFT
          temp_frequency=rx->frequency_a;
          temp_mode=rx->mode_a;
          rx->frequency_a=rx->frequency_b;
          rx->mode_a=rx->mode_b;
          rx->frequency_b=temp_frequency;
          rx->mode_b=temp_mode;
          frequency_changed(rx);
          receiver_mode_changed(rx,rx->mode_a);
          if(radio->transmitter->rx==rx) {
            if(rx->split) {
              transmitter_set_mode(radio->transmitter,rx->mode_b);
            } else {
              transmitter_set_mode(radio->transmitter,rx->mode_a);
            }
          }
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_SPLIT:
      switch(event->button) {
        case 1:  // LEFT
          rx->split=!rx->split;
          frequency_changed(rx);
          if(radio->transmitter->rx==rx) {
            if(rx->split) {
              transmitter_set_mode(radio->transmitter,rx->mode_b);
            } else {
              transmitter_set_mode(radio->transmitter,rx->mode_a);
            }
          }
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_STEP:
      switch(event->button) {
        case 1:  // LEFT
          menu=gtk_menu_new();
          menu_item=gtk_menu_item_new_with_label("1Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=1;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("10Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=10;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("25Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=25;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("50Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=50;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("100Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=100;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("250Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=250;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("500Hz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=500;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("1kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=1000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("2kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=2000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("2.5kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=2500;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("5kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=5000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("6.25kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=6250;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("9kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=9000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("10kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=10000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("12.5kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=12500;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("15kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=15000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("20kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=20000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("25kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=25000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("30kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=30000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("50kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=50000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("100kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=100000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("250kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=250000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("500kHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=500000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("1MHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=1000000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("10MHz");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=10000000;
          g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
          break;
        case 3:  // RIGHT
          break;
      }
      break;
    case BUTTON_ZOOM:
      switch(event->button) {
        case 1:  // LEFT
          menu=gtk_menu_new();
          menu_item=gtk_menu_item_new_with_label("x1");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=1;
          g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("x3");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=3;
          g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("x5");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=5;
          g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          menu_item=gtk_menu_item_new_with_label("x7");
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=7;
          g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
          gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
          gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
          gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
          break;
        case 3:  // RIGHT
          break;
      }
      break;
      
    case BUTTON_VFO:
      if(x>4 && x<212) {
        switch(event->button) {
          case 1: //LEFT
            if(!rx->locked) {
              menu=gtk_menu_new();
              for(i=0;i<BANDS+XVTRS;i++) {
#ifdef SOAPYSDR
                if(radio->discovered->protocol!=PROTOCOL_SOAPYSDR) {
                  if(i>=band70 && i<=band3400) {
                    continue;
                  }
                }
#endif
                band=(BAND*)band_get_band(i);
                if(strlen(band->title)>0) {
                menu_item=gtk_menu_item_new_with_label(band->title);
                choice=g_new0(CHOICE,1);
                choice->rx=rx;
                choice->selection=i;
                g_signal_connect(menu_item,"activate",G_CALLBACK(band_cb),choice);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
                }
              }
              gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
              gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
              gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
            }
            break;
          case 3: //RIGHT
            break;
        }
      }
      break;
    case SLIDER_AF:
      rx->volume=(double)(x-560)/100.0;
      if(rx->volume>1.0) rx->volume=1.0;
      if(rx->volume<0.0) rx->volume=0.0;
      SetRXAPanelGain1(rx->channel,rx->volume);
      update_vfo(rx);
      break;
    case SLIDER_AGC:
      rx->agc_gain=(double)(x-560)-20.0;
      if(rx->agc_gain>120.0) rx->agc_gain=120.0;
      if(rx->agc_gain<-20.0) rx->agc_gain=-20.0;
      SetRXAAGCTop(rx->channel, rx->agc_gain);
      update_vfo(rx);
      break;
    default:
      switch(event->button) {
        case 1:  // LEFT
          break;
        case 3:  // RIGHT
          if(rx->dialog==NULL) {
            rx->dialog=create_receiver_dialog(rx);
          }
          break;
      }
      break;
  }
  update_vfo(rx);
  return TRUE;
}

static gboolean vfo_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  int x=(int)event->x;
  int y=(int)event->y;

  switch(which_button(x,y)) {
    case VALUE_RIT:
      if(rx->rit_enabled) {
        if(event->direction==GDK_SCROLL_UP) {
          rx->rit=rx->rit+rx->rit_step;
          if(rx->rit>1000.) rx->rit=1000;
        } else {
          rx->rit=rx->rit-rx->rit_step;
          if(rx->rit<-1000) rx->rit=-1000;
        }
      }
      break;
    case SLIDER_AF:
      if(event->direction==GDK_SCROLL_UP) {
        rx->volume=rx->volume+0.01;
        if(rx->volume>1.0) rx->volume=1.0;
      } else {
        rx->volume=rx->volume-0.01;
        if(rx->volume<0.0) rx->volume=0.0;
      }
      SetRXAPanelGain1(rx->channel,rx->volume);
      break;
    case SLIDER_AGC:
      if(event->direction==GDK_SCROLL_UP) {
        rx->agc_gain=rx->agc_gain+1.0;
        if(rx->agc_gain>120.0) rx->agc_gain=120.0;
      } else {
        rx->agc_gain=rx->agc_gain-1.0;
        if(rx->agc_gain<-20.0) rx->agc_gain=-20.0;
      }
      SetRXAAGCTop(rx->channel, rx->agc_gain);
      break;
    case BUTTON_VFO:
      if(x>4 && x<212) {
        if(!rx->locked) {
          int digit=(x-5)/17;
          long long step=0LL;
          switch(digit) {
            case 0:
              step=10000000000LL;
              break;
            case 1:
              step=1000000000LL;
              break;
            case 2:
              step=100000000LL;
              break;
            case 3:
              step=10000000LL;
              break;
            case 4:
              step=1000000LL;
              break;
            case 5:
              step=0LL;
              break;
            case 6:
              step=100000LL;
              break;
            case 7:
              step=10000LL;
              break;
            case 8:
              step=1000LL;
              break;
            case 9:
              step=0LL;
              break;
            case 10:
              step=100LL;
              break;
            case 11:
              step=10LL;
              break;
            case 12:
              step=1LL;
              break;
          }
          if(event->direction==GDK_SCROLL_UP) {
            rx->frequency_a+=step;
          } else {
            rx->frequency_a-=step;
          }
          frequency_changed(rx);
        }
      } else if(x>220 && x<364) {
        if(!rx->locked) {
          int digit=(x-220)/12;
          long long step=0LL;
          switch(digit) {
            case 0:
              step=10000000000LL;
              break;
            case 1:
              step=1000000000LL;
              break;
            case 2:
              step=100000000LL;
              break;
            case 3:
              step=10000000LL;
              break;
            case 4:
              step=1000000LL;
              break;
            case 5:
              step=0LL;
              break;
            case 6:
              step=100000LL;
              break;
            case 7:
              step=10000LL;
              break;
            case 8:
              step=1000LL;
              break;
            case 9:
              step=0LL;
              break;
            case 10:
              step=100LL;
              break;
            case 11:
              step=10LL;
              break;
            case 12:
              step=1LL;
              break;
          }
          if(event->direction==GDK_SCROLL_UP) {
            rx->frequency_b+=step;
          } else {
            rx->frequency_b-=step;
          }
          frequency_changed(rx);
        }
      }
      break;
  }
  update_vfo(rx);
  return TRUE;
}

static gboolean vfo_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  gint x,y;
  GdkModifierType state;
  RECEIVER *rx=(RECEIVER *)data;
  if(!rx->locked) {
    gdk_window_get_device_position(event->window,event->device,&x,&y,&state);
    if(y>12 && y<48) {
      if(x>4 && x<212) {
        int digit=(x-5)/17;
        switch(digit) {
          //case 0:
          //case 1:
          case 2:
          case 3:
          case 5:
          case 6:
          case 7:
          case 9:
          case 10:
          case 11:
            gdk_window_set_cursor(gtk_widget_get_window(widget),gdk_cursor_new(GDK_DOUBLE_ARROW));
            break;
          default:
            gdk_window_set_cursor(gtk_widget_get_window(widget),gdk_cursor_new(GDK_ARROW));
            break;
        }
      } else if(x>220 && x<364) {
        int digit=(x-220)/12;
        switch(digit) {
          //case 0:
          //case 1:
          case 2:
          case 3:
          case 5:
          case 6:
          case 7:
          case 9:
          case 10:
          case 11:
            gdk_window_set_cursor(gtk_widget_get_window(widget),gdk_cursor_new(GDK_DOUBLE_ARROW));
            break;
          default:
            gdk_window_set_cursor(gtk_widget_get_window(widget),gdk_cursor_new(GDK_ARROW));
            break;
        }
      }
    } else {
      gdk_window_set_cursor(gtk_widget_get_window(widget),gdk_cursor_new(GDK_ARROW));
    }
  }
  return TRUE;
}

GtkWidget *create_vfo(RECEIVER *rx) {
  g_print("create_vfo: rx=%d\n",rx->channel);
  rx->vfo_surface=NULL;
  GtkWidget *vfo = gtk_drawing_area_new ();
  g_signal_connect (vfo,"configure-event",G_CALLBACK(vfo_configure_event_cb),(gpointer)rx);
  g_signal_connect (vfo,"draw",G_CALLBACK(vfo_draw_cb),(gpointer)rx);
  g_signal_connect(vfo,"motion-notify-event",G_CALLBACK(vfo_motion_notify_event_cb),rx);
  g_signal_connect (vfo,"button-press-event",G_CALLBACK(vfo_press_event_cb),(gpointer)rx);
  g_signal_connect(vfo,"scroll_event",G_CALLBACK(vfo_scroll_event_cb),(gpointer)rx);
  gtk_widget_set_events (vfo, gtk_widget_get_events (vfo)
                     | GDK_BUTTON_PRESS_MASK
                     | GDK_SCROLL_MASK
                     | GDK_POINTER_MOTION_MASK
                     | GDK_POINTER_MOTION_HINT_MASK);
  return vfo;
}

void update_vfo(RECEIVER *rx) {
  char temp[32];
  cairo_t *cr;

  if(rx!=NULL && rx->vfo_surface!=NULL) {
    cr = cairo_create (rx->vfo_surface);
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_paint (cr);
    long long af=rx->frequency_a+rx->ctun_offset;
    long long bf=rx->frequency_b;

    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    if(radio!=NULL && radio->transmitter!=NULL && rx==radio->transmitter->rx && !radio->transmitter->rx->split) {
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    }
    cairo_move_to(cr, 5, 12);
    cairo_set_font_size(cr, 12);
    cairo_show_text(cr, "VFO A");

    if(radio!=NULL && radio->transmitter!=NULL && rx==radio->transmitter->rx && !radio->transmitter->rx->split && isTransmitting(radio)) {
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    }
    sprintf(temp,"%5lld.%03lld.%03lld",af/(long long)1000000,(af%(long long)1000000)/(long long)1000,af%(long long)1000);
    cairo_move_to(cr, 5, 38);
    cairo_set_font_size(cr, 28);
    cairo_show_text(cr, temp);

    if(radio!=NULL && radio->transmitter!=NULL && rx==radio->transmitter->rx && radio->transmitter->rx->split) {
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    }
    cairo_move_to(cr, 220, 12);
    cairo_set_font_size(cr, 12);
    cairo_show_text(cr, "VFO B");

    if(radio!=NULL && radio->transmitter!=NULL && rx==radio->transmitter->rx && radio->transmitter->rx->split && isTransmitting(radio)) {
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    }
    sprintf(temp,"%5lld.%03lld.%03lld",bf/(long long)1000000,(bf%(long long)1000000)/(long long)1000,bf%(long long)1000);
    cairo_move_to(cr, 220, 38);
    cairo_set_font_size(cr, 20);
    cairo_show_text(cr, temp);

    FILTER* band_filters=filters[rx->mode_a];
    FILTER* band_filter=&band_filters[rx->filter_a];
    cairo_set_font_size(cr, 12);

    int x=5;

    cairo_move_to(cr, x, 58);
    if(rx->locked) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_show_text(cr, "Lock");
    x+=35;

    cairo_move_to(cr, x, 58);
    cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
    cairo_show_text(cr, mode_string[rx->mode_a]);
    x+=35;

    cairo_move_to(cr, x, 58);
    if(rx->mode_a==FMN) {
      if(rx->deviation==2500) {
        cairo_show_text(cr, "8000");
      } else {
        cairo_show_text(cr, "16000");
      }
    } else {
      cairo_show_text(cr, band_filters[rx->filter_a].title);
    }
    x+=35;

    cairo_move_to(cr, x, 58);
    if(rx->nb) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
      cairo_show_text(cr, "NB");
    } else if(rx->nb2) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
      cairo_show_text(cr, "NB2");
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
      cairo_show_text(cr, "NB");
    }
    x+=35;

    cairo_move_to(cr, x, 58);
    if(rx->nr) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
      cairo_show_text(cr, "NR");
    } else if(rx->nr2) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
      cairo_show_text(cr, "NR2");
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
      cairo_show_text(cr, "NR");
    }
    x+=35;


    if(rx->snb) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_move_to(cr, x, 58);
    cairo_show_text(cr, "SNB");
    x+=35;

    if(rx->anf) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_move_to(cr, x, 58);
    cairo_show_text(cr, "ANF");
    x+=35;

    cairo_move_to(cr, x, 58);
    switch(rx->agc) {
      case AGC_OFF:
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_show_text(cr, "AGC OFF");
        break;
      case AGC_LONG:
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_show_text(cr, "AGC LONG");
        break;
      case AGC_SLOW:
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_show_text(cr, "AGC SLOW");
        break;
      case AGC_MEDIUM:
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_show_text(cr, "AGC MED");
        break;
      case AGC_FAST:
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_show_text(cr, "AGC FAST");
        break;
    }
    x+=35;
    x+=35;
    
    cairo_move_to(cr, x, 58);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_show_text(cr, "BMK");
    x+=35;

    cairo_move_to(cr, x, 58);
    if(rx->cat_control>-1) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_show_text(cr, "CAT");
    x+=35;

    cairo_move_to(cr,x,58);
    if(rx->rit_enabled) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_show_text(cr, "RIT");
    x+=35;
 
    if(rx->rit_enabled) {
      cairo_move_to(cr,x,58);
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
      sprintf(temp,"%ld",rx->rit);
      cairo_show_text(cr, temp);
    }
    x+=35;

    cairo_move_to(cr,x,58);
    if(rx->ctun) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_show_text(cr, "CTUN");
    x+=35;

    x=70;
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr, x, 12);
    cairo_show_text(cr, "A>B");
    x+=35;

    cairo_move_to(cr, x, 12);
    cairo_show_text(cr, "A<B");
    x+=35;

    cairo_move_to(cr, x, 12);
    cairo_show_text(cr, "A<>B");
    x+=35;

    cairo_move_to(cr, x, 12);
    if(rx->split) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_show_text(cr, "Split");
    x+=35;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr,285,12);
    sprintf(temp,"Zoom x%d",rx->zoom);
    cairo_show_text(cr, temp);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr,355,12);
    sprintf(temp,"Step %ld Hz",rx->step);
    cairo_show_text(cr, temp);


    x=500;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_font_size(cr,10);
    cairo_move_to(cr,x,30);
    cairo_show_text(cr,"AF Gain");
    x+=60;

    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    cairo_rectangle(cr,x,25,rx->volume*100,5);
    cairo_fill(cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr,x,30);
    cairo_line_to(cr,x+100,30);
    cairo_stroke(cr);

    cairo_move_to(cr,x,30);
    cairo_line_to(cr,x,24);
    cairo_stroke(cr);
    x+=25;
    cairo_move_to(cr,x,30);
    cairo_line_to(cr,x,27);
    cairo_stroke(cr);
    x+=25;
    cairo_move_to(cr,x,30);
    cairo_line_to(cr,x,24);
    cairo_stroke(cr);
    x+=25;
    cairo_move_to(cr,x,30);
    cairo_line_to(cr,x,27);
    cairo_stroke(cr);
    x+=25;
    cairo_move_to(cr,x,30);
    cairo_line_to(cr,x,24);
    cairo_stroke(cr);


    x=500;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_font_size(cr,10);
    cairo_move_to(cr,x,45);
    cairo_show_text(cr,"AGC Gain");
    x+=60;

    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    cairo_rectangle(cr,x,40,rx->agc_gain+20.0,5);
    cairo_fill(cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x+140,45);
    cairo_stroke(cr);

    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,42);
    cairo_stroke(cr);
    x+=20;
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,39);
    cairo_stroke(cr);
    x+=20;
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,42);
    cairo_stroke(cr);
    x+=20;
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,42);
    cairo_stroke(cr);
    x+=20;
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,42);
    cairo_stroke(cr);
    x+=20;
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,42);
    cairo_stroke(cr);
    x+=20;
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,42);
    cairo_stroke(cr);
    x+=20;
    cairo_move_to(cr,x,45);
    cairo_line_to(cr,x,39);
    cairo_stroke(cr);

    cairo_destroy (cr);
    gtk_widget_queue_draw (rx->vfo);
  }
}
