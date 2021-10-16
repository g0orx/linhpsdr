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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#endif

#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "mode.h"
#include "filter.h"
#include "bookmark_dialog.h"
#include "property.h"

static GtkWidget *name_text;

static BOOKMARK *bookmark_head=NULL;
static BOOKMARK *bookmark_tail=NULL;

enum {
  NAME_COLUMN,
  FREQUENCY_A_COLUMN,
  FREQUENCY_B_COLUMN,
  CTUN_FREQUENCY_COLUMN,
  CTUN_COLUMN,
  MODE_COLUMN,
  FILTER_COLUMN,
  SPLIT_COLUMN,
  N_COLUMNS
};

static GtkListStore *store;
static GtkWidget *view;
static GtkCellRenderer *renderer;
static GtkTreeIter iter;
static GtkTreeSortable *sortable;

static char *split_char[] = {"OFF","SPLIT","SAT","RSAT"};

static void save_bookmarks() {
  char filename[128];
  sprintf(filename,"%s/.local/share/linhpsdr/bookmarks",
                        g_get_home_dir());
  initProperties();

  int i=0;
  char name[80];
  char value[128];

  BOOKMARK *bookmark=bookmark_head;
  while(bookmark!=NULL) {
    sprintf(name,"bookmark[%d].name",i);
    setProperty(name,bookmark->name);
    sprintf(name,"bookmark[%d].frequency_a",i);
    sprintf(value,"%ld",bookmark->frequency_a);
    setProperty(name,value);
    sprintf(name,"bookmark[%d].frequency_b",i);
    sprintf(value,"%ld",bookmark->frequency_b);
    setProperty(name,value);
    sprintf(name,"bookmark[%d].ctun_frequency",i);
    sprintf(value,"%ld",bookmark->ctun_frequency);
    setProperty(name,value);
    sprintf(name,"bookmark[%d].ctun",i);
    sprintf(value,"%d",bookmark->ctun);
    setProperty(name,value);
    sprintf(name,"bookmark[%d].band",i);
    sprintf(value,"%d",bookmark->band);
    setProperty(name,value);
    sprintf(name,"bookmark[%d].mode",i);
    sprintf(value,"%d",bookmark->mode);
    setProperty(name,value);
    sprintf(name,"bookmark[%d].filter",i);
    sprintf(value,"%d",bookmark->filter);
    setProperty(name,value);
    sprintf(name,"bookmark[%d].split",i);
    sprintf(value,"%d",bookmark->split);
    setProperty(name,value);
    i++;
    bookmark=(BOOKMARK *)bookmark->next;
  }
  saveProperties(filename);
}

static void restore_bookmarks() {
  char filename[128];
  sprintf(filename,"%s/.local/share/linhpsdr/bookmarks",
                        g_get_home_dir());
  int i=0;
  char name[80];
  char *value;

  loadProperties(filename);

  while(1) {
    sprintf(name,"bookmark[%d].name",i);
    value=getProperty(name);
    if(value==NULL) {
      break;
    }
    BOOKMARK *bookmark=g_new0(BOOKMARK,1);
    bookmark->previous=NULL;
    bookmark->next=NULL;
    bookmark->name=g_new0(gchar,strlen(value)+1);
    strcpy(bookmark->name,value);
    sprintf(name,"bookmark[%d].frequency_a",i);
    value=getProperty(name);
    bookmark->frequency_a=atoll(value);
    sprintf(name,"bookmark[%d].frequency_b",i);
    value=getProperty(name);
    bookmark->frequency_b=atoll(value);
    sprintf(name,"bookmark[%d].ctun_frequency",i);
    value=getProperty(name);
    bookmark->ctun_frequency=atoll(value);
    sprintf(name,"bookmark[%d].ctun",i);
    value=getProperty(name);
    bookmark->ctun=atoi(value);
    sprintf(name,"bookmark[%d].band",i);
    value=getProperty(name);
    bookmark->band=atoi(value);;
    sprintf(name,"bookmark[%d].mode",i);
    value=getProperty(name);
    bookmark->mode=atoi(value);
    sprintf(name,"bookmark[%d].filter",i);
    value=getProperty(name);
    bookmark->filter=atoi(value);
    sprintf(name,"bookmark[%d].split",i);
    value=getProperty(name);
    bookmark->split=atoi(value);

    if(bookmark_tail==NULL) {
      bookmark_head=bookmark;
      bookmark_tail=bookmark;
    } else {
      bookmark_tail->next=(void *)bookmark;
      bookmark->previous=bookmark_tail;
      bookmark_tail=bookmark;
    }
    i++;
  }
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;
  rx->bookmark_dialog=NULL;
  return FALSE;
}

