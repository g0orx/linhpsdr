#ifndef RIGCTL_H
#define RIGCTL_H

void launch_rigctl (RECEIVER *rx);
int launch_serial (RECEIVER *rx);
void disable_serial (RECEIVER *rx);

void  close_rigctl_ports ();
int   rigctlGetMode();
int   lookup_band(int);
char * rigctlGetFilter();
void set_freqB(long long);
extern int cat_control;
int set_alc(gpointer);
extern int rigctl_busy;

extern int rigctl_port_base;
extern int rigctl_enable;


#endif // RIGCTL_H
