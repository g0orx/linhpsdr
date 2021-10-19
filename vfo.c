/* 2018 - John Melton, G0ORX/N6LYT
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
#include "bpsk.h"
#include "receiver.h"
#include "receiver_dialog.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "band.h"
#include "main.h"
#include "vfo.h"
#include "bookmark_dialog.h"
#include "rigctl.h"
#include "bpsk.h"
#include "subrx.h"

typedef struct _choice {
  RECEIVER *rx;
  int selection;
  int sub_selection;
  GtkWidget *button;
} CHOICE;

typedef struct _step {
  gint64 step;
  char *label;
} STEP;

const long long ll_step[13]= {
   10000000000LL,
   1000000000LL,
   100000000LL,
   10000000LL,
   1000000LL,
   0LL,
   100000LL,
   10000LL,
   1000LL,
   0LL,
   100LL,
   10LL,
   1LL
};

gint64 steps[STEPS]={1,10,25,50,100,250,500,1000,5000,9000,10000,100000,250000,500000,1000000};
char *step_labels[STEPS]={"1Hz","10Hz","25Hz","50Hz","100Hz","250Hz","500Hz","1kHz","5kHz","9kHz","10kHz","100kHz","250KHz","500KHz","1MHz"};
   
static gboolean pressed=FALSE;
static gdouble last_x;
static gboolean has_moved=FALSE;

static int get_step(gint64 step) {
  int i;
  for(i=0;i<STEPS;i++) {
    if(steps[i]==step) {
      return i;
    }
  }
  return 4; // 100Hz
}

void vfo_a2b(RECEIVER *rx) {
  int m=rx->mode_b;
  int f=rx->filter_b;
  rx->band_b=rx->band_a;
  rx->frequency_b=rx->frequency_a;
  rx->mode_b=rx->mode_a;
  rx->filter_b=rx->filter_a;
  rx->filter_low_b=rx->filter_low_a;
  rx->filter_high_b=rx->filter_high_a;
  rx->lo_b=rx->lo_a;
  rx->error_b=rx->error_a;
  frequency_changed(rx);
  if(rx->subrx_enable) {
    if(m!=rx->mode_b) {
      subrx_mode_changed(rx);
    } else if(f!=rx->filter_b) {
      subrx_filter_changed(rx);
    }
  }
  update_vfo(rx);
}

static void a2b_cb(GtkButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  vfo_a2b(rx);
}

void vfo_b2a(RECEIVER *rx) {
  int m=rx->mode_a;
  int f=rx->filter_a;
  rx->band_a=rx->band_b;
  rx->frequency_a=rx->frequency_b;
  rx->mode_a=rx->mode_b;
  rx->filter_a=rx->filter_b;
  rx->filter_low_a=rx->filter_low_b;
  rx->filter_high_a=rx->filter_high_b;
  rx->lo_a=rx->lo_b;
  rx->error_a=rx->error_b;
  frequency_changed(rx);
  if(m!=rx->mode_a) {
    receiver_mode_changed(rx,m);
  } else if(f!=rx->filter_a) {
    receiver_filter_changed(rx,f);
  }
  update_vfo(rx);
}

static void b2a_cb(GtkButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  vfo_b2a(rx);
}

void vfo_aswapb(RECEIVER *rx) {
  gint temp_band;
  gint64 temp_frequency;
  gint temp_mode;
  gint temp_filter_low;
  gint temp_filter_high;
  gint temp_filter;
  gint64 temp_lo;
  gint64 temp_error;

  temp_band=rx->band_a;
  temp_frequency=rx->frequency_a;
  temp_mode=rx->mode_a;
  temp_filter=rx->filter_a;
  temp_filter_low=rx->filter_low_a;
  temp_filter_high=rx->filter_high_a;
  temp_lo=rx->lo_a;
  temp_error=rx->error_a;

  rx->band_a=rx->band_b;
  rx->frequency_a=rx->frequency_b;
  rx->mode_a=rx->mode_b;
  rx->filter_a=rx->filter_b;
  rx->filter_low_a=rx->filter_low_b;
  rx->filter_high_a=rx->filter_high_b;
  rx->lo_a=rx->lo_b;
  rx->error_a=rx->error_b;

  rx->band_b=temp_band;
  rx->frequency_b=temp_frequency;
  rx->mode_b=temp_mode;
  rx->filter_b=temp_filter;
  rx->filter_low_b=temp_filter_low;
  rx->filter_high_b=temp_filter_high;
  rx->lo_b=temp_lo;
  rx->lo_b=temp_lo;
  rx->lo_b=temp_lo;
  rx->error_b=temp_error;

  frequency_changed(rx);
  receiver_mode_changed(rx,rx->mode_a);
  if(radio->transmitter!=NULL && radio->transmitter->rx==rx) {
    if(rx->split!=SPLIT_OFF) {
      transmitter_set_mode(radio->transmitter,rx->mode_b);
    } else {
      transmitter_set_mode(radio->transmitter,rx->mode_a);
    }
  }

  if(rx->subrx_enable) {
    if(rx->mode_b!=rx->mode_a) {
      subrx_mode_changed(rx);
    } else if(rx->filter_b!=rx->filter_a) {
      subrx_filter_changed(rx);
    }
  }

  update_vfo(rx);
}

static void aswapb_cb(GtkButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  vfo_aswapb(rx);
}

static void split_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->split=rx->split==SPLIT_OFF?SPLIT_ON:SPLIT_OFF;
  g_signal_handlers_block_by_func(widget,G_CALLBACK(split_b_cb),user_data);
  gtk_toggle_button_set_active(widget,rx->split!=SPLIT_OFF);
  g_signal_handlers_unblock_by_func(widget,G_CALLBACK(split_b_cb),user_data);
  gtk_button_set_label(GTK_BUTTON(widget),"SPLIT");
  update_vfo(rx);
}

void split_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  RECEIVER *rx=choice->rx;
  rx->split=choice->selection;
  switch(rx->split) {
     case SPLIT_OFF:
     case SPLIT_ON:
       gtk_button_set_label(GTK_BUTTON(choice->button),"SPLIT");
       break;
     case SPLIT_SAT:
       gtk_button_set_label(GTK_BUTTON(choice->button),"SAT");
       break;
     case SPLIT_RSAT:
       gtk_button_set_label(GTK_BUTTON(choice->button),"RSAT");
       break;
  }
  g_signal_handlers_block_by_func(choice->button,G_CALLBACK(split_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(choice->button),rx->split!=SPLIT_OFF);
  g_signal_handlers_unblock_by_func(choice->button,G_CALLBACK(split_b_cb),rx);
  frequency_changed(rx);
  if(radio->transmitter && radio->transmitter->rx==rx) {
    switch(rx->split) {
      case SPLIT_OFF:
        transmitter_set_mode(radio->transmitter,rx->mode_a);
        break;
      case SPLIT_ON:
        // Split mode in CW, RX on VFO A, TX on VFO B.
        // When mode turned on, default to VFO A +1 kHz
        if (rx->mode_a == CWL || rx->mode_a ==CWU) {
          // Most pile-ups start with UP 1
          rx->frequency_b = rx->frequency_a + 1000;
          rx->mode_b=rx->mode_a;
        }
        transmitter_set_mode(radio->transmitter,rx->mode_b);
        break;
      case SPLIT_SAT:
      case SPLIT_RSAT:
        transmitter_set_mode(radio->transmitter,rx->mode_b);
        break;
    }
  }
  g_free(choice);
}

static gboolean split_b_press_cb(GtkWidget *widget,GdkEvent *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  switch(((GdkEventButton*)event)->button) {
    case 3:  // RIGHT
      menu=gtk_menu_new();
      menu_item=gtk_menu_item_new_with_label("Off");
      choice=g_new0(CHOICE,1);
      choice->rx=rx;
      choice->selection=SPLIT_OFF;
      choice->button=widget;
      g_signal_connect(menu_item,"activate",G_CALLBACK(split_cb),choice);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      menu_item=gtk_menu_item_new_with_label("SPLIT");
      choice=g_new0(CHOICE,1);
      choice->rx=rx;
      choice->selection=SPLIT_ON;
      choice->button=widget;
      g_signal_connect(menu_item,"activate",G_CALLBACK(split_cb),choice);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      menu_item=gtk_menu_item_new_with_label("SAT");
      choice=g_new0(CHOICE,1);
      choice->rx=rx;
      choice->selection=SPLIT_SAT;
      choice->button=widget;
      g_signal_connect(menu_item,"activate",G_CALLBACK(split_cb),choice);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      menu_item=gtk_menu_item_new_with_label("RSAT");
      choice=g_new0(CHOICE,1);
      choice->rx=rx;
      choice->selection=SPLIT_RSAT;
      choice->button=widget;
      g_signal_connect(menu_item,"activate",G_CALLBACK(split_cb),choice);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
      gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
      gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
      return TRUE;
      break;
  }
  return FALSE;
}

void zoom_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  RECEIVER *rx=choice->rx;
  char temp[16];
  receiver_change_zoom(rx,choice->selection);
  sprintf(temp,"ZOOM x%d",rx->zoom);
  gtk_button_set_label(GTK_BUTTON(choice->button),temp);
  g_free(choice);
}

static void zoom_b_cb(GtkWidget *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label("x1");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=1;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("x2");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=2;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("x3");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=3;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("x4");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=4;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("x5");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=5;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("x6");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=6;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("x7");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=7;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("x8");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=8;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(zoom_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(GTK_MENU(menu),NULL);
#else
  gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,NULL);
#endif
}

void step_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  choice->rx->step=steps[choice->selection];
  char temp[16];
  sprintf(temp,"STEP %s",step_labels[choice->selection]);
  gtk_button_set_label(GTK_BUTTON(choice->button),temp);
  g_free(choice);
}

static void step_b_cb(GtkWidget *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  int i;

  menu=gtk_menu_new();
  for(i=0;i<STEPS;i++) {
    menu_item=gtk_menu_item_new_with_label(step_labels[i]);
    choice=g_new0(CHOICE,1);
    choice->rx=rx;
    choice->selection=i;
    choice->button=widget;
    g_signal_connect(menu_item,"activate",G_CALLBACK(step_cb),choice);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  }
  gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(GTK_MENU(menu),NULL);
#else
  gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,NULL);
#endif
}

static void subrx_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  if(rx->subrx_enable) {
    rx->subrx_enable=FALSE;
    destroy_subrx(rx);
    rx->subrx=NULL;
  } else {
    create_subrx(rx);
    rx->subrx_enable=TRUE;
  }
  update_vfo(rx);
}

static void lock_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->locked=gtk_toggle_button_get_active(widget);
}

void mode_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  receiver_mode_changed(choice->rx,choice->selection);
  if(choice->rx->split!=SPLIT_OFF) {
    choice->rx->mode_b=choice->selection;
  }
  if(radio->transmitter!=NULL && radio->transmitter->rx==choice->rx) {
    if(choice->rx->split!=SPLIT_OFF) {
      transmitter_set_mode(radio->transmitter,choice->rx->mode_b);
    } else {
      transmitter_set_mode(radio->transmitter,choice->rx->mode_a);
    }
  }
  gtk_button_set_label(GTK_BUTTON(choice->button),mode_string[choice->selection]);
  g_free(choice);
}

static void mode_b_cb(GtkButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  int i;

  for(i=0;i<MODES;i++) {
    menu_item=gtk_menu_item_new_with_label(mode_string[i]);
    choice=g_new0(CHOICE,1);
    choice->rx=rx;
    choice->selection=i;
    choice->button=(GtkWidget *)widget;
    g_signal_connect(menu_item,"activate",G_CALLBACK(mode_cb),choice);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    }
  gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(GTK_MENU(menu),NULL);
#else
  gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,NULL);
#endif
}

void filter_cb(GtkWidget *menu_item,gpointer data) {
  FILTER *mode_filters;
  CHOICE *choice=(CHOICE *)data;
  receiver_filter_changed(choice->rx,choice->selection);
  mode_filters=filters[choice->rx->mode_a];
  gtk_button_set_label(GTK_BUTTON(choice->button),mode_filters[choice->selection].title);
  g_free(choice);
}

static void filter_b_cb(GtkButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  FILTER *mode_filters;
  char text[32];
  int i;
  if(rx->mode_a==FMN) {
    menu=gtk_menu_new();
    menu_item=gtk_menu_item_new_with_label("2.5k Dev");
    choice=g_new0(CHOICE,1);
    choice->rx=rx;
    choice->selection=0;
    choice->button=(GtkWidget *)widget;
    g_signal_connect(menu_item,"activate",G_CALLBACK(filter_cb),choice);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    menu_item=gtk_menu_item_new_with_label("5.0k Dev");
    choice=g_new0(CHOICE,1);
    choice->rx=rx;
    choice->selection=1;
    choice->button=(GtkWidget *)widget;
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
      choice->button=(GtkWidget *)widget;
      g_signal_connect(menu_item,"activate",G_CALLBACK(filter_cb),choice);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    }
  }
  gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(GTK_MENU(menu),NULL);
#else
  gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,NULL);
#endif
}

static gboolean nb_b_pressed_cb(GtkWidget *widget,GdkEventButton *event,gpointer user_data);

void nb_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)choice->rx->vfo,"vfo_data");
    
  switch(choice->selection) {
    case 0:
      choice->rx->nb=FALSE;
      choice->rx->nb2=FALSE;
      gtk_button_set_label(GTK_BUTTON(v->nb_b),"NB");
      break;
    case 1:
      choice->rx->nb=TRUE;
      choice->rx->nb2=FALSE;
      gtk_button_set_label(GTK_BUTTON(v->nb_b),"NB");
      break;
    case 2:
      choice->rx->nb=FALSE;
      choice->rx->nb2=TRUE;
      gtk_button_set_label(GTK_BUTTON(v->nb_b),"NB2");
      break;
  }
  g_signal_handlers_block_by_func(v->nb_b,G_CALLBACK(nb_b_pressed_cb),data);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->nb_b),choice->rx->nb|choice->rx->nb2);
  g_signal_handlers_unblock_by_func(v->nb_b,G_CALLBACK(nb_b_pressed_cb),data);

  update_noise(choice->rx);
  
  g_free(choice);
}

static gboolean nb_b_pressed_cb(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu;
  GtkWidget *menu_item;
  CHOICE *choice;

  menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label("OFF");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=0;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(nb_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("NB");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=1;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(nb_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("NB2");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=2;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(nb_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
  gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
  return TRUE;
}

static gboolean nr_b_pressed_cb(GtkWidget *widget,GdkEventButton *event,gpointer user_data);

void nr_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)choice->rx->vfo,"vfo_data");

  switch(choice->selection) {
    case 0:
      choice->rx->nr=FALSE;
      choice->rx->nr2=FALSE;
      gtk_button_set_label(GTK_BUTTON(v->nr_b),"NR");
      break;
    case 1:
      choice->rx->nr=TRUE;
      choice->rx->nr2=FALSE;
      gtk_button_set_label(GTK_BUTTON(v->nr_b),"NR");
      break;
    case 2:
      choice->rx->nr=FALSE;
      choice->rx->nr2=TRUE;
      gtk_button_set_label(GTK_BUTTON(v->nr_b),"NR2");
      break;
  }
  g_signal_handlers_block_by_func(v->nr_b,G_CALLBACK(nr_b_pressed_cb),data);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->nr_b),choice->rx->nr|choice->rx->nr2);
  g_signal_handlers_unblock_by_func(v->nr_b,G_CALLBACK(nr_b_pressed_cb),data);

  update_noise(choice->rx);

  g_free(choice);
}

static gboolean nr_b_pressed_cb(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu;
  GtkWidget *menu_item;
  CHOICE *choice;

  menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label("OFF");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=0;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(nr_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("NR");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=1;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(nr_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("NR2");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=2;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(nr_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
  gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
  return TRUE;
}

static void snb_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->snb=gtk_toggle_button_get_active(widget);
  update_noise(rx);  
}

static void anf_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->anf=gtk_toggle_button_get_active(widget);
  update_noise(rx);
}

static void bpsk_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->bpsk_enable=gtk_toggle_button_get_active(widget);
  if(rx->bpsk_enable) {
    rx->bpsk=create_bpsk(BPSK_CHANNEL,rx->band_a);
    rx->bpsk_enable=TRUE;
  } else {
    destroy_bpsk(rx->bpsk);
    rx->bpsk=NULL;
    rx->bpsk_enable=FALSE;
  }
}

static void rit_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->rit_enabled=gtk_toggle_button_get_active(widget);
  frequency_changed(rx);
  update_frequency(rx);
}

static gboolean rit_b_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  char text[16];
  if(event->direction==GDK_SCROLL_UP) {
    rx->rit=rx->rit+rx->rit_step;
    if(rx->rit>10000) rx->rit=10000;
  } else {
    rx->rit=rx->rit-rx->rit_step;
    if(rx->rit<-10000) rx->rit=-10000;
  }
  sprintf(text,"%+05ld",rx->rit); 
  gtk_label_set_text(GTK_LABEL(v->rit_value),text);
  frequency_changed(rx);
  update_frequency(rx);
  return TRUE;
}

void rit_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  choice->rx->rit_step=choice->selection;
  g_free(choice);
}

static gboolean rit_b_press_event_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  if(event->button==3) {  // RIGHT
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

    menu_item=gtk_menu_item_new_with_label("100Hz");
    choice=g_new0(CHOICE,1);
    choice->rx=rx;
    choice->selection=100;
    g_signal_connect(menu_item,"activate",G_CALLBACK(rit_cb),choice);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

    menu_item=gtk_menu_item_new_with_label("1000Hz");
    choice=g_new0(CHOICE,1);
    choice->rx=rx;
    choice->selection=1000;
    g_signal_connect(menu_item,"activate",G_CALLBACK(rit_cb),choice);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

    gtk_widget_show_all(menu);
    gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
    gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
    gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
    return TRUE;
  }
  return FALSE;
}


static gboolean rit_b_press_cb(GtkWidget *widget,GdkEvent *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  switch(((GdkEventButton*)event)->button) {
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

     menu_item=gtk_menu_item_new_with_label("100Hz");
     choice=g_new0(CHOICE,1);
     choice->rx=rx;
     choice->selection=100;
     g_signal_connect(menu_item,"activate",G_CALLBACK(rit_cb),choice);
     gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

     menu_item=gtk_menu_item_new_with_label("1000Hz");
     choice=g_new0(CHOICE,1);
     choice->rx=rx;
     choice->selection=1000;
     g_signal_connect(menu_item,"activate",G_CALLBACK(rit_cb),choice);
     gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

     gtk_widget_show_all(menu);
     gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
     gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
     gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
      return TRUE;
      break;
  }
  return FALSE;
}

static void xit_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  if(radio->transmitter!=NULL && radio->transmitter->rx==rx) {
    radio->transmitter->xit_enabled=gtk_toggle_button_get_active(widget);
  }
}

static gboolean xit_b_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  char text[16];
  if(radio->transmitter!=NULL && radio->transmitter->rx==rx) {
    if(event->direction==GDK_SCROLL_UP) {
      radio->transmitter->xit=radio->transmitter->xit+radio->transmitter->xit_step;
      if(radio->transmitter->xit>10000) radio->transmitter->xit=10000;
    } else {
      radio->transmitter->xit=radio->transmitter->xit-radio->transmitter->xit_step;
      if(radio->transmitter->xit<-10000) radio->transmitter->xit=-10000;
    }
    sprintf(text,"%+05ld",radio->transmitter->xit); 
    gtk_label_set_text(GTK_LABEL(v->xit_value),text);
  }
  return TRUE;
}

void xit_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  radio->transmitter->xit_step=choice->selection;
  g_free(choice);
}

static gboolean xit_b_press_cb(GtkWidget *widget,GdkEvent *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *menu_item;
  CHOICE *choice;
  switch(((GdkEventButton*)event)->button) {
    case 3:  // RIGHT
     menu=gtk_menu_new();

     menu_item=gtk_menu_item_new_with_label("1Hz");
     choice=g_new0(CHOICE,1);
     choice->rx=rx;
     choice->selection=1;
     g_signal_connect(menu_item,"activate",G_CALLBACK(xit_cb),choice);
     gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

     menu_item=gtk_menu_item_new_with_label("5Hz");
     choice=g_new0(CHOICE,1);
     choice->rx=rx;
     choice->selection=5;
     g_signal_connect(menu_item,"activate",G_CALLBACK(xit_cb),choice);
     gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

     menu_item=gtk_menu_item_new_with_label("10Hz");
     choice=g_new0(CHOICE,1);
     choice->rx=rx;
     choice->selection=10;
     g_signal_connect(menu_item,"activate",G_CALLBACK(xit_cb),choice);
     gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

     menu_item=gtk_menu_item_new_with_label("100Hz");
     choice=g_new0(CHOICE,1);
     choice->rx=rx;
     choice->selection=100;
     g_signal_connect(menu_item,"activate",G_CALLBACK(xit_cb),choice);
     gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

     menu_item=gtk_menu_item_new_with_label("1000Hz");
     choice=g_new0(CHOICE,1);
     choice->rx=rx;
     choice->selection=1000;
     g_signal_connect(menu_item,"activate",G_CALLBACK(xit_cb),choice);
     gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

     gtk_widget_show_all(menu);
     gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
     gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
     gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
      return TRUE;
      break;
  }
  return FALSE;
}

static void dup_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->duplex=gtk_toggle_button_get_active(widget);
}

static void ctun_b_cb(GtkToggleButton *widget,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  rx->ctun=!rx->ctun;
  receiver_set_ctun(rx);
}

static gboolean bmk_b_pressed_cb(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
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
  return TRUE;
}

static gboolean agc_b_pressed_cb(GtkWidget *widget,GdkEventButton *event,gpointer user_data);

void agc_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)choice->rx->vfo,"vfo_data");

  choice->rx->agc=choice->selection;
  set_agc(choice->rx);
  switch(choice->selection) {
    case AGC_OFF:
      gtk_button_set_label(GTK_BUTTON(choice->button),"AGC");
      break;
    case AGC_LONG:
      gtk_button_set_label(GTK_BUTTON(choice->button),"AGC LONG");
      break;
    case AGC_SLOW:
      gtk_button_set_label(GTK_BUTTON(choice->button),"AGC SLOW");
      break;
    case AGC_MEDIUM:
      gtk_button_set_label(GTK_BUTTON(choice->button),"AGC MED");
      break;
    case AGC_FAST:
      gtk_button_set_label(GTK_BUTTON(choice->button),"AGC FAST");
      break;
  }
  g_signal_handlers_block_by_func(v->agc_b,G_CALLBACK(agc_b_pressed_cb),data);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->agc_b),choice->rx->agc!=AGC_OFF);
  g_signal_handlers_unblock_by_func(v->agc_b,G_CALLBACK(agc_b_pressed_cb),data);

  update_noise(choice->rx);

  g_free(choice);
}

static gboolean agc_b_pressed_cb(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu;
  GtkWidget *menu_item;
  CHOICE *choice;

  menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label("OFF");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=AGC_OFF;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("LONG");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=AGC_LONG;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("SLOW");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=AGC_SLOW;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("MEDIUM");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=AGC_MEDIUM;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  menu_item=gtk_menu_item_new_with_label("FAST");
  choice=g_new0(CHOICE,1);
  choice->rx=rx;
  choice->selection=AGC_FAST;
  choice->button=widget;
  g_signal_connect(menu_item,"activate",G_CALLBACK(agc_cb),choice);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
  gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
  gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
  return TRUE;
}

static gboolean afgain_press_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  pressed=TRUE;
  last_x=event->x;
  has_moved=FALSE;
  return TRUE;
}

static gboolean afgain_scale_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(pressed) {
    gdouble moved=event->x-last_x;
    has_moved=TRUE;
    rx->volume=rx->volume+(moved/100.0);
    if(rx->volume>1.0) rx->volume=1.0;
    if(rx->volume<0.0) rx->volume=0.0;
    receiver_set_volume(rx);
    last_x=event->x;
    update_vfo(rx);
  }
  return TRUE;
}

static gboolean afgain_release_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(has_moved) {
    gdouble moved=event->x-last_x;
    rx->volume=rx->volume+(moved/100.0);
    if(rx->volume>1.0) rx->volume=1.0;
    if(rx->volume<0.0) rx->volume=0.0;
    receiver_set_volume(rx);
  } else {
    rx->volume=event->x/100.0;
    if(rx->volume>1.0) rx->volume=1.0;
    if(rx->volume<0.0) rx->volume=0.0;
    receiver_set_volume(rx);
  }
  pressed=FALSE;
  has_moved=FALSE;
  update_vfo(rx);
  return TRUE;
}

static gboolean afgain_scale_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  if(event->direction==GDK_SCROLL_DOWN) {
    if(rx->volume>0.0) {
      rx->volume=rx->volume-0.01;
    }
  } else if(event->direction==GDK_SCROLL_UP) {
    if(rx->volume<1.0) {
      rx->volume=rx->volume+0.01;
    }
  }
  gtk_level_bar_set_value(GTK_LEVEL_BAR(v->afgain_scale),rx->volume);
  receiver_set_volume(rx);
  return TRUE;
}

static gboolean agcgain_press_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  pressed=TRUE;
  last_x=event->x;
  has_moved=FALSE;
  return TRUE;
}

static gboolean agcgain_scale_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(pressed) {
    has_moved=TRUE;
    gdouble moved=event->x-last_x;
    rx->agc_gain=rx->agc_gain+moved;
    if(rx->agc_gain>120.0) rx->agc_gain=120.0;
    if(rx->agc_gain<-20.0) rx->agc_gain=-20.0;
    receiver_set_agc_gain(rx);
    last_x=event->x;
    update_vfo(rx);
  }
  return TRUE;
}

static gboolean agcgain_release_cb(GtkWidget *widget,GdkEventButton *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  if(has_moved) {
    gdouble moved=event->x-last_x;
    rx->agc_gain=rx->agc_gain+moved;
    if(rx->agc_gain>120.0) rx->agc_gain=120.0;
    if(rx->agc_gain<-20.0) rx->agc_gain=-20.0;
    receiver_set_agc_gain(rx);
  } else {
    rx->agc_gain=event->x-20.0;
    if(rx->agc_gain>120.0) rx->agc_gain=120.0;
    if(rx->agc_gain<-20.0) rx->agc_gain=-20.0;
    receiver_set_agc_gain(rx);
  }
  pressed=FALSE;
  has_moved=FALSE;
  update_vfo(rx);
  return TRUE;
}


static gboolean agcgain_scale_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  if(event->direction==GDK_SCROLL_DOWN) {
    if(rx->agc_gain>-20.0) {
      rx->agc_gain=rx->agc_gain-1.0;
    }
  } if(event->direction==GDK_SCROLL_UP) {
    if(rx->agc_gain<120.0) {
      rx->agc_gain=rx->agc_gain+1.0;
    }
  }
  gtk_level_bar_set_value(GTK_LEVEL_BAR(v->agcgain_scale),rx->agc_gain+20.0);
  receiver_set_agc_gain(rx);
  return TRUE;
}

void band_cb(GtkWidget *menu_item,gpointer data) {
  CHOICE *choice=(CHOICE *)data;
  set_band(choice->rx,choice->selection,choice->sub_selection);
  update_vfo(choice->rx);
  g_free(choice);
}

static gboolean frequency_a_press_cb(GtkWidget *widget,GdkEvent *event,gpointer user_data) {
  RECEIVER *rx=(RECEIVER *)user_data;
  GtkWidget *menu=gtk_menu_new();
  GtkWidget *band_menu;
  GtkWidget *menu_item;
  CHOICE *choice;
  BAND *band;
  BANDSTACK *bandstack;
  BANDSTACK_ENTRY *entry;
  char temp[64];
  int i,j;

  if(!rx->locked) {
    menu=gtk_menu_new();
    for(i=0;i<BANDS+XVTRS;i++) {
#ifdef SOAPYSDR
      if(radio->discovered->protocol!=PROTOCOL_SOAPYSDR) {
        if(i>=band70 && i<=bandAIR) {
          continue;
        }
      }
#endif
      band=(BAND*)band_get_band(i);
      bandstack=band->bandstack;

      if(strlen(band->title)>0) {
        GtkWidget *band_menu=gtk_menu_new();
        menu_item=gtk_menu_item_new_with_label(band->title);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),band_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);

	for(j=0;j<bandstack->entries;j++) {
          entry=&bandstack->entry[j];
	  sprintf(temp,"%5lld.%03lld.%03lld",entry->frequency/(long long)1000000,(entry->frequency%(long long)1000000)/(long long)1000,entry->frequency%(long long)1000);
          menu_item=gtk_menu_item_new_with_label(temp);
          choice=g_new0(CHOICE,1);
          choice->rx=rx;
          choice->selection=i;
          choice->sub_selection=j;
          g_signal_connect(menu_item,"activate",G_CALLBACK(band_cb),choice);
          gtk_menu_shell_append(GTK_MENU_SHELL(band_menu),menu_item);
	}
      }
    }
    gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
    gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
    gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,NULL,NULL);
#endif
  }
  return TRUE;
}

static gboolean frequency_a_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  int digit;

  if(!rx->locked) {
    digit=event->x/(gtk_widget_get_allocated_width(v->frequency_a_text)/rx->vfo_a_digits);
    long long step=0LL;
    if(digit<13) {
      step=ll_step[digit];
    }
    if(event->direction==GDK_SCROLL_DOWN && rx->ctun) {
      step=-step;
    }                    
    if(event->direction==GDK_SCROLL_UP && !rx->ctun) {
      step=-step;
    }                    
g_print("%s: digit=%d step=%lld\n",__FUNCTION__,digit,step);
    receiver_move(rx,step,FALSE);
  }
  update_vfo(rx);
  return TRUE;
}

static gboolean frequency_a_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  int digit;

  if(!rx->locked) {
    digit=event->x/(gtk_widget_get_allocated_width(v->frequency_a_text)/rx->vfo_a_digits);
    if(digit>=0 && digit<13) {
      long long step=ll_step[digit];
      if(step==0LL) {
        gdk_window_set_cursor(gtk_widget_get_window(v->frequency_a_text),gdk_cursor_new(GDK_ARROW));
      } else {
        gdk_window_set_cursor(gtk_widget_get_window(v->frequency_a_text),gdk_cursor_new(GDK_DOUBLE_ARROW));
      }
    } else {
      gdk_window_set_cursor(gtk_widget_get_window(v->frequency_a_text),gdk_cursor_new(GDK_ARROW));
    }
 }
 return TRUE;
}

static gboolean frequency_b_press_cb(GtkWidget *widget,GdkEvent *event,gpointer user_data) {
  return TRUE;
}

static gboolean frequency_b_scroll_event_cb(GtkWidget *widget,GdkEventScroll *event,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  int digit;

  if(!rx->locked) {
    digit=event->x/(gtk_widget_get_allocated_width(v->frequency_b_text)/rx->vfo_b_digits);
    long long step=0LL;
    if(digit<13) {
      step=ll_step[digit];
    }
    if(event->direction==GDK_SCROLL_DOWN) {
      step=-step;
    }
    switch(rx->split) {
      case SPLIT_OFF:
      case SPLIT_ON:
        receiver_move_b(rx,step,FALSE,FALSE);
        break;
      case SPLIT_SAT:
      case SPLIT_RSAT:
        receiver_move_b(rx,step,FALSE,FALSE);
        break;
    }
  }
  frequency_changed(rx);
  update_vfo(rx);
  return TRUE;
}

static gboolean frequency_b_motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");
  int digit;

  if(!rx->locked) {
    digit=event->x/(gtk_widget_get_allocated_width(v->frequency_b_text)/rx->vfo_b_digits);
    if(digit>=0 && digit<13) {
      long long step=ll_step[digit];
      if(step==0LL) {
        gdk_window_set_cursor(gtk_widget_get_window(v->frequency_b_text),gdk_cursor_new(GDK_ARROW));
      } else {
        gdk_window_set_cursor(gtk_widget_get_window(v->frequency_b_text),gdk_cursor_new(GDK_DOUBLE_ARROW));
      }
    } else {
      gdk_window_set_cursor(gtk_widget_get_window(v->frequency_b_text),gdk_cursor_new(GDK_ARROW));
    }
 }
 return TRUE;
}

GtkWidget *create_vfo(RECEIVER *rx) {
  char temp[32];

  g_print("%s: rx=%d\n",__FUNCTION__,rx->channel);

  int x=0;
  int y=0;

  VFO_DATA *v=g_new(VFO_DATA,1);

  v->vfo=gtk_layout_new(NULL,NULL);


  gtk_widget_set_name(v->vfo,"vfo");

  v->vfo_a_text=gtk_label_new("VFO A");
  gtk_widget_set_name(v->vfo_a_text,"vfo-a-text");
  gtk_widget_set_size_request(v->vfo_a_text,40,15);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->vfo_a_text,x,y);
  x+=50;

  v->a2b=gtk_button_new_with_label("A>B");
  gtk_widget_set_name(v->a2b,"vfo-button");
  gtk_widget_set_size_request(v->a2b,40,15);
  g_signal_connect(v->a2b, "pressed", G_CALLBACK(a2b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->a2b,x,y);
  x+=45;

  v->b2a=gtk_button_new_with_label("A<B");
  gtk_widget_set_name(v->b2a,"vfo-button");
  gtk_widget_set_size_request(v->b2a,40,15);
  g_signal_connect(v->b2a, "pressed", G_CALLBACK(b2a_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->b2a,x,y);
  x+=45;

  v->aswapb=gtk_button_new_with_label("A<>B");
  gtk_widget_set_name(v->aswapb,"vfo-button");
  gtk_widget_set_size_request(v->aswapb,40,15);
  g_signal_connect(v->aswapb, "pressed", G_CALLBACK(aswapb_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->aswapb,x,y);
  x+=45;

  switch(rx->split) {
    case SPLIT_OFF:
    case SPLIT_ON:
      strcpy(temp, "SPLIT");
      break;
    case SPLIT_SAT:
      strcpy(temp, "SAT");
      break;
    case SPLIT_RSAT:
      strcpy(temp, "RSAT");
      break;
  }
  v->split_b=gtk_toggle_button_new_with_label(temp);
  gtk_widget_set_name(v->split_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->split_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->split_b),rx->split!=SPLIT_OFF);
  gtk_widget_set_size_request(v->split_b,40,15);
  g_signal_connect(v->split_b, "toggled", G_CALLBACK(split_b_cb),rx);
  g_signal_connect(v->split_b, "button_press_event", G_CALLBACK(split_b_press_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->split_b,x,y);

  x=240;

  v->vfo_b_text=gtk_label_new("VFO B");
  gtk_widget_set_name(v->vfo_b_text,"vfo-b-text");
  gtk_widget_set_size_request(v->vfo_b_text,40,15);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->vfo_b_text,x,y);
  x+=50;

  sprintf(temp,"ZOOM x%d",rx->zoom);
  v->zoom_b=gtk_button_new_with_label(temp);
  gtk_widget_set_name(v->zoom_b,"vfo-button");
  gtk_widget_set_size_request(v->zoom_b,60,15);
  g_signal_connect(v->zoom_b, "pressed",G_CALLBACK(zoom_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->zoom_b,x,y);
  x+=70;

  sprintf(temp,"STEP %s",step_labels[get_step(rx->step)]);
  v->step_b=gtk_button_new_with_label(temp);
  gtk_widget_set_name(v->step_b,"vfo-button");
  gtk_widget_set_size_request(v->step_b,70,15);
  g_signal_connect(v->step_b, "pressed",G_CALLBACK(step_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->step_b,x,y);
  x+=100;

  v->tx_label=gtk_label_new("");
  gtk_widget_set_name(v->tx_label,"warning-label");
  gtk_widget_set_size_request(v->tx_label,60,15);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->tx_label,x,y);
  if(radio!=NULL && radio->transmitter!=NULL) {
    if(radio->transmitter->rx==rx) {
      gtk_label_set_text(GTK_LABEL(v->tx_label),"ASSIGNED TX");
    }
  }
  
  /* ... */

  x=0;
  y=16;

  long long af=rx->frequency_a;
  if(rx->ctun) af=rx->ctun_frequency;
  if(rx->entering_frequency) af=rx->entered_frequency;
    
  sprintf(temp,"%5lld.%03lld.%03lld",af/(long long)1000000,(af%(long long)1000000)/(long long)1000,af%(long long)1000);
  v->frequency_a_text=gtk_label_new(temp);
  rx->vfo_a_digits=strlen(temp);
  gtk_widget_set_name(v->frequency_a_text,"frequency-a-text");
  gtk_label_set_width_chars(GTK_LABEL(v->frequency_a_text),rx->vfo_a_digits);

  GtkWidget *event_box_a=gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box_a),v->frequency_a_text);
  gtk_layout_put(GTK_LAYOUT(v->vfo),event_box_a,x,y);
  gtk_widget_set_events(event_box_a, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
  g_signal_connect(event_box_a,"button_press_event",G_CALLBACK(frequency_a_press_cb),rx);
  g_signal_connect(event_box_a,"motion-notify-event",G_CALLBACK(frequency_a_motion_notify_event_cb),rx);
  g_signal_connect(event_box_a,"scroll_event",G_CALLBACK(frequency_a_scroll_event_cb),(gpointer)rx);

  x+=240;
  y=20;

  long long bf=rx->frequency_b;
  sprintf(temp,"%5lld.%03lld.%03lld",bf/(long long)1000000,(bf%(long long)1000000)/(long long)1000,bf%(long long)1000);
  v->frequency_b_text=gtk_label_new(temp);
  rx->vfo_b_digits=strlen(temp);
  gtk_widget_set_name(v->frequency_b_text,"frequency-b-text");
  gtk_label_set_width_chars(GTK_LABEL(v->frequency_b_text),rx->vfo_b_digits);

  GtkWidget *event_box_b=gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box_b),v->frequency_b_text);
  gtk_layout_put(GTK_LAYOUT(v->vfo),event_box_b,x,y);
  g_signal_connect(event_box_b,"motion-notify-event",G_CALLBACK(frequency_b_motion_notify_event_cb),rx);
  g_signal_connect(event_box_b,"scroll_event",G_CALLBACK(frequency_b_scroll_event_cb),(gpointer)rx);
  gtk_widget_set_events(event_box_b, GDK_SCROLL_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

  x+=180;
  y=23;

  v->subrx_b=gtk_toggle_button_new_with_label("SUBRX");
  gtk_widget_set_name(v->subrx_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->subrx_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->subrx_b),FALSE);
  gtk_widget_set_size_request(v->subrx_b,35,6);
  g_signal_connect(v->subrx_b, "toggled", G_CALLBACK(subrx_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->subrx_b,x,y);

  x=x+60;
  y=17;

  GtkWidget *afgain_label=gtk_label_new("AF Gain:");
  gtk_widget_set_name(afgain_label,"afgain-text");
  gtk_layout_put(GTK_LAYOUT(v->vfo),afgain_label,x,y);
  x+=60;
  y=18;


  v->afgain_scale=gtk_level_bar_new();
  gtk_level_bar_remove_offset_value(GTK_LEVEL_BAR(v->afgain_scale),GTK_LEVEL_BAR_OFFSET_LOW);
  gtk_level_bar_remove_offset_value(GTK_LEVEL_BAR(v->afgain_scale),GTK_LEVEL_BAR_OFFSET_HIGH);
  gtk_level_bar_remove_offset_value(GTK_LEVEL_BAR(v->afgain_scale),GTK_LEVEL_BAR_OFFSET_FULL);
  gtk_widget_set_name(v->afgain_scale,"afgain-scale");
  gtk_widget_set_size_request(v->afgain_scale,100,15);
  gtk_level_bar_set_value(GTK_LEVEL_BAR(v->afgain_scale),rx->volume);

  GtkWidget *event_box_afgain=gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box_afgain),v->afgain_scale);
  gtk_layout_put(GTK_LAYOUT(v->vfo),event_box_afgain,x,y);
  g_signal_connect(event_box_afgain,"motion-notify-event",G_CALLBACK(afgain_scale_motion_notify_event_cb),rx);
  g_signal_connect(event_box_afgain,"scroll_event",G_CALLBACK(afgain_scale_scroll_event_cb),(gpointer)rx);
  g_signal_connect(event_box_afgain,"button_press_event",G_CALLBACK(afgain_press_cb),rx);
  g_signal_connect(event_box_afgain,"button_release_event",G_CALLBACK(afgain_release_cb),rx);
  gtk_widget_set_events(event_box_afgain, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

  x=480;
  y=31;

  GtkWidget *agcgain_label=gtk_label_new("AGC Gain:");
  gtk_widget_set_name(agcgain_label,"agcgain-text");
  gtk_layout_put(GTK_LAYOUT(v->vfo),agcgain_label,x,y);

  x+=60;
  y=32;

  v->agcgain_scale=gtk_level_bar_new_for_interval(0.0,140.0);
  gtk_level_bar_remove_offset_value(GTK_LEVEL_BAR(v->agcgain_scale),GTK_LEVEL_BAR_OFFSET_LOW);
  gtk_level_bar_remove_offset_value(GTK_LEVEL_BAR(v->agcgain_scale),GTK_LEVEL_BAR_OFFSET_HIGH);
  gtk_level_bar_remove_offset_value(GTK_LEVEL_BAR(v->agcgain_scale),GTK_LEVEL_BAR_OFFSET_FULL);
  gtk_widget_set_name(v->agcgain_scale,"agcgain-scale");
  gtk_widget_set_size_request(v->agcgain_scale,140,15);
  gtk_level_bar_set_value(GTK_LEVEL_BAR(v->agcgain_scale),rx->agc_gain+20.0);

  GtkWidget *event_box_agcgain=gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box_agcgain),v->agcgain_scale);
  gtk_layout_put(GTK_LAYOUT(v->vfo),event_box_agcgain,x,y);
  g_signal_connect(event_box_agcgain,"motion-notify-event",G_CALLBACK(agcgain_scale_motion_notify_event_cb),rx);
  g_signal_connect(event_box_agcgain,"scroll_event",G_CALLBACK(agcgain_scale_scroll_event_cb),(gpointer)rx);
  g_signal_connect(event_box_agcgain,"button_press_event",G_CALLBACK(agcgain_press_cb),rx);
  g_signal_connect(event_box_agcgain,"button_release_event",G_CALLBACK(agcgain_release_cb),rx);
  gtk_widget_set_events(event_box_agcgain, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);


  y=47;
  x=0;

  v->lock_b=gtk_toggle_button_new_with_label("Lock");
  gtk_widget_set_name(v->lock_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->lock_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->lock_b),FALSE);
  gtk_widget_set_size_request(v->lock_b,35,6);
  g_signal_connect(v->lock_b, "toggled", G_CALLBACK(lock_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->lock_b,x,y);
  x=x+40;

  v->mode_b=gtk_button_new_with_label(mode_string[rx->mode_a]);
  gtk_widget_set_name(v->mode_b,"vfo-mode-filter-button");
  gtk_widget_set_size_request(v->mode_b,35,6);
  g_signal_connect(v->mode_b, "pressed", G_CALLBACK(mode_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->mode_b,x,y);
  x=x+40;

  FILTER *band_filters=filters[rx->mode_a];
  
  if(rx->mode_a==FMN) {
    if(rx->deviation==2500) {
      strcpy(temp,"8000");
    } else {
      strcpy(temp,"16000");
    }
  } else {
    strcpy(temp,band_filters[rx->filter_a].title);
  }

  v->filter_b=gtk_button_new_with_label(temp);
  gtk_widget_set_name(v->filter_b,"vfo-mode-filter-button");
  gtk_widget_set_size_request(v->filter_b,35,6);
  g_signal_connect(v->filter_b, "pressed", G_CALLBACK(filter_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->filter_b,x,y);
  x=x+40;

  strcpy(temp,"NB");
  if(rx->nb2) {
    strcpy(temp,"NB2");
  }
  v->nb_b=gtk_toggle_button_new_with_label(temp);
  gtk_widget_set_name(v->nb_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->nb_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->nb_b),rx->nb|rx->nb2);
  gtk_widget_set_size_request(v->nb_b,35,6);
  g_signal_connect(v->nb_b, "button-press-event", G_CALLBACK(nb_b_pressed_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->nb_b,x,y);
  x=x+40;

  v->nr_b=gtk_toggle_button_new_with_label("NR");
  gtk_widget_set_name(v->nr_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->nr_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->nr_b),rx->nr|rx->nr2);
  gtk_widget_set_size_request(v->nr_b,35,6);
  g_signal_connect(v->nr_b, "button-press-event", G_CALLBACK(nr_b_pressed_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->nr_b,x,y);
  x=x+40;

  v->snb_b=gtk_toggle_button_new_with_label("SNB");
  gtk_widget_set_name(v->snb_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->snb_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->snb_b),rx->snb);
  gtk_widget_set_size_request(v->snb_b,35,6);
  g_signal_connect(v->snb_b, "toggled", G_CALLBACK(snb_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->snb_b,x,y);
  x=x+40;

  v->anf_b=gtk_toggle_button_new_with_label("ANF");
  gtk_widget_set_name(v->anf_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->anf_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->anf_b),rx->anf);
  gtk_widget_set_size_request(v->anf_b,35,6);
  g_signal_connect(v->anf_b, "toggled", G_CALLBACK(anf_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->anf_b,x,y);
  x=x+40;

  switch(rx->agc) {
    case AGC_OFF:
      strcpy(temp,"AGC");
      break;
    case AGC_LONG:
      strcpy(temp,"AGC LONG");
      break;
    case AGC_SLOW:
      strcpy(temp,"AGC SLOW");
      break;
    case AGC_MEDIUM:
      strcpy(temp,"AGC MED");
      break;
    case AGC_FAST:
      strcpy(temp,"AGC FAST");
      break;
  }
  v->agc_b=gtk_toggle_button_new_with_label(temp);
  gtk_widget_set_name(v->agc_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->agc_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->agc_b),rx->agc!=AGC_OFF);
  gtk_widget_set_size_request(v->agc_b,75,6);
  g_signal_connect(v->agc_b, "button-press-event", G_CALLBACK(agc_b_pressed_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->agc_b,x,y);
  x=x+80;

  v->rit_b=gtk_toggle_button_new_with_label("RIT");
  gtk_widget_set_name(v->rit_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->rit_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->rit_b),rx->rit_enabled);
  gtk_widget_set_size_request(v->rit_b,35,6);
  g_signal_connect(v->rit_b, "toggled", G_CALLBACK(rit_b_cb),rx);
  g_signal_connect(v->rit_b, "button-press-event", G_CALLBACK(rit_b_press_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->rit_b,x,y);
  x=x+40;

  sprintf(temp,"%+05ld",rx->rit); 
  v->rit_value=gtk_label_new(temp);
  gtk_widget_set_name(v->rit_value,"rit-value");

  GtkWidget *event_box_rit_b=gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box_rit_b),v->rit_value);
  gtk_layout_put(GTK_LAYOUT(v->vfo),event_box_rit_b,x,y);
  g_signal_connect(event_box_rit_b,"scroll_event",G_CALLBACK(rit_b_scroll_event_cb),(gpointer)rx);
  gtk_widget_set_events(event_box_rit_b, GDK_SCROLL_MASK);

  x=x+50;

  v->xit_b=gtk_toggle_button_new_with_label("XIT");
  gtk_widget_set_name(v->xit_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->xit_b),FALSE);
  if(radio->transmitter!=NULL && radio->transmitter->rx==rx) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->xit_b),radio->transmitter->xit_enabled);
  }
  gtk_widget_set_size_request(v->xit_b,35,6);
  g_signal_connect(v->xit_b, "toggled", G_CALLBACK(xit_b_cb),rx);
  g_signal_connect(v->xit_b, "button-press-event", G_CALLBACK(xit_b_press_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->xit_b,x,y);
  x=x+40;

  if(radio->transmitter!=NULL && radio->transmitter->rx==rx) {
    sprintf(temp,"%+05ld",radio->transmitter->xit); 
  } else {
    sprintf(temp,"%+05ld",0L); 
  }
  v->xit_value=gtk_label_new(temp);
  gtk_widget_set_name(v->xit_value,"xit-value");

  GtkWidget *event_box_xit_b=gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box_xit_b),v->xit_value);
  gtk_layout_put(GTK_LAYOUT(v->vfo),event_box_xit_b,x,y);
  g_signal_connect(event_box_xit_b,"scroll_event",G_CALLBACK(xit_b_scroll_event_cb),(gpointer)rx);
  gtk_widget_set_events(event_box_xit_b, GDK_SCROLL_MASK);

  x=x+50;

  v->ctun_b=gtk_toggle_button_new_with_label("CTUN");
  gtk_widget_set_name(v->ctun_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->ctun_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->ctun_b),rx->ctun);
  gtk_widget_set_size_request(v->ctun_b,35,6);
  g_signal_connect(v->ctun_b, "toggled", G_CALLBACK(ctun_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->ctun_b,x,y);
  x=x+40;

  v->dup_b=gtk_toggle_button_new_with_label("DUP");
  gtk_widget_set_name(v->dup_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->dup_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->dup_b),rx->duplex);
  gtk_widget_set_size_request(v->dup_b,35,6);
  g_signal_connect(v->dup_b, "toggled", G_CALLBACK(dup_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->dup_b,x,y);
  x=x+40;

  v->bpsk_b=gtk_toggle_button_new_with_label("BPSK");
  gtk_widget_set_name(v->bpsk_b,"vfo-toggle");
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(v->bpsk_b),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->bpsk_b),rx->bpsk_enable);
  gtk_widget_set_size_request(v->bpsk_b,35,6);
  g_signal_connect(v->bpsk_b, "toggled", G_CALLBACK(bpsk_b_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->bpsk_b,x,y);
  x=x+40;

  v->bmk_b=gtk_button_new_with_label("BMK");
  gtk_widget_set_name(v->bmk_b,"vfo-button");
  gtk_widget_set_size_request(v->bmk_b,35,6);
  g_signal_connect(v->bmk_b, "button-press-event", G_CALLBACK(bmk_b_pressed_cb),rx);
  gtk_layout_put(GTK_LAYOUT(v->vfo),v->bmk_b,x,y);
  x=x+40;

  gtk_widget_show_all(v->vfo);

  g_object_set_data ((GObject *)v->vfo,"vfo_data",v);
  return v->vfo;
}