static gboolean add_cb(GtkWidget *widget,gpointer data) {
  RECEIVER *rx=(RECEIVER *)data;

  BOOKMARK *bookmark=g_new0(BOOKMARK,1);
  bookmark->name=g_new0(gchar,strlen(gtk_entry_get_text(GTK_ENTRY(name_text)))+1);
  strcpy(bookmark->name,gtk_entry_get_text(GTK_ENTRY(name_text)));
  bookmark->frequency_a=rx->frequency_a;
  bookmark->frequency_b=rx->frequency_b;
  bookmark->ctun_frequency=rx->ctun_frequency;
  bookmark->ctun=rx->ctun;
  bookmark->band=rx->band_a;
  bookmark->mode=rx->mode_a;
  bookmark->filter=rx->filter_a;
  bookmark->split=rx->split;
  bookmark->next=NULL;

  if(bookmark_tail==NULL) {
    bookmark_head=bookmark;
    bookmark_tail=bookmark;
  } else {
    bookmark_tail->next=(void *)bookmark;
    bookmark_tail=bookmark;
  }

  gtk_widget_destroy(rx->bookmark_dialog);
  rx->bookmark_dialog=NULL;
  save_bookmarks();
  return TRUE;
}

static gboolean update_cb(GtkWidget *widget,gpointer data) {
  BOOKMARK_INFO *info=(BOOKMARK_INFO *)data;

  g_free(info->bookmark->name);
  info->bookmark->name=g_new0(gchar,strlen(gtk_entry_get_text(GTK_ENTRY(name_text)))+1);
  strcpy(info->bookmark->name,gtk_entry_get_text(GTK_ENTRY(name_text)));

  gtk_widget_destroy(info->rx->bookmark_dialog);
  info->rx->bookmark_dialog=NULL;
  g_free(info);
  save_bookmarks();
  return TRUE;
}

static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data) {
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *name;
  gchar *frequency_a;
  gchar *frequency_b;
  gchar *ctun_frequency;
  gchar *ctun;
  gchar *mode;
  gchar *filter;
  gchar *split;

  if (gtk_tree_selection_get_selected(selection,&model,&iter)) {
    gtk_tree_model_get(model,&iter, NAME_COLUMN, &name, -1);
    gtk_tree_model_get(model,&iter, FREQUENCY_A_COLUMN, &frequency_a, -1);
    gtk_tree_model_get(model,&iter, FREQUENCY_B_COLUMN, &frequency_b, -1);
    gtk_tree_model_get(model,&iter, CTUN_FREQUENCY_COLUMN, &ctun_frequency, -1);
    gtk_tree_model_get(model,&iter, CTUN_COLUMN, &ctun, -1);
    gtk_tree_model_get(model,&iter, MODE_COLUMN, &mode, -1);
    gtk_tree_model_get(model,&iter, FILTER_COLUMN, &filter, -1);
    gtk_tree_model_get(model,&iter, SPLIT_COLUMN, &split, -1);
    BOOKMARK *bookmark=bookmark_head;
    while(bookmark!=NULL) {
      if(g_strcmp0(name,bookmark->name)==0) {
        break;
      }
      bookmark=(BOOKMARK *)bookmark->next;
    }
    g_free (name);
  }
}

void edit_cb(GtkWidget *menuitem,gpointer data) {
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar *name;
  gchar *frequency_a;
  gchar *frequency_b;
  gchar *ctun_frequency;
  gchar *ctun;
  gchar *mode;
  gchar *filter;
  gchar *split;
  RECEIVER *rx=(RECEIVER *)data;
  BOOKMARK *bookmark=bookmark_head;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  if (gtk_tree_selection_get_selected(selection,&model,&iter)) {
    gtk_tree_model_get(model,&iter, NAME_COLUMN, &name, -1);
    gtk_tree_model_get(model,&iter, FREQUENCY_A_COLUMN, &frequency_a, -1);
    gtk_tree_model_get(model,&iter, FREQUENCY_B_COLUMN, &frequency_b, -1);
    gtk_tree_model_get(model,&iter, CTUN_FREQUENCY_COLUMN, &ctun_frequency, -1);
    gtk_tree_model_get(model,&iter, CTUN_COLUMN, &ctun, -1);
    gtk_tree_model_get(model,&iter, MODE_COLUMN, &mode, -1);
    gtk_tree_model_get(model,&iter, FILTER_COLUMN, &filter, -1);
    gtk_tree_model_get(model,&iter, SPLIT_COLUMN, &split, -1);
    while(bookmark!=NULL) {
      if(g_strcmp0(name,bookmark->name)==0) {
        break;
      }
      bookmark=(BOOKMARK *)bookmark->next;
    }
    //g_free (name);
    if(bookmark!=NULL) {
      // edit this one
      gtk_widget_destroy(rx->bookmark_dialog);
      rx->bookmark_dialog=create_bookmark_dialog(rx,EDIT_BOOKMARK,bookmark);
    }
  }

}

