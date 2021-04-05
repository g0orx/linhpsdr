/*
 * cwdaemon - morse sounding daemon for the parallel or serial port
 * Copyright (C) 2002 - 2005 Joop Stakenborg <pg4i@amsat.org>
 *		        and many authors, see the AUTHORS file.
 * Copyright (C) 2012 - 2015 Kamil Ignacak <acerion@wp.pl>
 * Modififed 2020 by M5EVT.
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

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#define _POSIX_C_SOURCE 200809L /* nanosleep(), strdup() */
#define _GNU_SOURCE /* getopt_long() */


/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

#include <stdlib.h>
#include <libcw.h>
#include <libcw_debug.h>
#include "cwdaemon.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <gtk/gtk.h>

/**
   \file cwdaemon.c

   cwdaemon exchanges data with client through messages. Most of
   messages are sent by client application to cwdaemon - those
   are called in this file "requests". On several occasions cwdaemon
   sends some data back to the client. Such messages are called
   "replies".

   message:
       - request
       - reply

   Size of a message is not constant.
   Maximal size of a message is CWDAEMON_MESSAGE_SIZE_MAX.
*/


#include "band.h"
#include "channel.h"
#include "discovered.h"
#include "mode.h"
#include "filter.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "protocol1.h"
#include "audio.h"
#include "signal.h"
#include "vfo.h"
#include "transmitter.h"



/* cwdaemon constants. */
#define CWDAEMON_MORSE_SPEED_DEFAULT           30 /* [wpm] */
#define CWDAEMON_MORSE_TONE_DEFAULT           650 /* [Hz] */
#define CWDAEMON_MORSE_VOLUME_DEFAULT          10 /* [%] */

/* TODO: why the limitation to 50 ms? Is it enough? */
#define CWDAEMON_PTT_DELAY_DEFAULT              0 /* [ms] */
#define CWDAEMON_PTT_DELAY_MIN                  0 /* [ms] */
#define CWDAEMON_PTT_DELAY_MAX                 50 /* [ms] */

/* Notice that the range accepted by cwdaemon is different than that
   accepted by libcw. */
#define CWDAEMON_MORSE_WEIGHTING_DEFAULT        0
#define CWDAEMON_MORSE_WEIGHTING_MIN          -50
#define CWDAEMON_MORSE_WEIGHTING_MAX           50

#define CWDAEMON_NETWORK_PORT_DEFAULT                  6789
#define CWDAEMON_AUDIO_SYSTEM_DEFAULT      CW_AUDIO_PA /* Console buzzer, from libcw.h. */
#define CWDAEMON_VERBOSITY_DEFAULT     CWDAEMON_VERBOSITY_W /* Threshold of verbosity of debug strings. */

#define CWDAEMON_USECS_PER_MSEC         1000 /* Just to avoid magic numbers. */
#define CWDAEMON_USECS_PER_SEC       1000000 /* Just to avoid magic numbers. */
#define CWDAEMON_MESSAGE_SIZE_MAX        256 /* Maximal size of single message. */
#define CWDAEMON_REQUEST_QUEUE_SIZE_MAX 4000 /* Maximal size of common buffer/fifo where requests may be pushed to. */

#define CWDAEMON_TUNE_SECONDS_MAX  10 /* Maximal time of tuning. TODO: why the limitation to 10 s? Is it enough? */

GMutex cwdaemon_mutex;

bool keytx;
bool keysidetone;


/* Default values of parameters, may be modified only through
   command line arguments passed to cwdaemon.

   After setting these variables with values passed in command line,
   these become the default state of cwdaemon.  Values of default_*
   will be used when resetting libcw and cwdaemon to initial state. */
static int default_morse_speed  = CWDAEMON_MORSE_SPEED_DEFAULT;
static int default_morse_tone   = CWDAEMON_MORSE_TONE_DEFAULT;
static int default_morse_volume = CWDAEMON_MORSE_VOLUME_DEFAULT;
static int default_ptt_delay    = CWDAEMON_PTT_DELAY_DEFAULT;
static int default_audio_system = CWDAEMON_AUDIO_SYSTEM_DEFAULT;
static int default_weighting    = CWDAEMON_MORSE_WEIGHTING_DEFAULT;

/* Actual values of parameters, used to control ongoing operation of
   cwdaemon+libcw. These values can be modified through requests
   received from socket in cwdaemon_receive(). */
static int current_morse_speed  = CWDAEMON_MORSE_SPEED_DEFAULT;
static int current_morse_tone   = CWDAEMON_MORSE_TONE_DEFAULT;
static int current_morse_volume = CWDAEMON_MORSE_VOLUME_DEFAULT;
static int current_ptt_delay    = CWDAEMON_PTT_DELAY_DEFAULT;
static int current_audio_system = CWDAEMON_AUDIO_SYSTEM_DEFAULT;
static int current_weighting    = CWDAEMON_MORSE_WEIGHTING_DEFAULT;

/* Level of libcw's tone queue that triggers 'callback for low level
   in tone queue'.  The callback function is
   cwdaemon_tone_queue_low_callback(), it is registered with
   cw_register_tone_queue_low_callback().

   I REALLY don't think that you would want to set it to any value
   other than '1'. */
static const int tq_low_watermark = 1;

/* Quick and dirty solution to following problem: when cwdaemon for
   some reason fails to open audio output, and attempts to play
   characters received from client, it crashes.  It doesn't know that
   it attempts to play to closed audio output.

   This is a flag telling cwdaemon if audio output is available or not.

   TODO: the variable is almost unused. Start using it.

   TODO: decide on terminology: "audio system" or "sound system". */
static bool has_audio_output = false;




/* Network variables. */

/* cwdaemon usually receives requests from client, but on occasions
   it needs to send a reply back. This is why in addition to
   request_* we also have reply_* */

//static int socket_descriptor = 0;

/* Default UDP port we listen on. Can be changed only through command
   line switch.

   There is a code path suggesting that it was possible to change the
   port using network request, but now this code path is marked as
   "obsolete".
 */
//static int port = CWDAEMON_NETWORK_PORT_DEFAULT;

//static struct sockaddr_in request_addr;
//static socklen_t          request_addrlen;

//static struct sockaddr_in reply_addr;
//static socklen_t          reply_addrlen;

static char reply_buffer[CWDAEMON_MESSAGE_SIZE_MAX];



/* Debug variables. */


/* This table is addressed with values defined in "enum
   cwdaemon_verbosity" (src/cwdaemon.h). */
static const char *cwdaemon_verbosity_labels[] = {
	"NN",    /* None - obviously this label will never be used. */
	"EE",    /* Error. */
	"WW",    /* Warning. */
	"II",    /* Information. */
	"DD" };  /* Debug. */

/* Various variables. */
static int wordmode = 0;               /* Start in character mode. */
static int inactivity_seconds = 9999;  /* Inactive since nnn seconds. */


/* Incoming requests without Escape code are stored in this pseudo-FIFO
   before they are played. */
static char request_queue[CWDAEMON_REQUEST_QUEUE_SIZE_MAX];