void update_vfo(RECEIVER *rx) {
  char temp[32];
  char *markup;

  if(rx->vfo==NULL) return;

  VFO_DATA *v=(VFO_DATA *)g_object_get_data((GObject *)rx->vfo,"vfo_data");

  if(v==NULL) return;

  // VFO A
  long long af=rx->frequency_a;
  if(rx->ctun) af=rx->ctun_frequency;
  if(rx->entering_frequency) af=rx->entered_frequency;

  sprintf(temp,"%5lld.%03lld.%03lld",af/(long long)1000000,(af%(long long)1000000)/(long long)1000,af%(long long)1000);
  if(radio!=NULL && radio->transmitter!=NULL && rx==radio->transmitter->rx && radio->transmitter->rx->split==SPLIT_OFF && isTransmitting(radio)) {
    markup=g_markup_printf_escaped("<span foreground=\"#D94545\">%s</span>",temp);
  } else {
    markup=g_markup_printf_escaped("<span foreground=\"#A3CCD1\">%s</span>",temp);
  }
  gtk_label_set_markup(GTK_LABEL(v->frequency_a_text),markup);

  // VFO B
  if(radio!=NULL && radio->transmitter!=NULL && rx==radio->transmitter->rx && radio->transmitter->rx->split!=SPLIT_OFF && isTransmitting(radio)) {
    markup=g_markup_printf_escaped("<span foreground=\"#D94545\">%s</span>","VFO B");
  } else {
    markup=g_markup_printf_escaped("<span foreground=\"#ED9D80\">%s</span>","VFO B");
  }
  gtk_label_set_markup(GTK_LABEL(v->vfo_b_text),markup);

  long long bf=rx->frequency_b;
  sprintf(temp,"%5lld.%03lld.%03lld",bf/(long long)1000000,(bf%(long long)1000000)/(long long)1000,bf%(long long)1000);
  if(radio!=NULL && radio->transmitter!=NULL && rx==radio->transmitter->rx && radio->transmitter->rx->split!=SPLIT_OFF && isTransmitting(radio)) {
    markup=g_markup_printf_escaped("<span foreground=\"#D94545\">%s</span>",temp);
  } else {
    markup=g_markup_printf_escaped("<span foreground=\"#ED9D80\">%s</span>",temp);
  }
  gtk_label_set_markup(GTK_LABEL(v->frequency_b_text),markup);

  // ASSIGNED TX
  if(radio!=NULL && radio->transmitter!=NULL) {
    TRANSMITTER *tx=radio->transmitter;

    if(tx->rx==rx) {
      gtk_label_set_text(GTK_LABEL(v->tx_label),"ASSIGNED TX");
    } else {
      gtk_label_set_text(GTK_LABEL(v->tx_label),"");
    }

    g_signal_handlers_block_by_func(v->xit_b,G_CALLBACK(xit_b_cb),rx);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->xit_b),tx->xit_enabled);
    g_signal_handlers_unblock_by_func(v->xit_b,G_CALLBACK(xit_b_cb),rx);
  }

  // update AF Gain scale
  gtk_level_bar_set_value(GTK_LEVEL_BAR(v->afgain_scale),rx->volume);

  // update AGC Gain scale
  gtk_level_bar_set_value(GTK_LEVEL_BAR(v->agcgain_scale),rx->agc_gain+20.0);

  // update Lock button
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->lock_b),rx->locked);

  // update mode button
  gtk_button_set_label(GTK_BUTTON(v->mode_b),mode_string[rx->mode_a]);

  // update filter button
  FILTER *band_filters=filters[rx->mode_a];
  if(rx->mode_a==FMN) {
    if(rx->deviation==2500) {
      strcpy(temp,"8000");
    } else {
      strcpy(temp,"16000");
    }
  } else {
    strcpy(temp,band_filters[rx->filter_a].title);
  }
  gtk_button_set_label(GTK_BUTTON(v->filter_b),temp);

  // update NB button
  if(rx->nb) {
      gtk_button_set_label(GTK_BUTTON(v->nb_b),"NB");
  } else if(rx->nb2) {
      gtk_button_set_label(GTK_BUTTON(v->nb_b),"NB2");
  } else {
      gtk_button_set_label(GTK_BUTTON(v->nb_b),"NB");
  }
  g_signal_handlers_block_by_func(v->nb_b,G_CALLBACK(nb_b_pressed_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->nb_b),rx->nb|rx->nb2);
  g_signal_handlers_unblock_by_func(v->nb_b,G_CALLBACK(nb_b_pressed_cb),rx);

  // update NR button
  if(rx->nr) {
      gtk_button_set_label(GTK_BUTTON(v->nr_b),"NR");
  } else if(rx->nr2) {
      gtk_button_set_label(GTK_BUTTON(v->nr_b),"NR2");
  } else {
      gtk_button_set_label(GTK_BUTTON(v->nr_b),"NR");
  }
  g_signal_handlers_block_by_func(v->nr_b,G_CALLBACK(nr_b_pressed_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->nr_b),rx->nr|rx->nr2);
  g_signal_handlers_unblock_by_func(v->nr_b,G_CALLBACK(nr_b_pressed_cb),rx);

  // update SNB button
  g_signal_handlers_block_by_func(v->snb_b,G_CALLBACK(snb_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->snb_b),rx->snb);
  g_signal_handlers_unblock_by_func(v->snb_b,G_CALLBACK(snb_b_cb),rx);
 
  // update ANF button
  g_signal_handlers_block_by_func(v->anf_b,G_CALLBACK(anf_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->anf_b),rx->anf);
  g_signal_handlers_unblock_by_func(v->anf_b,G_CALLBACK(anf_b_cb),rx);
 
  // update AGC button
  switch(rx->agc) {
    case AGC_OFF:
      strcpy(temp,"AGC");
      break;
    case AGC_LONG:
      strcpy(temp,"AGC LONG");
      break;
    case AGC_SLOW:
      strcpy(temp,"AGC SLOW");
      break;
    case AGC_MEDIUM:
      strcpy(temp,"AGC MED");
      break;
    case AGC_FAST:
      strcpy(temp,"AGC FAST");
      break;
  }
  gtk_button_set_label(GTK_BUTTON(v->agc_b),temp);
  g_signal_handlers_block_by_func(v->agc_b,G_CALLBACK(agc_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->agc_b),rx->agc!=AGC_OFF);
  g_signal_handlers_unblock_by_func(v->agc_b,G_CALLBACK(agc_cb),rx);

  // update RIT button
  g_signal_handlers_block_by_func(v->rit_b,G_CALLBACK(rit_b_press_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->rit_b),rx->rit_enabled);
  g_signal_handlers_unblock_by_func(v->rit_b,G_CALLBACK(rit_b_press_cb),rx);

  // update XIT button
  if(radio->transmitter!=NULL && radio->transmitter->rx==rx) {
    g_signal_handlers_block_by_func(v->xit_b,G_CALLBACK(xit_b_press_cb),rx);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->xit_b),radio->transmitter->xit_enabled);
    g_signal_handlers_unblock_by_func(v->xit_b,G_CALLBACK(xit_b_press_cb),rx);
  }

  // update CTUN button
  g_signal_handlers_block_by_func(v->ctun_b,G_CALLBACK(ctun_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->ctun_b),rx->ctun);
  g_signal_handlers_unblock_by_func(v->ctun_b,G_CALLBACK(ctun_b_cb),rx);

  // update DUP button
  g_signal_handlers_block_by_func(v->dup_b,G_CALLBACK(dup_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->dup_b),rx->duplex);
  g_signal_handlers_unblock_by_func(v->dup_b,G_CALLBACK(dup_b_cb),rx);

  // update BPSK button
  g_signal_handlers_block_by_func(v->bpsk_b,G_CALLBACK(bpsk_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->bpsk_b),rx->bpsk_enable);
  g_signal_handlers_unblock_by_func(v->bpsk_b,G_CALLBACK(bpsk_b_cb),rx);
 
  // update ZOOM button
  sprintf(temp,"ZOOM x%d",rx->zoom);
  gtk_button_set_label(GTK_BUTTON(v->zoom_b),temp);

  // update STEP button
  sprintf(temp,"STEP %s",step_labels[get_step(rx->step)]);
  gtk_button_set_label(GTK_BUTTON(v->step_b),temp);

  // update SPLIT button
  switch(rx->split) {
     case SPLIT_OFF:
     case SPLIT_ON:
       gtk_button_set_label(GTK_BUTTON(v->split_b),"SPLIT");
       break;
     case SPLIT_SAT:
       gtk_button_set_label(GTK_BUTTON(v->split_b),"SAT");
       break;
     case SPLIT_RSAT:
       gtk_button_set_label(GTK_BUTTON(v->split_b),"RSAT");
       break;
  }
  g_signal_handlers_block_by_func(v->split_b,G_CALLBACK(split_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->split_b),rx->split!=SPLIT_OFF);
  g_signal_handlers_unblock_by_func(v->split_b,G_CALLBACK(split_b_cb),rx);

  // SUBRX button
  g_signal_handlers_block_by_func(v->subrx_b,G_CALLBACK(subrx_b_cb),rx);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(v->subrx_b),rx->subrx!=NULL);
  g_signal_handlers_unblock_by_func(v->subrx_b,G_CALLBACK(subrx_b_cb),rx);

}