void delete_cb(GtkWidget *menuitem,gpointer data) {
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar *name;
  gchar *frequency_a;
  gchar *frequency_b;
  gchar *ctun_frequency;
  gchar *ctun;
  gchar *mode;
  gchar *filter;
  gchar *split;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  if (gtk_tree_selection_get_selected(selection,&model,&iter)) {
    gtk_tree_model_get(model,&iter, NAME_COLUMN, &name, -1);
    gtk_tree_model_get(model,&iter, FREQUENCY_A_COLUMN, &frequency_a, -1);
    gtk_tree_model_get(model,&iter, FREQUENCY_B_COLUMN, &frequency_b, -1);
    gtk_tree_model_get(model,&iter, CTUN_FREQUENCY_COLUMN, &ctun_frequency, -1);
    gtk_tree_model_get(model,&iter, CTUN_COLUMN, &ctun, -1);
    gtk_tree_model_get(model,&iter, MODE_COLUMN, &mode, -1);
    gtk_tree_model_get(model,&iter, FILTER_COLUMN, &filter, -1);
    gtk_tree_model_get(model,&iter, SPLIT_COLUMN, &split, -1);
    BOOKMARK *bookmark=bookmark_head;
    while(bookmark!=NULL) {
      if(g_strcmp0(name,bookmark->name)==0) {
        break;
      }
      bookmark=(BOOKMARK *)bookmark->next;
    }
    g_free (name);
    if(bookmark!=NULL) {
      // delete this one
      if(bookmark->previous==NULL) {
        // first in list
        bookmark_head=bookmark->next;
      } else {
        BOOKMARK *p=(BOOKMARK *)bookmark->previous;
        p->next=bookmark->next;
      }
      if(bookmark->next==NULL) {
        bookmark_tail=bookmark->previous;
      } else {
        BOOKMARK *n=(BOOKMARK *)bookmark->next;
        n->previous=bookmark->previous;
      }
      gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
      save_bookmarks();
    }
  }
}

void cancel_cb(GtkWidget *menuitem,gpointer userdata) {
}

gboolean button_pressed_cb(GtkWidget *view,GdkEventButton *event,gpointer data) {
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar *name;
  GtkWidget *menu;
  GtkWidget *menuitem;
  char label[128];
  RECEIVER *rx=(RECEIVER *)data;

  if(event->type==GDK_BUTTON_PRESS && event->button==3) {
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    if (gtk_tree_selection_get_selected(selection,&model,&iter)) {
      gtk_tree_model_get(model,&iter, NAME_COLUMN, &name, -1);
      menu=gtk_menu_new();
      sprintf(label,"Edit: %s",name);
      menuitem=gtk_menu_item_new_with_label(label);
      g_signal_connect(menuitem,"activate",G_CALLBACK(edit_cb),rx);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);
      sprintf(label,"Delete: %s",name);
      menuitem=gtk_menu_item_new_with_label(label);
      g_signal_connect(menuitem,"activate",G_CALLBACK(delete_cb),rx);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);
      menuitem=gtk_menu_item_new_with_label("Cancel");
      g_signal_connect(menuitem,"activate",G_CALLBACK(cancel_cb),rx);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);
      gtk_widget_show_all(menu);
#if GTK_CHECK_VERSION(3,22,0)
      gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent *)event);
#else
      gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
#endif
      g_free (name);
    }
    return TRUE;
  }
  return FALSE;
}