/* Flag for PTT state/behaviour. */
static unsigned char ptt_flag = 0;

/* Automatically turn PTT on and off.
   Turn PTT on when starting to play Morse characters, and turn PTT off when
   there are no more characters to play.
   "Automatically" means that cwdaemon toggles PTT without any additional
   actions taken by client. Client doesn't have to tell cwdaemon when to
   turn PTT on/off - this is done by cwdaemon itself, automatically.

   If ptt delay is non-zero, cwdaemon performs delay between turning PTT on
   and starting to play Morse characters.
   TODO: is there a delay before turning PTT off? */
#define PTT_ACTIVE_AUTO		0x01

/* PTT is turned on and off manually.
   It is the client who decides when to turn the PTT on and off.
   The client has to send 'a' escape code, followed by '1' or '0' to
   'manually' turn PTT on or off.
   'MANUAL' - the opposite of 'AUTO' where it is cwdaemon that decides
   when to turn PTT on and off.
   Perhaps "PTT_ON_REQUEST" would be a better name of the constant. */
#define PTT_ACTIVE_MANUAL	0x02

/* Don't turn PTT off until cwdaemon sends back an echo to client.
   client may request echoing back to it a reply when cwdaemon finishes
   playing given request. PTT shouldn't be turned off when sending the
   reply (TODO: why it shouldn't?).

   This flag is set whenever client sends request for sending back a
   reply (i.e. either <ESC>h request, or caret request).

   This flag is re-set whenever such reply is sent (to be more
   precise: after playing a requested text, but just before sending to
   the client the requested reply). */
#define PTT_ACTIVE_ECHO		0x04



void cwdaemon_set_ptt_on(cwdevice *device, const char *info);
void cwdaemon_set_ptt_off(cwdevice *device, const char *info);
void cwdaemon_switch_band(cwdevice *device, unsigned int band);

void cwdaemon_play_request(char *request);

void cwdaemon_tune(uint32_t seconds);
void cwdaemon_keyingevent(void *arg, int keystate);
void cwdaemon_prepare_reply(char *reply, const char *request, size_t n);
void cwdaemon_tone_queue_low_callback(void *arg);

bool    cwdaemon_initialize_socket(void);
void    cwdaemon_close_socket(void);
ssize_t cwdaemon_sendto(const char *reply);
int     cwdaemon_recvfrom(char *request, int n);
int     cwdaemon_receive(void);
void    cwdaemon_handle_escaped_request(char *request);

void cwdaemon_reset_almost_all(void);

/* Functions managing libcw output. */
bool cwdaemon_open_libcw_output(int audio_system);
void cwdaemon_close_libcw_output(void);
void cwdaemon_reset_libcw_output(void);

/* Utility functions. */
bool cwdaemon_get_long(const char *buf, long *lvp);
void cwdaemon_udelay(unsigned long us);

//static bool cwdaemon_params_cwdevice(const char *optarg);
static bool cwdaemon_params_wpm(int *wpm, const char *optarg);
static bool cwdaemon_params_tune(uint32_t *seconds, const char *optarg);
static int  cwdaemon_params_pttdelay(int *delay, const char *optarg);
static bool cwdaemon_params_volume(int *volume, const char *optarg);
static bool cwdaemon_params_weighting(int *weighting, const char *optarg);
static bool cwdaemon_params_tone(int *tone, const char *optarg);
static bool cwdaemon_params_system(int *system, const char *optarg);
static bool cwdaemon_params_ptt_on_off(const char *optarg);

/* Auto, manual, echo. */
static char cwdaemon_debug_ptt_flag[3 + 1];
static const char *cwdaemon_debug_ptt_flags(void);

/* Selected keying device:
   serial port (cwdevice_ttys) || parallel port (cwdevice_lp) || null (cwdevice_null).
   It should be configured with cwdaemon_cwdevice_set(). */
/* FIXME: if no device is specified in command line, and no physical
   device is available, the global_cwdevice is NULL, which causes the
   program to break. */
static cwdevice *global_cwdevice = NULL;

const char *cwdaemon_debug_ptt_flags(void)
{

	if (ptt_flag & PTT_ACTIVE_AUTO) {
		cwdaemon_debug_ptt_flag[0] = 'A';
	} else {
		cwdaemon_debug_ptt_flag[0] = 'a';
	}

	if (ptt_flag & PTT_ACTIVE_MANUAL) {
		cwdaemon_debug_ptt_flag[1] = 'M';
	} else {
		cwdaemon_debug_ptt_flag[1] = 'm';
	}

	if (ptt_flag & PTT_ACTIVE_ECHO) {
		cwdaemon_debug_ptt_flag[2] = 'E';
	} else {
		cwdaemon_debug_ptt_flag[2] = 'e';
	}

	cwdaemon_debug_ptt_flag[3] = '\0';

	return cwdaemon_debug_ptt_flag;
}

/**
   \brief Sleep for specified amount of microseconds

   Function can detect an interrupt from a signal, and continue sleeping,
   but only once.

   \param us - microseconds to sleep
*/
void cwdaemon_udelay(unsigned long us)
{
	struct timespec sleeptime, time_remainder;

	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = us * 1000;

	if (nanosleep(&sleeptime, &time_remainder) == -1) {
		if (errno == EINTR) {
			nanosleep(&time_remainder, NULL);
		} else {
			printf("Nanosleep");
		}
	}

	return;
}

/**
   \brief Switch PTT on

   \param device - current keying device
   \param info - debug information displayed when performing the switching
*/
void cwdaemon_set_ptt_on(cwdevice *device, const char *info)
{
	/* For backward compatibility it is assumed that ptt_delay=0
	   means "cwdaemon shouldn't turn PTT on, at all". */

	if (current_ptt_delay && !(ptt_flag & PTT_ACTIVE_AUTO)) {
		device->ptt(device, ON);
		cwdaemon_udelay(current_ptt_delay * CWDAEMON_USECS_PER_MSEC);

		ptt_flag |= PTT_ACTIVE_AUTO;
	}

	return;
}

/**
   \brief Switch PTT off

   \param device - current keying device
   \param info - debug information displayed when performing the switching
*/
void cwdaemon_set_ptt_off(cwdevice *device, const char *info)
{
	device->ptt(device, OFF);
	ptt_flag = 0;
	return;
}

/**
   \brief Tune for a number of seconds

   Play a continuous sound for a given number of seconds.

   Parameter type is uint32_t, which gives us maximum of 4294967295
   seconds, i.e. ~136 years. Should be enough.

   TODO: change the argument type to size_t.

   \param seconds - time of tuning
*/
void cwdaemon_tune(uint32_t seconds)
{
	if (seconds > 0) {
		cw_flush_tone_queue();
		cwdaemon_set_ptt_on(global_cwdevice, "PTT (TUNE) on");

		/* make it similar to normal CW, allowing interrupt */
		for (uint32_t i = 0; i < seconds; i++) {
			cw_queue_tone(CWDAEMON_USECS_PER_SEC, current_morse_tone);
		}

		cw_send_character('e');	/* append minimal tone to return to normal flow */
	}

	return;
}

