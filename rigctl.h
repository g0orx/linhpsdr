/* Copyright (C)
* 2019 - John Melton, G0ORX/N6LYT
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

#ifndef RIGCTL_H
#define RIGCTL_H

/*
typedef struct _client {
  int socket;
  int address_length;
  struct sockaddr_in address;
  GThread *thread_id;
} CLIENT;

typedef struct _rigctl {
  int listening_port;
  GThread *server_thread_id;
  CLIENT *client;
} RIGCTL;
*/

extern void launch_rigctl (RECEIVER *rx);
extern int launch_serial ();
extern void disable_serial ();

void  close_rigctl_ports ();
//int   rigctlGetMode();
int   lookup_band(int);
char * rigctlGetFilter();
void set_freqB(long long);
int set_band(void *);
//extern int cat_control;
int set_alc(gpointer);
extern int rigctl_busy;

extern int rigctl_port_base;
extern int rigctl_enable;


#endif // RIGCTL_H
