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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wdsp.h>

#include "discovery.h"
#include "discovered.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "radio.h"
#include "main.h"
#include "protocol1.h"
#include "protocol2.h"
#include "property.h"
#include "rigctl.h"
#include "version.h"

GtkWidget *main_window;
static GtkWidget *grid;

static sem_t *wisdom_sem;
static GThread *wisdom_thread_id;

static GtkListStore *store;
static GtkWidget *list;
static GtkTreeModel *model;
static GtkWidget *view;
static gulong selection_signal_id;
static GtkWidget *none_found;
static GtkWidget *start;
static GtkWidget *retry;

static DISCOVERED *d=NULL;

RADIO *radio;

enum {
  NAME_COLUMN,
  VERSION_COLUMN,
  PROTOCOL_COLUMN,
  IP_COLUMN,
  MAC_COLUMN,
  INTERFACE_COLUMN,
  STATUS_COLUMN,
  N_COLUMNS
};

static gboolean main_delete (GtkWidget *widget) {
  if(radio!=NULL) {
    radio_save_state(radio);
    switch(radio->discovered->protocol) {
      case PROTOCOL_1:
        protocol1_stop();
        break;
      case PROTOCOL_2:
        protocol2_stop();
        break;
    }
  }
  _exit(0);
}

static gpointer wisdom_thread(gpointer arg) {
g_print("Creating wisdom file: %s\n", (char *)arg);
  WDSPwisdom ((char *)arg);
  sem_post(wisdom_sem);
  return NULL;
}

static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data) {
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *ip;
  gint i;

g_print("tree_selection_changed_cb\n");
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, IP_COLUMN, &ip, -1);
    for(i=0;i<devices;i++) {
      if(g_strcmp0(ip,inet_ntoa(discovered[i].info.network.address.sin_addr))==0) {
        break;
      }
    }
    if(i<devices) {
      g_print("found %d\n",i);
      d=&discovered[i];
      switch(d->status) {
        case STATE_AVAILABLE:
          gtk_widget_set_sensitive(start, TRUE);
          break;
        case STATE_SENDING:
          gtk_widget_set_sensitive(start, FALSE);
          break;
      }
    } else {
      d=NULL;
      g_print("could not find %s\n",ip);
    }
    g_free (ip);
  }
}

static int discover(void *data) {
  char v[32];
  char mac[32];
  gint i;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;
  GtkTreeIter iter0;

  discovery();
  g_print("main: discovery found %d devices\n",devices);

  if(devices>0) {
    view=gtk_tree_view_new();

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Device", renderer, "text", NAME_COLUMN, NULL);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Protocol", renderer, "text", PROTOCOL_COLUMN, NULL);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Version", renderer, "text", VERSION_COLUMN, NULL);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "IP", renderer, "text", IP_COLUMN, NULL);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "MAC", renderer, "text", MAC_COLUMN, NULL);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "IFACE", renderer, "text", INTERFACE_COLUMN, NULL);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Status", renderer, "text", STATUS_COLUMN, NULL);


    store=gtk_list_store_new(N_COLUMNS,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);


    for(i=0;i<devices;i++) {
      d=&discovered[i];
      sprintf(v,"%d.%d", d->software_version/10, d->software_version%10);
      sprintf(mac,"%02X:%02X:%02X:%02X:%02X:%02X",
        d->info.network.mac_address[0],
        d->info.network.mac_address[1],
        d->info.network.mac_address[2],
        d->info.network.mac_address[3],
        d->info.network.mac_address[4],
        d->info.network.mac_address[5]);

g_print("adding %s\n",d->name);
      gtk_list_store_append(store,i==0?&iter0:&iter);
      gtk_list_store_set(store,i==0?&iter0:&iter,
        NAME_COLUMN, d->name,
        PROTOCOL_COLUMN, d->protocol==PROTOCOL_1?"1":"2",
        VERSION_COLUMN, v,
          IP_COLUMN, inet_ntoa(d->info.network.address.sin_addr),
        MAC_COLUMN, mac,
        INTERFACE_COLUMN, d->info.network.interface_name,
        STATUS_COLUMN, d->status==2?"Idle":"In Use",
        -1);
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));

    gtk_grid_attach(GTK_GRID(grid), view, 1, 0, 4, 1); 
    GtkTreeSelection *selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    selection_signal_id=g_signal_connect(G_OBJECT(selection),"changed",G_CALLBACK(tree_selection_changed_cb),NULL);
    gtk_tree_selection_unselect_all(selection);
    gtk_tree_selection_select_iter(selection,&iter0);

  } else {
    gtk_widget_set_sensitive(start, FALSE);
    none_found=gtk_label_new("No HPSDR devices found");
    gtk_grid_attach(GTK_GRID(grid), none_found, 1, 0, 4, 1); 
  }

  //gtk_widget_show_all(grid);
  gtk_widget_show_all(main_window);

  gdk_window_set_cursor(gtk_widget_get_window(main_window),gdk_cursor_new(GDK_ARROW));

  return 0;
}