/**
   \brief Reset some initial parameters of cwdaemon and libcw

   TODO: split this function into:
   cwdaemon_reset_basic_params()
   cwdaemon_reset_libcw_output()
   and call these two functions separately instead of this one.
   This function that combines these two doesn't make much sense.
*/
void cwdaemon_reset_almost_all(void)
{
	current_morse_speed  = default_morse_speed;
	current_morse_tone   = default_morse_tone;
	current_morse_volume = default_morse_volume;
	current_audio_system = default_audio_system;
	current_ptt_delay    = default_ptt_delay;
	current_weighting    = default_weighting;

	cwdaemon_reset_libcw_output();

	return;
}

/**
   \brief Open audio sink using libcw

   \param audio_system - audio system to be used by libcw

   \return -1 on failure
   \return 0 otherwise
*/
bool cwdaemon_open_libcw_output(int audio_system)
{
	int rv = cw_generator_new(audio_system, NULL);
	if (audio_system == CW_AUDIO_OSS && rv == CW_FAILURE) {
		/* When reopening libcw output, previous audio system may
		   block audio device for a short period of time after the
		   output has been closed. In such a situation OSS may fail
		   to open audio device. Let's give it some time. */
		for (int i = 0; i < 5; i++) {
			printf("delaying switching to OSS, please wait few seconds.\n");
			sleep(4);
			rv = cw_generator_new(audio_system, NULL);
			if (rv == CW_SUCCESS) {
				break;
			}
		}
	}
	if (rv != CW_FAILURE) {
		rv = cw_generator_start();
		printf("starting generator with sound system \"%s\": %s", cw_get_audio_system_label(audio_system), rv ? "success" : "failure");

	} else {
		/* FIXME:
		   When cwdaemon failed to create a generator, and user
		   kills non-forked cwdaemon through Ctrl+C, there is
		   a memory protection error.

		   It seems that this error has been fixed with
		   changes in libcw, committed on 31.12.2012.
		   To be observed. */
		printf( "failed to create generator with sound system \"%s\"", cw_get_audio_system_label(audio_system));
	}

	return rv == CW_FAILURE ? false : true;
}

/**
   \brief Close libcw audio output
*/
void cwdaemon_close_libcw_output(void) {
	cw_generator_stop();
	cw_generator_delete();
	return;
}

/**
   \brief Reset parameters of libcw to default values

   Function uses values of cwdaemon's global 'default_' variables, and some
   other values to reset state of libcw.
*/
void cwdaemon_reset_libcw_output(void)
{
	/* This function is called when cwdaemon receives '0' escape code.
	   README describes this code as "Reset to default values".
	   Therefore we use default_* below.

	   However, the function is called after "current_" values
	   have been reset to "default_" values. So maybe we could use
	   "current_" values and somehow encapsulate the calls to
	   cw_set_*() functions? The calls are also made elsewhere.
	*/

	/* Delete old generator (if it exists). */
	cwdaemon_close_libcw_output();

	printf("setting sound system \"%s\"", cw_get_audio_system_label(default_audio_system));

	if (cwdaemon_open_libcw_output(default_audio_system)) {
		has_audio_output = true;
	} else {
		has_audio_output = false;
		return;
	}

	/* Remember that tone queue is bound to a generator.  When
	   cwdaemon switches on request to other sound system, it will
	   have to re-register the callback. */
	cw_register_tone_queue_low_callback(cwdaemon_tone_queue_low_callback, NULL, tq_low_watermark);

	cw_set_frequency(default_morse_tone);
	cw_set_send_speed(default_morse_speed);
	cw_set_volume(default_morse_volume);
	cw_set_gap(0);
	cw_set_weighting(default_weighting * 0.6 + CWDAEMON_MORSE_WEIGHTING_MAX);

	return;
}

/**
   \brief Properly parse a 'long' integer

   Parse a string with digits, convert it to a long integer

   \param buf - input buffer with a string
   \param lvp - pointer to output long int variable

   \return false on failure
   \return true on success
*/
bool cwdaemon_get_long(const char *buf, long *lvp)
{
	errno = 0;

	char *ep;
	long lv = strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0') {
		return false;
	}

	if (errno == ERANGE && (lv == LONG_MAX || lv == LONG_MIN)) {
		return false;
	}
	*lvp = lv;

	return true;
}

/**
   \brief Prepare reply for the caller

   Fill \p reply buffer with data from given \p request, prepare some
   other variables for sending reply to the client.

   Text of the reply is usually defined by caller, i.e. it is sent by client
   to cwdaemon and marked by the client as text to be used in reply.

   The reply should be sent back to client as soon as cwdaemon
   finishes processing/playing received request.

   There are two different procedures for recognizing what should be sent
   back as reply and when:
   \li received request ending with '^' character: the text of the request
       should be played, but it also should be used as a reply.
       This function does not specify when the reply should be sent back.
       All it does is it prepares the text of reply.

       '^' can be used for char-by-char communication: client software
       message with single character followed by '^'. cwdaemon plays the
       character, and informs client software about playing the sound. Then
       client software can send request with next character followed by '^'.
   \li received request starting with "<ESC>h" escape code: the text of
       request should be sent back to the client after playing text of *next*
       request. So there are two requests sent by client to cwdaemon:
       first defines reply, and the second defines text to be played.
       First should be echoed back (but not played), second should be played.

   \param reply - buffer for reply (allocated by caller)
   \param request - buffer with request
   \param n - size of data in request
*/
void cwdaemon_prepare_reply(char *reply, const char *request, size_t n)
{
	/* Since we need to prepare a reply, we need to mark our
	   intent to send echo. The echo (reply) will be sent to client
	   when libcw's tone queue becomes empty.

	   It is important to set this flag at the beginning of the function. */
	ptt_flag |= PTT_ACTIVE_ECHO;
	printf("PTT flag +PTT_ACTIVE_ECHO (0x%02x/%s)", ptt_flag, cwdaemon_debug_ptt_flags());

	/* We are sending reply to the same host that sent a request. */
	memcpy(&radio->reply_addr, &radio->request_addr, sizeof(radio->reply_addr));
	radio->reply_addrlen = radio->request_addrlen;

	strncpy(reply, request, n);
	reply[n] = '\0'; /* FIXME: where is boundary checking? */

	printf("text of request: \"%s\", text of reply: \"%s\"\n", request, reply);
	printf("now waiting for end of transmission before echoing back to client\n");

	return;
}

/**
   \brief Wrapper around sendto()

   Wrapper around sendto(), sending a \p reply to client.
   Client is specified by reply_* network variables.

   \param reply - reply to be sent

   \return -1 on failure
   \return number of characters sent on success
*/
ssize_t cwdaemon_sendto(const char *reply)
{
	size_t len = strlen(reply);
	/* TODO: Do we *really* need to end replies with CRLF? */
	assert(reply[len - 2] == '\r' && reply[len - 1] == '\n');

	ssize_t rv = sendto(radio->socket_descriptor, reply, len, 0,
			    (struct sockaddr *) &radio->reply_addr, radio->reply_addrlen);

	if (rv == -1) {
		printf("sendto: \"%s\"", strerror(errno));
		return -1;
	} else {
		return rv;
	}
}

