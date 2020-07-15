/*
 * cwdaemon - morse sounding daemon for the parallel or serial port
 * Copyright (C) 2002 - 2005 Joop Stakenborg <pg4i@amsat.org>
 *                      and many authors, see the AUTHORS file.
 * Copyright (C) 2012 - 2015 Kamil Ignacak <acerion@wp.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef _CWDAEMON_H
#define _CWDAEMON_H

#ifndef OFF
# define OFF 0
#endif
#ifndef ON
# define ON 1
#endif
#define MICROPHONE 0
#define SOUNDCARD 1

#define MAX_DEVICE 20

#include <gtk/gtk.h>
#include <stdbool.h>

typedef struct cwdev_s {
	int (*init) (struct cwdev_s *, int fd);
	int (*free) (struct cwdev_s *);
	int (*reset) (struct cwdev_s *);
	int (*cw) (struct cwdev_s *, int onoff);
	int (*ptt) (struct cwdev_s *, int onoff);
	int (*ssbway) (struct cwdev_s *, int onoff);
	int (*switchband) (struct cwdev_s *, unsigned char bandswitch);
	int (*footswitch) (struct cwdev_s *);
	int fd;
	char *desc; /* "parport0", "ttyS0", "null" - name of device used for keying. */
}
cwdevice;


/* cwdaemon debug verbosity levels. */
enum cwdaemon_verbosity {
	CWDAEMON_VERBOSITY_N, /* None. Don't display any debug information. */
	CWDAEMON_VERBOSITY_E, /* Error */
	CWDAEMON_VERBOSITY_W, /* Warning */
	CWDAEMON_VERBOSITY_I, /* Info */
	CWDAEMON_VERBOSITY_D  /* Debug */
};


extern gpointer cwdaemon_thread(gpointer data);

extern GMutex cwdaemon_mutex;

extern bool keytx;
extern bool keysidetone;


extern void cwdaemon_stop(void);
extern void cwdaemon_close_socket(void);
extern void cwdaemon_close_libcw_output(void);

extern void cwdaemon_errmsg(const char *info, ...);
extern void cwdaemon_debug(int verbosity, const char *func, int line, const char *format, ...);

extern int dev_get_tty(const char *fname);
extern int dev_get_null(const char *fname);
extern int dev_get_parport(const char *fname);

#if defined (HAVE_LINUX_PPDEV_H) || defined (HAVE_DEV_PPBUS_PPI_H)
extern int lp_init (cwdevice * dev, int fd);
extern int lp_free (cwdevice * dev);
extern int lp_reset (cwdevice * dev);
extern int lp_cw (cwdevice * dev, int onoff);
extern int lp_ptt (cwdevice * dev, int onoff);
extern int lp_ssbway (cwdevice * dev, int onoff);
extern int lp_switchband (cwdevice * dev, unsigned char bandswitch);
extern int lp_footswitch (cwdevice * dev);
#endif

extern int ttys_init (cwdevice * dev, int fd);
extern int ttys_free (cwdevice * dev);
extern int ttys_reset (cwdevice * dev);
extern int ttys_cw (cwdevice * dev, int onoff);
extern int ttys_ptt (cwdevice * dev, int onoff);

extern int null_init (cwdevice * dev, int fd);
extern int null_free (cwdevice * dev);
extern int null_reset (cwdevice * dev);
extern int null_cw (cwdevice * dev, int onoff);
extern int null_ptt (cwdevice * dev, int onoff);

#endif /* _CWDAEMON_H */