static gboolean wisdom_delete(GtkWidget *widget) {
  _exit(0);
}

static int check_wisdom(void *data) {
  char *res;
  char wisdom_directory[1024];
  char wisdom_file[1024];
  GtkWidget *dialog;
  char label[128];

  sprintf(wisdom_directory,"%s/.local/share/linhpsdr/",g_get_home_dir());
  sprintf(wisdom_file,"%swdspWisdom",wisdom_directory);
  if(access(wisdom_file,F_OK)<0) {
#ifdef __APPLE__
      wisdom_sem=sem_open("wisdomsem",O_CREAT,0700,0);
#else
      wisdom_sem=malloc(sizeof(sem_t));
      int rc=sem_init(wisdom_sem, 0, 0);
#endif
      wisdom_thread_id = g_thread_new( "Wisdoom", wisdom_thread, (gpointer)wisdom_directory);
      if( ! wisdom_thread_id ) {
        g_print("g_thread_new failed for wisdom_thread\n");
        exit( -1 );
      }

      dialog=gtk_dialog_new();
      g_signal_connect (dialog, "delete-event", G_CALLBACK (wisdom_delete), NULL);
      gtk_window_set_title(GTK_WINDOW(dialog),"Linux HPSDR: Creating FFTW3 wisdom file");
      GtkWidget *content=gtk_dialog_get_content_area(GTK_DIALOG(dialog));
      GtkWidget *grid=gtk_grid_new();
      gtk_grid_set_row_spacing(GTK_GRID(grid),10);
      GtkWidget *info=gtk_label_new("               Optimizing FFT sizes through 262145:               ");
      gtk_grid_attach(GTK_GRID(grid),info,0,0,1,1);
      GtkWidget *text=gtk_label_new("                         ");
      gtk_grid_attach(GTK_GRID(grid),text,0,1,1,1);
      GtkWidget *patient=gtk_label_new("(Please be patient. This will take several minutes.)");
      gtk_grid_attach(GTK_GRID(grid),patient,0,2,1,1);
      gtk_container_add(GTK_CONTAINER(content),grid);
      gtk_widget_show_all(dialog);
      while(sem_trywait(wisdom_sem)<0) {
        sprintf(label,"          %s          ",wisdom_get_status());
        gtk_label_set_label(GTK_LABEL(text),label);
        while (gtk_events_pending ())
          gtk_main_iteration ();
        usleep(100000); // 100ms
      }
      gtk_widget_destroy(dialog);
  }
  g_idle_add(discover,NULL);
  return 0;
}

gboolean retry_cb(GtkWidget *widget,gpointer data) {
  gdk_window_set_cursor(gtk_widget_get_window(main_window),gdk_cursor_new(GDK_WATCH));
  if(view!=NULL) {
    GtkTreeSelection *selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    g_signal_handler_disconnect(selection,selection_signal_id);
    gtk_container_remove(GTK_CONTAINER(grid),view);
    view=NULL;
  }
  if(none_found!=NULL) {
    gtk_container_remove(GTK_CONTAINER(grid),none_found);
    none_found=NULL;
  }
  g_idle_add(discover,NULL);
  return TRUE;
}

gboolean start_cb(GtkWidget *widget,gpointer data) { 
  char v[32];
  char mac[32];
  gchar title[128];
  char *value;
  gint x=-1;
  gint y=-1;
  gint width;
  gint height;

  if(d!=NULL && d->status==STATE_AVAILABLE) {
    g_snprintf(v,sizeof(v),"%d.%d", d->software_version/10, d->software_version%10);
    g_snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",
        d->info.network.mac_address[0],
        d->info.network.mac_address[1],
        d->info.network.mac_address[2],
        d->info.network.mac_address[3],
        d->info.network.mac_address[4],
        d->info.network.mac_address[5]);
    g_snprintf((gchar *)&title,sizeof(title),"Linux HPSDR (%s): %s P%d v%s %s (%s) on %s",
      version,
      d->name,
      d->protocol==PROTOCOL_1?1:2,
      v,
      inet_ntoa(d->info.network.address.sin_addr),
      mac,
      d->info.network.interface_name);

    g_print("starting %s\n",title);
    gdk_window_set_cursor(gtk_widget_get_window(main_window),gdk_cursor_new(GDK_WATCH));
    gtk_window_set_title(GTK_WINDOW (main_window),title);
    while(gtk_events_pending()) gtk_main_iteration();

    radio=create_radio(d);
    gtk_container_remove(GTK_CONTAINER(grid),view);
    gtk_container_remove(GTK_CONTAINER(grid),start);
    gtk_container_remove(GTK_CONTAINER(grid),retry);
    gtk_grid_attach(GTK_GRID(grid), radio->visual, 1, 0, 4, 1);
    gtk_widget_show_all(grid);

    //launch_rigctl(radio);

    value=getProperty("radio.x");
    if(value!=NULL) x=atoi(value);
    value=getProperty("radio.y");
    if(value!=NULL) y=atoi(value);
g_print("x=%d y=%d\n",x,y);
    if(x!=-1 && y!=-1) {
g_print("moving main_window to x=%d y=%d\n",x,y);
      gtk_window_move(GTK_WINDOW(main_window),x,y);
/*
      value=getProperty("radio.width");
      if(value!=NULL) width=atoi(value);
      value=getProperty("radio.height");
      if(value!=NULL) height=atoi(value);
      gtk_window_resize(GTK_WINDOW(main_window),width,height);
*/
    }
    gdk_window_set_cursor(gtk_widget_get_window(main_window),gdk_cursor_new(GDK_ARROW));
  }
  return TRUE;
}

