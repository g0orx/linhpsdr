#ifndef RIGCTL_H
#define RIGCTL_H

extern void launch_rigctl(RECEIVER *rx);
extern void disable_rigctl(RECEIVER *rx);

extern int launch_serial(RECEIVER *rx);
extern void disable_serial(RECEIVER *rx);

extern int   rigctlGetMode();
extern int   lookup_band(int);
extern char * rigctlGetFilter();
extern void set_freqB(long long);
extern int set_alc(gpointer);

extern void rigctl_set_debug(RECEIVER *rx);

extern int cat_control;
extern int rigctl_busy;

extern int rigctl_port_base;
extern int rigctl_enable;


#endif // RIGCTL_H