/**
   \brief Receive request sent through socket

   Received request is returned through \p request.
   Possible trailing '\r' and '\n' characters are stripped.
   Request is ended with '\0'.

   \param request - buffer for received request
   \param n - size of the buffer

   \return -2 if peer has performed an orderly shutdown
   \return -1 if an error occurred during call to recvfrom
   \return  0 if no request has been received
   \return length of received request otherwise
 */
int cwdaemon_recvfrom(char *request, int n)
{
	ssize_t recv_rc = recvfrom(radio->socket_descriptor,
				   request,
				   n,
				   0, /* flags */
				   (struct sockaddr *) &radio->request_addr,
				   /* TODO: request_addrlen may be modified. Check it. */
				   &radio->request_addrlen);

	if (recv_rc == -1) { /* No requests available? */

		if (errno == EAGAIN || errno == EWOULDBLOCK) { /* "a portable application should check for both possibilities" */
			/* Yup, no requests available from non-blocking socket. Good luck next time. */
			/* TODO: how much CPU time does it cost to loop waiting for a request?
			   Would it be reasonable to configure the socket as blocking?
			   How large is receive timeout? */

			return 0;
		} else {
			/* Some other error. May be a serious error. */
			g_print("Close thread\n");
			return -1;
		}
	} else if (recv_rc == 0) {
		/* "peer has performed an orderly shutdown" */
		return -2;
	} else {
		; /* pass */
	}

	/* Remove CRLF if present. TCP buffer may end with '\n', so make
	   sure that every request is consistently ended with NUL only.
	   Do it early, do it now. */
	int z;
	while (recv_rc > 0
	       && ( (z = request[recv_rc - 1]) == '\n' || z == '\r') ) {

		request[--recv_rc] = '\0';
	}

	return recv_rc;
}

/**
   \brief Receive message from socket, act upon it

   Watch the socket and if there is an escape character check what it is,
   otherwise play morse.

   FIXME: duplicate return value (zero and zero).

   \return 0 when an escape code has been received
   \return 0 when no request or an empty request has been received
   \return 1 when a text request has been played
*/
int cwdaemon_receive(void)
{
	/* The request may be a printable string, so +1 for ending NUL
	   added somewhere below is necessary. */
	char request_buffer[CWDAEMON_MESSAGE_SIZE_MAX + 1];

	ssize_t recv_rc = cwdaemon_recvfrom(request_buffer, CWDAEMON_MESSAGE_SIZE_MAX);

	if (recv_rc == -2) {
		/* Sender has closed connection. */
		return -1;
	} else if (recv_rc == -1) {
		/* TODO: should we really exit?
		   Shouldn't we recover from the error? */
		return -1;
	} else if (recv_rc == 0) {
		//printf("...recv_from (no data)\n");
		return 0;
	} else {
		; /* pass */
	}

	request_buffer[recv_rc] = '\0';

	printf("-------------------\n");
	/* TODO: replace the magic number 27 with constant. */
	if (request_buffer[0] != 27) {
		/* No ESCAPE. All received data should be treated
		   as text to be sent using Morse code.

		   Note that this does not exclude possibility of
		   caret request (e.g. "some text^"), which does
		   require sending a reply to client. Such request is
		   correctly handled by cwdaemon_play_request(). */
		printf("request: \"%s\"\n", request_buffer);
		if ((strlen(request_buffer) + strlen(request_queue)) <= CWDAEMON_REQUEST_QUEUE_SIZE_MAX - 1) {
			strcat(request_queue, request_buffer);
			cwdaemon_play_request(request_queue);
		} else {
			; /* TODO: how to handle this case? */
		}
		return 1;
	} else {
		/* Don't print literal escape value, use <ESC>
		   symbol. First reason is that the literal value
		   doesn't look good in console (some non-printable
		   glyph), second reason is that printing <ESC>c to
		   terminal makes funny things with the lines already
		   printed to the terminal (tested in xfce4-terminal
		   and xterm). */
		printf("escaped request: \"<ESC>%s\"\n", request_buffer + 1);
		cwdaemon_handle_escaped_request(request_buffer);
		return 0;
	}
}