void tree_selection_activated_cb(GtkTreeView *treeview,GtkTreePath *path,GtkTreeViewColumn *col,gpointer data) {
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar *name;
  gchar *frequency_a;
  gchar *frequency_b;
  gchar *ctun_frequency;
  gchar *ctun;
  gchar *mode;
  gchar *filter;
  gchar *split;
  RECEIVER *rx=(RECEIVER *)data;

  model = gtk_tree_view_get_model(treeview);
  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gtk_tree_model_get(model,&iter,NAME_COLUMN,&name, -1);
    gtk_tree_model_get(model,&iter,FREQUENCY_A_COLUMN,&frequency_a, -1);
    gtk_tree_model_get(model,&iter,FREQUENCY_B_COLUMN,&frequency_b, -1);
    gtk_tree_model_get(model,&iter,CTUN_FREQUENCY_COLUMN,&ctun_frequency, -1);
    gtk_tree_model_get(model,&iter,CTUN_COLUMN,&ctun, -1);
    gtk_tree_model_get(model,&iter,MODE_COLUMN,&mode, -1);
    gtk_tree_model_get(model,&iter,FILTER_COLUMN,&filter, -1);
    gtk_tree_model_get(model,&iter,SPLIT_COLUMN,&split, -1);
    BOOKMARK *bookmark=bookmark_head;
    while(bookmark!=NULL) {
      if(g_strcmp0(name,bookmark->name)==0) {
        break;
      }
      bookmark=(BOOKMARK *)bookmark->next;
    }
    g_free (name);
    if(bookmark!=NULL) {
      rx->frequency_a=bookmark->frequency_a;
      rx->frequency_b=bookmark->frequency_b;
      rx->ctun_frequency=bookmark->ctun_frequency;
      rx->ctun=bookmark->ctun;
      rx->mode_a=bookmark->mode;
      rx->filter_a=bookmark->filter;
      rx->split=bookmark->split;
      rx->band_a=bookmark->band;
      receiver_mode_changed(rx,rx->mode_a);
      receiver_filter_changed(rx,rx->filter_a);
      frequency_changed(rx);
      if(radio->transmitter) {
      if(radio->transmitter->rx==rx) {
          if(rx->split) {
            transmitter_set_mode(radio->transmitter,rx->mode_b);
          } else {
            transmitter_set_mode(radio->transmitter,rx->mode_a);
          }
        }
      }
    }
  }
}

static gint compare_func(GtkTreeModel *model,GtkTreeIter *a,GtkTreeIter *b,gpointer data) {
  gint col=GPOINTER_TO_INT(data);
  gchar *astring, *bstring;
  gint result=0;

  gtk_tree_model_get(model, a, col, &astring, -1);
  gtk_tree_model_get(model, b, col, &bstring, -1);

  if (astring == NULL || bstring == NULL) {
    if (astring == NULL && bstring == NULL) {
      result=0;
    } else {
      result = (astring == NULL) ? -1 : 1;
    }
  } else {
    result = g_utf8_collate(astring,bstring);
  }
  g_free(astring);
  g_free(bstring);
  return result;
}

