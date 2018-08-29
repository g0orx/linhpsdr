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