/**
   The function may call exit() if a request from client asks the
   daemon to exit.
*/
void cwdaemon_handle_escaped_request(char *request)
{
	/* Take action depending on Escape code. */
	switch ((int) request[1]) {
	case '0':
		/* Reset all values. */
    /*
		printf("requested resetting of parameters");
		request_queue[0] = '\0';
		cwdaemon_reset_almost_all();
		wordmode = 0;
		async_abort = 0;
		//global_cwdevice->reset(global_cwdevice);

		ptt_flag = 0;
		printf("PTT flag = 0 (0x%02x/%s)", ptt_flag, cwdaemon_debug_ptt_flags());
		printf("resetting completed");
    */
		break;
	case '2':
		/* Set speed of Morse code, in words per minute. */
		if (cwdaemon_params_wpm(&current_morse_speed, request + 2)) {
			cw_set_send_speed(current_morse_speed);
		}
		break;
	case '3':
    break;
		/* Set tone (frequency) of morse code, in Hz.
		   The code assumes that minimal valid frequency is zero. */
		assert (CW_FREQUENCY_MIN == 0);
		if (cwdaemon_params_tone(&current_morse_tone, request + 2)) {
			if (current_morse_tone > 0) {

				cw_set_frequency(current_morse_tone);
				printf("tone: %d Hz", current_morse_tone);

				/* TODO: Should we really be adjusting
				   volume when the command is for
				   frequency? It would be more
				   "elegant" not to do so. */
				cw_set_volume(current_morse_volume);

			} else { /* current_morse_tone==0, sidetone off */
				cw_set_volume(0);
				printf("volume off");
			}
		}
		break;
	case '4':
		/* Abort currently sent message. */
		if (wordmode) {
			printf("requested aborting of message - ignoring (word mode is active)");
		} else {
			printf("requested aborting of message - executing (character mode is active)");
			if (ptt_flag & PTT_ACTIVE_ECHO) {
				printf("echo \"break\"");
				cwdaemon_sendto("break\r\n");
			}
			request_queue[0] = '\0';
			cw_flush_tone_queue();
			cw_wait_for_tone_queue();
			if (ptt_flag) {
				cwdaemon_set_ptt_off(global_cwdevice, "PTT off");
			}
			ptt_flag &= 0;
			printf("PTT flag = 0 (0x%02x/%s)", ptt_flag, cwdaemon_debug_ptt_flags());
		}
		break;
	case '5':
		/* Exit cwdaemon. */
		errno = 0;
		printf("requested exit of daemon");
		exit(EXIT_SUCCESS);

	case '6':
		/* Set uninterruptable (word mode). */
    break;
		request[0] = '\0';
		request_queue[0] = '\0';
		wordmode = 1;
		printf("wordmode set");
		break;
	case '7':
    break;
		/* Set weighting of morse code dits and dashes.
		   Remember that cwdaemon uses values in range
		   -50/+50, but libcw accepts values in range
		   20/80. This is why you have the calculation
		   when calling cw_set_weighting(). */
		if (cwdaemon_params_weighting(&current_weighting, request + 2)) {
			cw_set_weighting(current_weighting * 0.6 + CWDAEMON_MORSE_WEIGHTING_MAX);
		}
		break;
	case '8': {
		/* Set type of keying device. */
    // UNUSED
		break;
	}
	case '9':
		/* Change network port number.
		   TODO: why this is obsolete? */
		break;
	case 'a':
		/* PTT keying on or off */
		cwdaemon_params_ptt_on_off(request + 2);

		break;
	case 'b':
		/* SSB way. */
		break;
	case 'c':
		{
			/* FIXME: change this uint32_t to size_t. */
			uint32_t seconds = 0;
			/* Tune for a number of seconds. */
			if (cwdaemon_params_tune(&seconds, request + 2)) {
				cwdaemon_tune(seconds);
			}
			break;
		}
	case 'd':
		{
      break;
			/* Set PTT delay (TOD, Turn On Delay).
			   The value is milliseconds. */

			int rv = cwdaemon_params_pttdelay(&current_ptt_delay, request + 2);

			if (rv == 0) {
				/* Value totally invalid. */
				printf(
					       "invalid requested PTT delay [ms]: \"%s\" (should be integer between %d and %d inclusive)",
					       request + 2,
					       CWDAEMON_PTT_DELAY_MIN, CWDAEMON_PTT_DELAY_MAX);
			} else if (rv == 1) {
				/* Value totally valid. Information
				   debug string has been already
				   printed in
				   cwdaemon_params_pttdelay(). */
			} else { /* rv == 2 */
				/* Value invalid (out-of-range), but
				   acceptable when sent over network
				   request and then clipped to be
				   in-range. Value has been clipped in
				   cwdaemon_params_pttdelay(), but a
				   warning debug string must be
				   printed here. */
				printf("requested PTT delay [ms] out of range: \"%s\", clipping to \"%d\" (should be between %d and %d inclusive)",
					       request + 2,
					       CWDAEMON_PTT_DELAY_MAX,
					       CWDAEMON_PTT_DELAY_MIN, CWDAEMON_PTT_DELAY_MAX);
			}

			if (rv && current_ptt_delay == 0) {
				cwdaemon_set_ptt_off(global_cwdevice, "ensure PTT off");
			}
		}

		break;
	case 'e':
		/* Set band switch output on parport bits 9 (MSB), 8, 7, 2 (LSB). */
		break;
	case 'f': {
      break;
		/* Change sound system used by libcw. */
		/* FIXME: if "request+2" describes unavailable sound system,
		   cwdaemon fails to open the new sound system. Since
		   the old one is closed with cwdaemon_close_libcw_output(),
		   cwdaemon has no working sound system, and is unable to
		   play sound.

		   This can be fixed either by querying libcw if "request+2"
		   sound system is available, or by first trying to
		   open new sound system and then - on success -
		   closing the old one. In either case cwdaemon would
		   require some method to inform client about success
		   or failure to open new sound system.	*/
		if (cwdaemon_params_system(&current_audio_system, request + 2)) {
			/* Handle valid request for changing sound system. */
			cwdaemon_close_libcw_output();

			if (cwdaemon_open_libcw_output(current_audio_system)) {
				has_audio_output = true;
			} else {
				/* Fall back to NULL audio system. */
				cwdaemon_close_libcw_output();
				if (cwdaemon_open_libcw_output(CW_AUDIO_NULL)) {
					printf("fall back to \"Null\" sound system");
					current_audio_system = CW_AUDIO_PA;
					has_audio_output = true;
				} else {
					printf(
						       "failed to fall back to \"Null\" sound system");
					has_audio_output = false;
				}
			}

			if (has_audio_output) {

				/* Tone queue is bound to a
				   generator. Creating new generator
				   requires re-registering the
				   callback. */
				cw_register_tone_queue_low_callback(cwdaemon_tone_queue_low_callback, NULL, tq_low_watermark);

				/* This call recalibrates length of
				   dot and dash. */
				cw_set_frequency(current_morse_tone);

				cw_set_send_speed(current_morse_speed);
				cw_set_volume(current_morse_volume);

				/* Regardless if we are using
				   "default" or "current" parameters,
				   the gap is always zero. */
				cw_set_gap(0);

				cw_set_weighting(current_weighting * 0.6 + CWDAEMON_MORSE_WEIGHTING_MAX);
			}
		}
		break;
	}
	case 'g':
		/* Set volume of sound, in percents. */
		if (cwdaemon_params_volume(&current_morse_volume, request + 2)) {
			cw_set_volume(current_morse_volume);
		}
		break;
	case 'h':
		/* Data after '<ESC>h' is a text to be used as reply.
		   It shouldn't be echoed back to client immediately.

		   Instead, cwdaemon should wait for another request
		   (I assume that it will be a regular text to be
		   played), play it, and then send prepared reply back
		   to the client.  So this is a reply with delay. */

		/* 'request + 1' skips the leading <ESC>, but
		   preserves 'h'.  The 'h' is a part of reply text. If
		   the client didn't specify reply text, the 'h' will
		   be the only content of server's reply. */

		cwdaemon_prepare_reply(reply_buffer, request + 1, strlen(request + 1));
		printf("reply is ready, waiting for message from client (reply: \"%s\")", reply_buffer);
		/* cwdaemon will wait for queue-empty callback before
		   sending the reply. */
		break;
	} /* switch ((int) request[1]) */

	return;
}