GtkWidget *create_bookmark_dialog(RECEIVER *rx,gint function,BOOKMARK *bookmark) {
  int x;
  int y;
  gchar temp[128];
  gchar temp_a[128];
  gchar temp_b[128];
  gchar temp_ctun_frequency[128];
  gchar temp_ctun[128];

  if(bookmark_head==NULL) {
    restore_bookmarks();
  }
  GtkWidget *dialog=gtk_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(main_window));
  g_signal_connect (dialog,"delete_event",G_CALLBACK(delete_event),(gpointer)rx);
  GtkWidget *content=gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  GtkWidget *grid=gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(content),grid);

  switch(function) {
    case ADD_BOOKMARK:
      g_snprintf((gchar *)&temp,sizeof(temp),"Linux HPSDR: Bookmarks");
      gtk_window_set_title(GTK_WINDOW(dialog),temp);

      x=0;
      y=0;
      g_snprintf((gchar *)&temp_a,sizeof(temp_a),"%4ld.%03ld.%03ld",rx->frequency_a/(long int)1000000,(rx->frequency_a%(long int)1000000)/(long int)1000,rx->frequency_a%(long int)1000);
      g_snprintf((gchar *)&temp_b,sizeof(temp_b),"%4ld.%03ld.%03ld",rx->frequency_b/(long int)1000000,(rx->frequency_b%(long int)1000000)/(long int)1000,rx->frequency_b%(long int)1000);
      g_snprintf((gchar *)&temp_ctun_frequency,sizeof(temp),"%4ld.%03ld.%03ld",rx->ctun_frequency/(long int)1000000,(rx->ctun_frequency%(long int)1000000)/(long int)1000,rx->ctun_frequency%(long int)1000);
      GtkWidget *name_title=gtk_label_new("Name: ");
      gtk_grid_attach(GTK_GRID(grid),name_title,x,y,1,1);
      x++;
      name_text=gtk_entry_new();
      if(rx->ctun) {
        gtk_entry_set_text(GTK_ENTRY(name_text),temp_ctun_frequency);
      } else {
        gtk_entry_set_text(GTK_ENTRY(name_text),temp_a);
      }
      gtk_grid_attach(GTK_GRID(grid),name_text,x,y,1,1);
      y++;
      x=0;
      GtkWidget *frequency_title=gtk_label_new("Frequency A: ");
      gtk_grid_attach(GTK_GRID(grid),frequency_title,x,y,1,1);
      x++;
      GtkWidget *frequency_text=gtk_label_new(temp_a);
      gtk_grid_attach(GTK_GRID(grid),frequency_text,x,y,1,1);
      y++;
      x=0;
      frequency_title=gtk_label_new("Frequency B: ");
      gtk_grid_attach(GTK_GRID(grid),frequency_title,x,y,1,1);
      x++;
      frequency_text=gtk_label_new(temp_b);
      gtk_grid_attach(GTK_GRID(grid),frequency_text,x,y,1,1);
      y++;
      x=0;
      GtkWidget *ctun_frequency_title=gtk_label_new("CTUN Frequency: ");
      gtk_grid_attach(GTK_GRID(grid),ctun_frequency_title,x,y,1,1);
      x++;
      frequency_text=gtk_label_new(temp_ctun_frequency);
      gtk_grid_attach(GTK_GRID(grid),frequency_text,x,y,1,1);
      y++;
      x=0;
      GtkWidget *ctun_title=gtk_label_new("CTUN: ");
      gtk_grid_attach(GTK_GRID(grid),ctun_title,x,y,1,1);
      x++;
      g_snprintf((gchar *)&temp,sizeof(temp),"%d",rx->ctun);
      GtkWidget *ctun_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),ctun_text,x,y,1,1);
      y++;
      x=0;
      GtkWidget *mode_title=gtk_label_new("Mode: ");
      gtk_grid_attach(GTK_GRID(grid),mode_title,x,y,1,1);
      x++;
      g_snprintf((gchar *)&temp,sizeof(temp),"%s",mode_string[rx->mode_a]);
      GtkWidget *mode_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),mode_text,x,y,1,1);
      y++;
      x=0;
      GtkWidget *filter_title=gtk_label_new("Filter: ");
      gtk_grid_attach(GTK_GRID(grid),filter_title,x,y,1,1);
      x++;
      FILTER* band_filters=filters[rx->mode_a];
      g_snprintf((gchar *)&temp,sizeof(temp),"%s",band_filters[rx->filter_a].title);
      GtkWidget *filter_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),filter_text,x,y,1,1);

      y++;
      x=0; 
      GtkWidget *split_title=gtk_label_new("Split: ");
      gtk_grid_attach(GTK_GRID(grid),split_title,x,y,1,1);
      x++;
      GtkWidget *split_text=gtk_label_new(split_char[rx->split]);
      gtk_grid_attach(GTK_GRID(grid),split_text,x,y,1,1);

      y++;
      x=0; 
      GtkWidget* button=gtk_button_new_with_label("Add Bookmark");
      g_signal_connect(button,"clicked",G_CALLBACK(add_cb),(gpointer)rx);
      gtk_grid_attach(GTK_GRID(grid),button,x,y,1,1);

      break;
    case VIEW_BOOKMARKS:
      g_snprintf((gchar *)&temp,sizeof(temp),"Linux HPSDR: RX-%d: Bookmarks",rx->channel);
      gtk_window_set_title(GTK_WINDOW(dialog),temp);

      store=gtk_list_store_new(N_COLUMNS,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
      sortable=GTK_TREE_SORTABLE(store);

      gtk_tree_sortable_set_sort_func(sortable,NAME_COLUMN,compare_func,GINT_TO_POINTER(NAME_COLUMN),NULL);
      gtk_tree_sortable_set_sort_func(sortable,FREQUENCY_A_COLUMN,compare_func,GINT_TO_POINTER(FREQUENCY_A_COLUMN),NULL);
      gtk_tree_sortable_set_sort_func(sortable,FREQUENCY_B_COLUMN,compare_func,GINT_TO_POINTER(FREQUENCY_B_COLUMN),NULL);
      gtk_tree_sortable_set_sort_func(sortable,CTUN_FREQUENCY_COLUMN,compare_func,GINT_TO_POINTER(CTUN_FREQUENCY_COLUMN),NULL);
      gtk_tree_sortable_set_sort_func(sortable,CTUN_COLUMN,compare_func,GINT_TO_POINTER(CTUN_COLUMN),NULL);
      gtk_tree_sortable_set_sort_func(sortable,MODE_COLUMN,compare_func,GINT_TO_POINTER(MODE_COLUMN),NULL);
      gtk_tree_sortable_set_sort_func(sortable,FILTER_COLUMN,compare_func,GINT_TO_POINTER(FILTER_COLUMN),NULL);
      gtk_tree_sortable_set_sort_func(sortable,SPLIT_COLUMN,compare_func,GINT_TO_POINTER(SPLIT_COLUMN),NULL);
      gtk_tree_sortable_set_sort_column_id(sortable, NAME_COLUMN, GTK_SORT_ASCENDING);

      view=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Name", renderer, "text", NAME_COLUMN, NULL);
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Frequency A", renderer, "text", FREQUENCY_A_COLUMN, NULL);
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Frequency B", renderer, "text", FREQUENCY_B_COLUMN, NULL);
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "CTUN Frequency", renderer, "text", CTUN_FREQUENCY_COLUMN, NULL);
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "CTUN", renderer, "text", CTUN_COLUMN, NULL);
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Mode", renderer, "text", MODE_COLUMN, NULL);
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Filter", renderer, "text", FILTER_COLUMN, NULL);
      renderer=gtk_cell_renderer_text_new();
      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "Split", renderer, "text", SPLIT_COLUMN, NULL);
      BOOKMARK *bmk=bookmark_head;
      while(bmk!=NULL) {
        FILTER* band_filters=filters[bmk->mode];
        //FILTER* band_filter=&band_filters[bmk->filter]; // TO REMOVE
        g_snprintf((gchar *)&temp_a,sizeof(temp),"%4ld.%03ld.%03ld",bmk->frequency_a/(long int)1000000,(bmk->frequency_a%(long int)1000000)/(long int)1000,bmk->frequency_a%(long int)1000);
        g_snprintf((gchar *)&temp_b,sizeof(temp),"%4ld.%03ld.%03ld",bmk->frequency_b/(long int)1000000,(bmk->frequency_b%(long int)1000000)/(long int)1000,bmk->frequency_b%(long int)1000);
        g_snprintf((gchar *)&temp_ctun_frequency,sizeof(temp_ctun_frequency),"%4ld.%03ld.%03ld",bmk->ctun_frequency/(long int)1000000,(bmk->ctun_frequency%(long int)1000000)/(long int)1000,bmk->ctun_frequency%(long int)1000);
        g_snprintf((gchar *)&temp_ctun,sizeof(temp_ctun),"%d",bmk->ctun);

        gtk_list_store_append(store,&iter);
        gtk_list_store_set(store,&iter,
                           NAME_COLUMN, bmk->name,
                           FREQUENCY_A_COLUMN, temp_a,
                           FREQUENCY_B_COLUMN, temp_b,
                           CTUN_FREQUENCY_COLUMN, temp_ctun_frequency,
                           CTUN_COLUMN, temp_ctun,
                           MODE_COLUMN, mode_string[bmk->mode],
                           FILTER_COLUMN, band_filters[bmk->filter].title,
                           SPLIT_COLUMN, split_char[bmk->split],
                           -1);
        bmk=(BOOKMARK *)bmk->next;
      }
      gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(view),TRUE);
      gtk_grid_attach(GTK_GRID(grid), view, 0, 0, 4, 1);
      GtkTreeSelection *selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
      gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
      g_signal_connect(G_OBJECT(selection),"changed",G_CALLBACK(tree_selection_changed_cb),rx);
      g_signal_connect(view,"row-activated", G_CALLBACK(tree_selection_activated_cb), rx);
      g_signal_connect(view,"button-press-event", G_CALLBACK(button_pressed_cb), rx);
      break;
    case EDIT_BOOKMARK:
      g_snprintf((gchar *)&temp,sizeof(temp),"Linux HPSDR: Bookmark");
      gtk_window_set_title(GTK_WINDOW(dialog),temp);

      x=0;
      y=0;
      name_title=gtk_label_new("Name: ");
      gtk_grid_attach(GTK_GRID(grid),name_title,x,y,1,1);
      x++;
      name_text=gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(name_text),bookmark->name);
      gtk_grid_attach(GTK_GRID(grid),name_text,x,y,1,1);
      y++;
      x=0;
      g_snprintf((gchar *)&temp_a,sizeof(temp_a),"%4ld.%03ld.%03ld",bookmark->frequency_a/(long int)1000000,(bookmark->frequency_a%(long int)1000000)/(long int)1000,bookmark->frequency_a%(long int)1000);
      frequency_title=gtk_label_new("Frequency A: ");
      gtk_grid_attach(GTK_GRID(grid),frequency_title,x,y,1,1);
      x++;
      frequency_text=gtk_label_new(temp_a);
      gtk_grid_attach(GTK_GRID(grid),frequency_text,x,y,1,1);
      y++;
      x=0;
      g_snprintf((gchar *)&temp_b,sizeof(temp_b),"%4ld.%03ld.%03ld",bookmark->frequency_b/(long int)1000000,(bookmark->frequency_b%(long int)1000000)/(long int)1000,bookmark->frequency_b%(long int)1000);
      frequency_title=gtk_label_new("Frequency B: ");
      gtk_grid_attach(GTK_GRID(grid),frequency_title,x,y,1,1);
      x++;
      frequency_text=gtk_label_new(temp_b);
      gtk_grid_attach(GTK_GRID(grid),frequency_text,x,y,1,1);
      y++;
      x=0;
      g_snprintf((gchar *)&temp,sizeof(temp),"%4ld.%03ld.%03ld",bookmark->ctun_frequency/(long int)1000000,(bookmark->ctun_frequency%(long int)1000000)/(long int)1000,bookmark->ctun_frequency%(long int)1000);
      frequency_title=gtk_label_new("CTUN Frequency: ");
      gtk_grid_attach(GTK_GRID(grid),frequency_title,x,y,1,1);
      x++;
      frequency_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),frequency_text,x,y,1,1);
      y++;
      x=0;
      g_snprintf((gchar *)&temp,sizeof(temp),"%d",bookmark->ctun);
      frequency_title=gtk_label_new("CTUN: ");
      gtk_grid_attach(GTK_GRID(grid),frequency_title,x,y,1,1);
      x++;
      frequency_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),frequency_text,x,y,1,1);
      y++;
      x=0;
      mode_title=gtk_label_new("Mode: ");
      gtk_grid_attach(GTK_GRID(grid),mode_title,x,y,1,1);
      x++;
      g_snprintf((gchar *)&temp,sizeof(temp),"%s",mode_string[bookmark->mode]);
      mode_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),mode_text,x,y,1,1);
      y++;
      x=0;
      filter_title=gtk_label_new("Filter: ");
      gtk_grid_attach(GTK_GRID(grid),filter_title,x,y,1,1);
      x++;
      band_filters=filters[rx->mode_a];
      //band_filter=&band_filters[bookmark->filter]; // TO REMOVE
      g_snprintf((gchar *)&temp,sizeof(temp),"%s",band_filters[bookmark->filter].title);
      filter_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),filter_text,x,y,1,1);

      y++;
      x=0;
      filter_title=gtk_label_new("Split: ");
      gtk_grid_attach(GTK_GRID(grid),filter_title,x,y,1,1);
      x++;
      g_snprintf((gchar *)&temp,sizeof(temp),"%s",split_char[bookmark->split]);
      filter_text=gtk_label_new(temp);
      gtk_grid_attach(GTK_GRID(grid),filter_text,x,y,1,1);

      y++;
      x=0;
      button=gtk_button_new_with_label("Update Bookmark");
      BOOKMARK_INFO *info=g_new0(BOOKMARK_INFO,1);
      info->rx=rx;
      info->bookmark=bookmark;
      g_signal_connect(button,"clicked",G_CALLBACK(update_cb),(gpointer)info);
      gtk_grid_attach(GTK_GRID(grid),button,x,y,1,1);

      break;
  }

  gtk_widget_show_all(dialog);
  return dialog;
}
