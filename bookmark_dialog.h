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

enum {
  ADD_BOOKMARK=0,
  VIEW_BOOKMARKS,
  EDIT_BOOKMARK
};

typedef struct _bookmark {
  void *previous;
  void *next;
  gint64 frequency_a;
  gint64 frequency_b;
  gint64 ctun_frequency;
  gboolean ctun;
  split_type split;
  int band;
  int mode;
  int filter;
  char *name;
} BOOKMARK;

typedef struct _bookmark_info {
  RECEIVER *rx;
  BOOKMARK *bookmark;
} BOOKMARK_INFO;

extern GtkWidget *create_bookmark_dialog(RECEIVER *rx,gint function,BOOKMARK *bookmark);