/**
   \brief Process received request, play relevant characters

   Check every character in given request, act upon markers
   for speed increase or decrease, and play other characters.

   Function modifies contents of buffer \p request.

   \param request - request to be processed
*/
void cwdaemon_play_request(char *request)
{
	cw_block_callback(true);

	char *x = request;

	while (*x) {
		switch ((int) *x) {
		case '+':
		case '-':
			/* Speed increase & decrease */
			/* Repeated '+' and '-' characters are allowed,
			   in such cases increase and decrease of speed is
			   multiple of 2 wpm. */
			do {
				current_morse_speed += (*x == '+') ? 2 : -2;
				x++;
			} while (*x == '+' || *x == '-');

			if (current_morse_speed < CW_SPEED_MIN) {
				current_morse_speed = CW_SPEED_MIN;
			} else if (current_morse_speed > CW_SPEED_MAX) {
				current_morse_speed = CW_SPEED_MAX;
			} else {
				;
			}
			cw_set_send_speed(current_morse_speed);
			break;
		case '~':
			/* 2 dots time additional for the next char. The gap
			   is always reset after playing the char. */
			cw_set_gap(2);
			x++;
			break;
		case '^':
			/* Send echo to main program when CW playing is done. */
			*x = '\0';     /* Remove '^' and possible trailing garbage. */
			/* '^' can be found at the end of request, and
			   it means "echo text of current request back
			   to client once you finish playing it". */
			cwdaemon_prepare_reply(reply_buffer, request, strlen(request));

			/* cwdaemon will wait for queue-empty callback
			   before sending the reply. */
			break;
		case '*':
			/* TODO: what's this? */
			*x = '+';
		default:
			cwdaemon_set_ptt_on(global_cwdevice, "PTT (auto) on");
			/* PTT is now in AUTO. It will be turned off on low
			   tone queue, in cwdaemon_tone_queue_low_callback(). */

			printf("Morse character \"%c\" to be queued in libcw\n", *x);
			cw_send_character(*x);
			//printf("Morse character \"%c\" has been queued in libcw", *x);

			x++;
			if (cw_get_gap() == 2) {
				if (*x == '^') {
					/* '^' is supposed to be the
					   last character in the
					   message, meaning that all
					   that was before it should
					   be used as reply text. So
					   x++ will jump to ending
					   NUL. */
					x++;
				} else {
					cw_set_gap(0);
				}
			}
			break;
		}
	}

	/* All characters processed, mark buffer as empty. */
	*request = '\0';

	cw_block_callback(false);

	return;
}

/**
   \brief Callback function for key state change

   Function passed to libcw, will be called every time a state of libcw's
   internal ("software") key changes, i.e. every time it starts or ends
   producing dit or dash.
   When the software key is closed (dit or dash), \p keystate is 1.
   Otherwise \p keystate is 0. Following the changes of \p keystate the
   function changes state of bit on output of its keying device.

   So it goes like this:
   input text in cwdaemon is split into characters ->
   characters in cwdaemon are sent to libcw ->
   characters in libcw are converted to dits/dashes ->
   dits/dashes are played by libcw, and at the same time libcw changes
           state of its software key ->
   libcw calls cwdaemon_keyingevent() on changes of software key ->
   cwdaemon_keyingevent() changes state of a bit on cwdaemon's keying device

   \param arg - unused argument
   \param keystate - state of a key, as seen by libcw
*/
void cwdaemon_keyingevent(__attribute__((unused)) void *arg, int keystate)
{
  g_mutex_lock(&cwdaemon_mutex);  
	if (keystate == 1) {
    keytx = true;
	} else {
    keytx = false;
	}
  g_mutex_unlock(&cwdaemon_mutex);   
	inactivity_seconds = 0;

	printf("keying event \"%d\"\n", keystate);

	return;
}


/**
   \brief Callback routine called when tone queue is empty

   Callback routine registered with cw_register_tone_queue_low_callback(),
   will be called by libcw every time number of tones drops in queue below
   specific level.

   \param arg - unused argument
*/
void cwdaemon_tone_queue_low_callback(__attribute__((unused)) void *arg)
{
  return;
	int len = cw_get_tone_queue_length();
	printf("low TQ callback: start, TQ len = %d, PTT flag = 0x%02x/%s",
		       len, ptt_flag, cwdaemon_debug_ptt_flags());

	if (len > tq_low_watermark) {
		printf("low TQ callback: TQ len larger than watermark, TQ len = %d", len);
	}

	if (ptt_flag == PTT_ACTIVE_AUTO
	    /* PTT is (most probably?) on, in purely automatic mode.
	       This means that as soon as there are no new chars to
	       play, we should turn PTT off. */

	    && request_queue[0] == '\0'
	    /* No new text has been queued in the meantime. */

	    && cw_get_tone_queue_length() <= tq_low_watermark) {
		/* TODO: check if this third condition is really necessary. */
		/* Originally it was 'cw_get_tone_queue_length() <= 1',
		   I'm guessing that '1' here was the same '1' as the
		   third argument to cw_register_tone_queue_low_callback().
		   Feel free to correct me ;) */


		printf("low TQ callback: branch 1, PTT flag = 0x%02x/%s", ptt_flag, cwdaemon_debug_ptt_flags());

		cwdaemon_set_ptt_off(global_cwdevice, "PTT (auto) off");

	} else if (ptt_flag & PTT_ACTIVE_ECHO) {
		/* PTT_ACTIVE_ECHO: client has used special request to
		   indicate that it is waiting for reply (echo) from
		   the server (i.e. cwdaemon) after the server plays
		   all characters. */

		printf("low TQ callback: branch 2, PTT flag = 0x%02x/%s", ptt_flag, cwdaemon_debug_ptt_flags());

		/* Since echo is being sent, we can turn the flag off.
		   For some reason cwdaemon works better when we turn the
		   flag off before sending the reply, rather than turning
		   if after sending the reply. */
		ptt_flag &= ~PTT_ACTIVE_ECHO;
		printf("low TQ callback: PTT flag -PTT_ACTIVE_ECHO, PTT flag = 0x%02x/%s", ptt_flag, cwdaemon_debug_ptt_flags());


		printf("low TQ callback: echoing \"%s\" back to client             <----------", reply_buffer);
		strcat(reply_buffer, "\r\n"); /* Ensure exactly one CRLF */
		cwdaemon_sendto(reply_buffer);
		/* If this line is uncommented, the callback erases a valid
		   reply that should be sent back to client. Commenting the
		   line fixes the problem, and doesn't seem to introduce
		   any new ones.
		   TODO: investigate the original problem of erasing a valid
		   reply. */
		/* reply_buffer[0] = '\0'; */


		/* wait a bit more since we expect to get more text to send

		   TODO: the comment above is a bit unclear.  Perhaps
		   it means that we have dealt with escape request
		   requesting an echo, and now there may be a second
		   request (following the escape request) that still
		   needs to be played ("more text to send").

		   If so, we need to call the callback again, so that
		   it can check if text buffer is non-empty and if
		   tone queue is non-empty. We call the callback again
		   indirectly, by simulating almost empty tone queue
		   (i.e. by adding only two tones to tone queue).

		   I wonder why we don't call the callback directly,
		   maybe it has something to do with avoiding
		   recursion? */
		if (ptt_flag == PTT_ACTIVE_AUTO) {
			printf("low TQ callback: queueing two empty tones");
			cw_queue_tone(1, 0); /* ensure Q-empty condition again */
			cw_queue_tone(1, 0); /* when trailing gap also 'sent' */
		}
	} else {
		/* TODO: how to correctly handle this case?
		   Should we do something? */
		printf("low TQ callback: branch 3, PTT flag = 0x%02x/%s", ptt_flag, cwdaemon_debug_ptt_flags());
	}

	printf("low TQ callback: end, TQ len = %d, PTT flag = 0x%02x/%s",
		       cw_get_tone_queue_length(), ptt_flag, cwdaemon_debug_ptt_flags());

	return;

}