static void activate_hpsdr(GtkApplication *app, gpointer data) {
  struct utsname unameData;
  char title[64];
  char png_path[128];

  g_print("Build: %s %s\n",build_date,version);
  g_print("GTK+ version %d.%d.%d\n", gtk_major_version, gtk_minor_version, gtk_micro_version);
  uname(&unameData);
  g_print("sysname: %s\n",unameData.sysname);
  g_print("nodename: %s\n",unameData.nodename);
  g_print("release: %s\n",unameData.release);
  g_print("version: %s\n",unameData.version);
  g_print("machine: %s\n",unameData.machine);
  GdkScreen *screen=gdk_screen_get_default();
  if(screen==NULL) {
    g_print("HPSDR: no default screen!\n");
    _exit(0);
  }

#ifdef __APPLE__
  sprintf(png_path,"/usr/local/share/linhpsdr/hpsdr.png");
#else
  sprintf(png_path,"/usr/share/linhpsdr/hpsdr.png");
#endif
  main_window = gtk_application_window_new (app);
  sprintf(title,"Linux HPSDR (%s)",version);
  gtk_window_set_title (GTK_WINDOW (main_window), title);
  gtk_window_set_resizable(GTK_WINDOW(main_window), FALSE);
  GError *error;
  if(!gtk_window_set_icon_from_file (GTK_WINDOW(main_window), png_path, &error)) {
    g_print("Warning: failed to set icon for main_window: %s\n",png_path);
    if(error!=NULL) {
      g_print("%s\n",error->message);
    }
  }
  g_signal_connect (main_window, "delete-event", G_CALLBACK (main_delete), NULL);

  grid = gtk_grid_new();
  //gtk_widget_set_size_request(grid, 800, 480);
  //gtk_grid_set_row_homogeneous(GTK_GRID(grid),TRUE);
  //gtk_grid_set_column_homogeneous(GTK_GRID(grid),FALSE);

  //GtkWidget *image=gtk_image_new_from_file("~/.local/share/linhpsdr/hpsdr.png");
  GtkWidget *image=gtk_image_new_from_file(png_path);
  gtk_grid_attach(GTK_GRID(grid), image, 0, 0, 1, 1);

  gtk_container_add (GTK_CONTAINER (main_window), grid);

  retry=gtk_button_new_with_label("Retry Discovery");
  g_signal_connect(retry,"clicked",G_CALLBACK(retry_cb),NULL);
  gtk_grid_attach(GTK_GRID(grid), retry, 1, 1, 1, 1);

  start=gtk_button_new_with_label("Start Radio");
  g_signal_connect(start,"clicked",G_CALLBACK(start_cb),NULL);
  gtk_grid_attach(GTK_GRID(grid), start, 4, 1, 1, 1);

  //gtk_widget_show_all(main_window);

  g_idle_add(check_wisdom,NULL);

}

int main(int argc, char **argv) {
  GtkApplication *hpsdr;
  char text[1024];
  int rc;
  const char *homedir;

  if((homedir=getenv("HOME"))==NULL) {
    homedir=getpwuid(getuid())->pw_dir;
  }
  sprintf(text,"%s/.local",homedir);
  rc=mkdir(text,0777);
  sprintf(text,"%s/.local/share",homedir);
  rc=mkdir(text,0777);
  sprintf(text,"%s/.local/share/linhpsdr",homedir);
  rc=mkdir(text,0777);
  sprintf(text,"org.g0orx.hpsdr.pid%d",getpid());
  hpsdr=gtk_application_new(text, G_APPLICATION_FLAGS_NONE);
  g_signal_connect(hpsdr, "activate", G_CALLBACK(activate_hpsdr), NULL);
  rc=g_application_run(G_APPLICATION(hpsdr), argc, argv);
  g_object_unref(hpsdr);
  return rc;
}