// now only used by meter.c

void SetColour(cairo_t *cr, const int colour) {

  switch(colour) {
    case BACKGROUND: {
      cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
      break;
    }
    case OFF_WHITE: {
      cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
      break;
    }
    case BOX_ON: {
      cairo_set_source_rgb (cr, 0.624, 0.427, 0.690);
      break;
    }
    case BOX_OFF: {
      cairo_set_source_rgb (cr, 0.2, 0.2,       0.2);
      break;
    }
    case TEXT_A: {
      cairo_set_source_rgb(cr, 0.929, 0.616, 0.502);
      break;
    }
    case TEXT_B: {
      //light blue
      cairo_set_source_rgb(cr, 0.639, 0.800, 0.820);
      break;
    }
    case TEXT_C: {
      // Pale orange
      cairo_set_source_rgb(cr, 0.929,   0.616,  0.502);
      break;
    }
    case WARNING: {
      // Pale red
        cairo_set_source_rgb (cr, 0.851, 0.271, 0.271);
      break;
    }
    case DARK_LINES: {
      // Dark grey
        cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
      break;
    }
    case DARK_TEXT: {
      cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
      break;
    }
    case INFO_ON: {
      cairo_set_source_rgb(cr, 0.15, 0.58, 0.6);
      break;
    }
    case INFO_OFF: {
      cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
      break;
    }
  }
}