bool cwdaemon_params_wpm(int *wpm, const char *optarg)
{
	long lv = 0;
	if (!cwdaemon_get_long(optarg, &lv) || lv < CW_SPEED_MIN || lv > CW_SPEED_MAX) {
		printf("invalid requested morse speed [wpm]: \"%s\" (should be integer between %d and %d inclusive)",
			       optarg, CW_SPEED_MIN, CW_SPEED_MAX);
		return false;
	} else {
		*wpm = (int) lv;
		printf(
			       "requested morse speed [wpm]: \"%d\"", *wpm);
		return true;
	}
}


bool cwdaemon_params_tune(uint32_t *seconds, const char *optarg)
{
	long lv = 0;

	/* TODO: replace cwdaemon_get_long() with cwdaemon_get_uint32() */
	if (!cwdaemon_get_long(optarg, &lv) || lv < 0 || lv > CWDAEMON_TUNE_SECONDS_MAX) {
		printf("invalid requested tuning time [s]: \"%s\" (should be integer between %d and %d inclusive)",
			       optarg, 0, CWDAEMON_TUNE_SECONDS_MAX);
		return false;
	} else {
		printf(
			       "requested tuning time [s]: \"%ld\"", lv);

		*seconds = (uint32_t) lv;

		return true;
	}
}


/**
   \brief Handle parameter specifying PTT Turn On Delay

   This function, and handling its return values by callers, isn't as
   straightforward as it could be. This is because:
   \li of some backwards-compatibility reasons,
   \li because expected behaviour when handling command line argument,
   and when handling network request, are a bit different,
   \li because negative value of \p optarg is handled differently than
   too large value of \p optarg.

   The function clearly rejects negative value passed in \p
   optarg. Return value is then 0 (a "false" value in boolean
   expressions).

   It is more tolerant when it comes to non-negative values, returning
   1 or 2 (a "true" value in boolean expressions).

   When the non-negative value is out of range (larger than limit
   accepted by cwdaemon), the value is clipped to the limit, and put
   into \p delay. Return value is then 2. Caller of the function may
   then decide to accept or reject the value.

   When the non-negative value is in range, the value is put into \p
   delay, and return value is 1. Caller of the function must accept
   the value.

   Value passed in \p optarg is copied to \p delay only when function
   returns 1 or 2.

   \return 1 if value of \optarg is acceptable when it was provided as request and as command line argument (i.e. non-negative value in range);
   \return 2 if value of \optarg is acceptable when it was provided as request, but not acceptable when it was provided as command line argument (i.e. non-negative value out of range);
   \return 0 if value of \optarg is not acceptable, regardless how it was provided (i.e. a negative value or invalid value);
*/
int cwdaemon_params_pttdelay(int *delay, const char *optarg)
{
	long lv = 0;
	if (!cwdaemon_get_long(optarg, &lv)) {
		printf(
			       "invalid requested PTT delay [ms]: \"%s\" (should be integer between %d and %d inclusive)",
			       optarg,
			       CWDAEMON_PTT_DELAY_MIN, CWDAEMON_PTT_DELAY_MAX);

		/* 0 means "Value not acceptable in any context." */
		return 0;
	}

	if (lv > CWDAEMON_PTT_DELAY_MAX) {

		/* In theory we should reject invalid value, but for
		   some reason in some contexts we aren't very strict
		   about it. So be it. Just don't allow the value to
		   be larger than *_MAX limit. */
		*delay = CWDAEMON_PTT_DELAY_MAX;

		/* 2 means "Value in general invalid (non-negative,
		   but out of range), but in some contexts we may be
		   tolerant and accept it after it has been decreased
		   to an in-range value (*_MAX). */
		return 2;


	} else if (lv < CWDAEMON_PTT_DELAY_MIN) {

		/* In first branch of the if() we have accepted too
		   large value from misinformed client, but we can't
		   accept values that are clearly invalid (negative). */

		/* 0 means "Value is not acceptable in any context". */
		return 0;

	} else { /* Non-negative, in range. */
		*delay = (int) lv;

		//printf("requested PTT delay [ms]: \"%ld\"", *delay);

		/* 1 means "Value valid in all contexts." */
		return 1;
	}
}


bool cwdaemon_params_volume(int *volume, const char *optarg)
{
	long lv = 0;
	if (!cwdaemon_get_long(optarg, &lv) || lv < CW_VOLUME_MIN || lv > CW_VOLUME_MAX) {
		printf(
			       "invalid requested volume [%%]: \"%s\" (should be integer between %d and %d inclusive)",
			       optarg, CW_VOLUME_MIN, CW_VOLUME_MAX);
		return false;
	} else {
		*volume = (int) lv;
		printf(
			       "requested volume [%%]: \"%d\"", *volume);
		return true;
	}
}

bool cwdaemon_params_weighting(int *weighting, const char *optarg)
{
	long lv = 0;
	if (!cwdaemon_get_long(optarg, &lv) || lv < CWDAEMON_MORSE_WEIGHTING_MIN || lv > CWDAEMON_MORSE_WEIGHTING_MAX) {
		printf(
			       "invalid requested weighting: \"%s\" (should be integer between %d and %d inclusive)",
			       optarg, CWDAEMON_MORSE_WEIGHTING_MIN, CWDAEMON_MORSE_WEIGHTING_MAX);
		return false;
	} else {
		*weighting = (int) lv;
		//printf("requested weighting: \"%ld\"", *weighting);
		return true;
	}
}

bool cwdaemon_params_tone(int *tone, const char *optarg)
{
	long lv = 0;
	if (!cwdaemon_get_long(optarg, &lv) || lv < CW_FREQUENCY_MIN || lv > CW_FREQUENCY_MAX) {
		printf(
			       "invalid requested tone [Hz]: \"%s\" (should be integer between %d and %d inclusive)",
			       optarg, CW_FREQUENCY_MIN, CW_FREQUENCY_MAX);
		return false;
	} else {
		*tone = (int) lv;
		//printf("requested tone [Hz]: \"%ld\"", *tone);
		return true;
	}
}


bool cwdaemon_params_set_verbosity(int *verbosity, const char *optarg)
{
	if (!strcmp(optarg, "n") || !strcmp(optarg, "N")) {
		*verbosity = CWDAEMON_VERBOSITY_N;
	} else if (!strcmp(optarg, "e") || !strcmp(optarg, "E")) {
		*verbosity = CWDAEMON_VERBOSITY_E;
	} else if (!strcmp(optarg, "w") || !strcmp(optarg, "W")) {
		*verbosity = CWDAEMON_VERBOSITY_W;
	} else if (!strcmp(optarg, "i") || !strcmp(optarg, "I")) {
		*verbosity = CWDAEMON_VERBOSITY_I;
	} else if (!strcmp(optarg, "d") || !strcmp(optarg, "D")) {
		*verbosity = CWDAEMON_VERBOSITY_D;
	} else {
		printf("invalid requested verbosity level: \"%s\"", optarg);
		return false;
	}

	printf("requested verbosity level threshold: \"%s\"", cwdaemon_verbosity_labels[*verbosity]);
	return true;
}


bool cwdaemon_params_system(int *system, const char *optarg)
{
	if (!strncmp(optarg, "n", 1)) {
		*system = CW_AUDIO_NULL;
	} else if (!strncmp(optarg, "c", 1)) {
		*system = CW_AUDIO_CONSOLE;
	} else if (!strncmp(optarg, "s", 1)) {
		*system = CW_AUDIO_SOUNDCARD;
	} else if (!strncmp(optarg, "a", 1)) {
		*system = CW_AUDIO_ALSA;
	} else if (!strncmp(optarg, "p", 1)) {
		*system = CW_AUDIO_PA;
	} else if (!strncmp(optarg, "o", 1)) {
		*system = CW_AUDIO_OSS;
	} else {
		/* TODO: print only those audio systems that are
		   supported on given machine. */
		printf("Invalid requested sound system\n");
		return false;
	}

	printf("Requested sound system: \"%s\" (\"%s\")\n", optarg, cw_get_audio_system_label(*system));
	return true;
}


bool cwdaemon_params_ptt_on_off(const char *optarg)
{
	long lv = 0;

	/* PTT keying on or off */
	if (!cwdaemon_get_long(optarg, &lv)) {
		printf("invalid requested PTT state: \"%s\" (should be numeric value \"0\" or \"1\")", optarg);
		return false;
	} else {
		printf("requested PTT state: \"%s\"", optarg);
	}

	if (lv) {
		//global_cwdevice->ptt(global_cwdevice, ON);
		if (current_ptt_delay) {
			cwdaemon_set_ptt_on(global_cwdevice, "PTT (manual, delay) on");
		} else {
			printf("PTT (manual, immediate) on\n");
		}

		ptt_flag |= PTT_ACTIVE_MANUAL;
    
	} else if (ptt_flag & PTT_ACTIVE_MANUAL) {	/* only if manually activated */

		ptt_flag &= ~PTT_ACTIVE_MANUAL;

		if (!(ptt_flag & !PTT_ACTIVE_AUTO)) {	/* no PTT modifiers */

			if (request_queue[0] == '\0'/* no new text in the meantime */
			    && cw_get_tone_queue_length() <= 1) {

				cwdaemon_set_ptt_off(global_cwdevice, "PTT (manual, immediate) off");
			} else {
				/* still sending, cannot yet switch PTT off */
				ptt_flag |= PTT_ACTIVE_AUTO;	/* ensure auto-PTT active */
			}
		}
	}

	return true;
}

/* main program: fork, open network connection and go into an endless loop
   waiting for something to happen on the UDP port */
extern gpointer cwdaemon_thread(gpointer data)
{
  printf("Starting cwdaemon\n");
  keytx = false;
	if (!cwdaemon_initialize_socket()) {
    g_print("Failed to initialise socket\n");
    radio->cwdaemon_running = FALSE;
    g_thread_exit(NULL);    
		//exit(EXIT_FAILURE);
	}
  

	// Initialize libcw 
	cwdaemon_reset_almost_all();
  
  // Read the settings set from radio_dialog.c
  RADIO *radio=(RADIO *)data;

  printf("\n");
	current_morse_speed  = radio->cw_keyer_speed;
	current_morse_tone   = radio->cw_keyer_sidetone_frequency;
	current_morse_volume = (int)((((float)(radio->cw_keyer_sidetone_volume)/300)) * 100);
	current_weighting    = (radio->cw_keyer_weight)-50;  
  
  printf("Speed %d\n", current_morse_speed);
  printf("Tone %d\n", current_morse_tone);  
  printf("Vol %d\n", current_morse_volume);  
  
  
	cw_set_frequency(current_morse_tone);
	cw_set_send_speed(current_morse_speed);
	cw_set_volume(current_morse_volume);
	cw_set_gap(0);
	cw_set_weighting(current_weighting * 0.6 + CWDAEMON_MORSE_WEIGHTING_MAX);
  
	cw_register_keying_callback(cwdaemon_keyingevent, NULL);


	/* The main loop of cwdaemon. */
	request_queue[0] = '\0';
	
  radio->cwdaemon_running = TRUE;  
  while (radio->cwdaemon_running) {   
    
		fd_set readfd;
		struct timeval udptime;
		FD_ZERO(&readfd);
		FD_SET(radio->socket_descriptor, &readfd);
    
		if (inactivity_seconds < 10) {
			udptime.tv_sec = 1;
			inactivity_seconds++;
		} else {
			udptime.tv_sec = 5;
		}

		udptime.tv_usec = 0;
		/* udptime.tv_usec = 999000; */	/* 1s is more than enough */
		int fd_count = select(radio->socket_descriptor + 1, &readfd, NULL, NULL, &udptime);
		//int fd_count = select(radio->socket_descriptor + 1, &readfd, NULL, NULL, NULL); 
		if (fd_count == -1 && errno != EINTR) {
			printf("Select");
		} else {
			int rv = cwdaemon_receive();
      if (rv == -1) {
        radio->cwdaemon_running = FALSE;         
      }
		}

  } 
  g_print("Close cwdaemon thread\n");
  cwdaemon_close_libcw_output();
  cwdaemon_close_socket();
  
  g_thread_exit(NULL);
  return NULL;
  //return;
}


/**
   \brief Initialize network variables

   Initialize network socket and other network variables.

   \return false on failure
   \return true on success
*/
bool cwdaemon_initialize_socket(void)
{
	memset(&radio->request_addr, '\0', sizeof (radio->request_addr));
	radio->request_addr.sin_family = AF_INET;
	radio->request_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	radio->request_addr.sin_port = htons(radio->cwd_port);
	radio->request_addrlen = sizeof (radio->request_addr);

	radio->socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (radio->socket_descriptor == -1) {
		printf("Socket open");
		return NULL;
	}
  int on = 1;
  setsockopt(radio->socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(radio->socket_descriptor,
		 (struct sockaddr *) &radio->request_addr,
		 radio->request_addrlen) == -1) {

		printf("Bind");
		return false;
	}

	int save_flags = fcntl(radio->socket_descriptor, F_GETFL);
	if (save_flags == -1) {
		printf("Trying get flags");
		return false;
	}
	save_flags |= O_NONBLOCK;

	if (fcntl(radio->socket_descriptor, F_SETFL, save_flags) == -1) {
		printf("Trying non-blocking");
		return false;
	}

	return true;
}

void cwdaemon_stop(void) {
  
  g_mutex_lock(&cwdaemon_mutex); 
  radio->cwdaemon_running = FALSE;
  g_mutex_unlock(&cwdaemon_mutex);   
  
  g_print("Stop cwdaemon\n");
  g_print("cwd run %d\n", radio->cwdaemon_running);
  //cwdaemon_close_libcw_output();
  //cwdaemon_close_socket();
  g_print("cwd run %d\n", radio->cwdaemon_running);  
}

void cwdaemon_close_socket(void)
{
  g_print("Close cwdaemon socket\n");
	if (radio->socket_descriptor) {
		if (close(radio->socket_descriptor) == -1) {
			printf("Close socket\n");
			//exit(EXIT_FAILURE);
		}
    radio->socket_descriptor = -1;
	}
	return;
}
