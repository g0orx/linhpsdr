/*
 * Layer-2 of MIDI support
 *
 * (C) Christoph van Wullen, DL1YCF
 *
 * Using the data in MIDICommandsTable, this subroutine translates the low-level
 * MIDI events into MIDI actions in the SDR console.
 */

#include <gtk/gtk.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "receiver.h"
#include "discovered.h"
#include "adc.h"
#include "dac.h"
#include "transmitter.h"
#include "wideband.h"
#include "radio.h"
#include "main.h"
#include "midi.h"

extern int midi_rx;

struct cmdtable MidiCommandsTable;

void NewMidiEvent(enum MIDIevent event, int channel, int note, int val) {

    struct desc *desc;
    int new;
    static enum MIDIaction last_wheel_action=ACTION_NONE ;
    static struct timespec tp, last_wheel_tp={0,0};
    long delta;

//g_print("%s: EVENT=%d CHAN=%d NOTE=%d VAL=%d\n",__FUNCTION__,event,channel,note,val);
    if (event == MIDI_PITCH) {
	desc=MidiCommandsTable.pitch;
    } else {
	desc=MidiCommandsTable.desc[note];
    }
//g_print("%s: init DESC=%p\n",__FUNCTION__,desc);
    while (desc) {
//g_print("%s: DESC=%p next=%p CHAN=%d EVENT=%d\n",__FUNCTION__,desc,desc->next,desc->channel,desc->event);
	if ((desc->channel == channel || desc->channel == -1) && (desc->event == event)) {
	    // Found matching entry
	    switch (desc->event) {
		case MIDI_NOTE:
		    if ((val == 1 || (desc->onoff == 1)) && desc->type == MIDI_KEY) {
			DoTheMidi(desc->action, desc->type, val);
		    }
		    break;
		case MIDI_CTRL:
		    if (desc->type == MIDI_KNOB) {
			// normalize value to range 0 - 100
			new = (val*100)/127;
			DoTheMidi(desc->action, desc->type, new);
		    } else if (desc->type == MIDI_WHEEL) {
			if (desc->delay > 0 && last_wheel_action == desc->action) {
			  clock_gettime(CLOCK_MONOTONIC, &tp);
			  delta=1000*(tp.tv_sec - last_wheel_tp.tv_sec);
			  delta += (tp.tv_nsec - last_wheel_tp.tv_nsec)/1000000;
			  if (delta < desc->delay) break;
			  last_wheel_tp = tp;
			}
			// translate value to direction
			new=0;
			new=val-64;
			if ((val >= desc->vfl1) && (val <= desc->vfl2)) new=-100;
			if ((val >= desc-> fl1) && (val <= desc-> fl2)) new=-10;
			if ((val >= desc->lft1) && (val <= desc->lft2)) new=-1;
			if ((val >= desc->rgt1) && (val <= desc->rgt2)) new= 1;
			if ((val >= desc-> fr1) && (val <= desc-> fr2)) new= 10;
			if ((val >= desc->vfr1) && (val <= desc->vfr2)) new= 100;
//			fprintf(stderr,"WHEEL: val=%d new=%d thrs=%d/%d, %d/%d, %d/%d, %d/%d, %d/%d, %d/%d\n",
//                                  val, new, desc->vfl1, desc->vfl2, desc->fl1, desc->fl2, desc->lft1, desc->lft2,
//				          desc->rgt1, desc->rgt2, desc->fr1, desc->fr2, desc->vfr1, desc->vfr2);
			if (new != 0) DoTheMidi(desc->action, desc->type, new);
			last_wheel_action=desc->action;
		    }
		    break;
		case MIDI_PITCH:
		    if (desc->type == MIDI_KNOB) {
			// normalize value to 0 - 100
			new = (val*100)/16383;
			DoTheMidi(desc->action, desc->type, new);
		    }
		    break;
		case EVENT_NONE:
		    break;
	    }
	    break;
	} else {
	    desc=desc->next;
	}
    }
    if (!desc) {
      // Nothing found. This is nothing to worry about, but log the key to stderr
        if (event == MIDI_PITCH) g_print("%s: Unassigned PitchBend Value=%d\n",__FUNCTION__, val);
        if (event == MIDI_NOTE ) g_print("%s: Unassigned Key Note=%d Val=%d\n",__FUNCTION__, note, val);
        if (event == MIDI_CTRL ) g_print("%s: Unassigned Controller Ctl=%d Val=%d\n",__FUNCTION__, note, val);
    } else if(midi_debug) {
        if (event == MIDI_PITCH) g_print("%s: PitchBend Value=%d\n",__FUNCTION__, val);
        if (event == MIDI_NOTE ) g_print("%s: Key Note=%d Val=%d\n",__FUNCTION__, note, val);
        if (event == MIDI_CTRL ) g_print("%s: Controller Ctl=%d Val=%d\n",__FUNCTION__, note, val);
    }
}

gchar *midi_types[] = {"NONE","KEY","KNOB/SLIDER","*INVALID*","WHEEL"};
gchar *midi_events[] = {"NONE","NOTE","CTRL","PITCH"};

/*
 * This data structre connects names as used in the midi.props file with
 * our MIDIaction enum values.
 * Take care that no key word is contained in another one!
 * Example: use "CURRVFO" not "VFO" otherwise there is possibly
 * a match for "VFO" when the key word is "VFOA".
 */

/*
static struct {
  enum MIDIaction action;
  const char *str;
  enum MIDItype type;
} ActionTable[] = {
*/
ACTION_TABLE ActionTable[] = {
	{ ACTION_NONE,		"NONE",		TYPE_NONE},
	{ VFO_A2B,		"A2B",		MIDI_KEY},
        { MIDI_AF_GAIN,      	"AF GAIN",	MIDI_KNOB|MIDI_WHEEL},
	{ AGCATTACK,   		"AGC",	MIDI_KEY},
        { MIDI_AGC,     	"AGC GAIN",	MIDI_KNOB|MIDI_WHEEL},
        { ANF,     		"ANF",		MIDI_KEY},
        { ATT,          	"ATT",		MIDI_KNOB|MIDI_WHEEL},
	{ VFO_B2A,		"B2A",		MIDI_KEY},
        { MIDI_BAND_160,        "1.8",	MIDI_KEY},
        { MIDI_BAND_80,         "3.5",	MIDI_KEY},
        { MIDI_BAND_60,         "5",	MIDI_KEY},
        { MIDI_BAND_40,         "7",	MIDI_KEY},
        { MIDI_BAND_30,         "10",	MIDI_KEY},
        { MIDI_BAND_20,         "14",	MIDI_KEY},
        { MIDI_BAND_17,         "18",	MIDI_KEY},
        { MIDI_BAND_15,         "21",	MIDI_KEY},
        { MIDI_BAND_12,         "24",	MIDI_KEY},
	{ MIDI_BAND_10,         "28",	MIDI_KEY},
        { MIDI_BAND_6,          "50",	MIDI_KEY},
        { MIDI_BAND_70,         "70",	MIDI_KEY},
        { MIDI_BAND_144,        "144",	MIDI_KEY},
        { MIDI_BAND_220,        "220",	MIDI_KEY},
        { MIDI_BAND_430,        "430",	MIDI_KEY},
        { MIDI_BAND_902,        "902",	MIDI_KEY},
        { MIDI_BAND_1240,       "1240",	MIDI_KEY},
        { MIDI_BAND_2300,       "2300",	MIDI_KEY},
        { MIDI_BAND_3400,       "3400",	MIDI_KEY},
        { MIDI_BAND_AIR,        "AIR",	MIDI_KEY},
        { MIDI_BAND_GEN,        "GEN",	MIDI_KEY},
        { MIDI_BAND_WWV,        "WWV",	MIDI_KEY},
        { BAND_DOWN,            "BAND DOWN",	MIDI_KEY},
        { BAND_UP,              "BAND UP",	MIDI_KEY},
        { COMPRESS,     	"COMPRESS",	MIDI_KEY},
	{ MIDI_CTUN,  		"CTUN",		MIDI_KEY},
	{ VFO,			"CURR VFO",	MIDI_WHEEL},
	{ CWLEFT,		"CW LEFT",		MIDI_KEY},
	{ CWRIGHT,		"CW RIGHT",		MIDI_KEY},
	{ CWSPEED,		"CW SPEED",	MIDI_KNOB|MIDI_WHEEL},
	{ DIV_COARSEGAIN,	"DIV COARSE GAIN",	MIDI_KNOB|MIDI_WHEEL},
	{ DIV_COARSEPHASE,	"DIV COARSE PHASE",	MIDI_KNOB|MIDI_WHEEL},
	{ DIV_FINEGAIN,		"DIV FINE GAIN",	MIDI_KNOB|MIDI_WHEEL},
	{ DIV_FINEPHASE,	"DIV FINE PHASE",	MIDI_KNOB|MIDI_WHEEL},
	{ DIV_GAIN,		"DIV GAIN",	MIDI_KNOB|MIDI_WHEEL},
	{ DIV_PHASE,		"DIV PHASE",	MIDI_KNOB|MIDI_WHEEL},
	{ DIV_TOGGLE,		"DIV TOGGLE",	MIDI_KEY},
	{ MIDI_DUP,  		"DUP",		MIDI_KEY},
        { FILTER_DOWN,  	"FILTER DOWN",	MIDI_KEY},
        { FILTER_UP,    	"FILTER UP",	MIDI_KEY},
	{ MIDI_LOCK,    	"LOCK",		MIDI_KEY},
	{ MENU_ABOUT,           "MENU ABOUT",	MIDI_KEY},
        { MENU_EER,             "MENU EER",	MIDI_KEY},
        { MENU_MIDI,            "MENU MID",	MIDI_KEY},
        { MENU_OC,              "MENU OC",	MIDI_KEY},
        { MENU_PA,              "MENU PA",	MIDI_KEY},
        { MENU_PS,              "MENU  PS",	MIDI_KEY},
        { MENU_RADIO,           "MENU  RADIO",	MIDI_KEY},
        { MENU_RX,              "MENU RX",	MIDI_KEY},
        { MENU_TX,              "MENU TX",	MIDI_KEY},
        { MENU_XVTR,            "MENU XVTR",	MIDI_KEY},
        { MIC_VOLUME,   	"MIC GAIN",	MIDI_KNOB|MIDI_WHEEL},
	{ MODE_DOWN,		"MODE DOWN",	MIDI_KEY|MIDI_KNOB|MIDI_WHEEL},
	{ MODE_UP,		"MODE UP",	MIDI_KEY|MIDI_KNOB|MIDI_WHEEL},
        { MIDI_MOX,     	"MOX",	MIDI_KEY},
	{ MIDI_MUTE,		"MUTE",	MIDI_KEY},
	{ MIDI_NB,    		"NOISE BLANKER",	MIDI_KEY},
	{ MIDI_NR,    		"NOISE REDUCTION",	MIDI_KEY},
	{ NUMPAD_0,		"NUMPAD 0",	MIDI_KEY},
	{ NUMPAD_1,		"NUMPAD 1",	MIDI_KEY},
	{ NUMPAD_2,		"NUMPAD 2",	MIDI_KEY},
	{ NUMPAD_3,		"NUMPAD 3",	MIDI_KEY},
	{ NUMPAD_4,		"NUMPAD 4",	MIDI_KEY},
	{ NUMPAD_5,		"NUMPAD 5",	MIDI_KEY},
	{ NUMPAD_6,		"NUMPAD 6",	MIDI_KEY},
	{ NUMPAD_7,		"NUMPAD 7",	MIDI_KEY},
	{ NUMPAD_8,		"NUMPAD 8",	MIDI_KEY},
	{ NUMPAD_9,		"NUMPAD 9",	MIDI_KEY},
	{ NUMPAD_CL,		"NUMPAD CLEAR",	MIDI_KEY},
	{ NUMPAD_ENTER,		"NUMPAD ENTER",	MIDI_KEY},
        { MIDI_PAN,		"PAN",	MIDI_KNOB|MIDI_WHEEL},
        { PAN_HIGH,     	"PAN HIGH",	MIDI_KNOB|MIDI_WHEEL},
        { PAN_LOW,      	"PAN LOW",	MIDI_KNOB|MIDI_WHEEL},
        { PRE,          	"PREAMP",	MIDI_KEY},
        { MIDI_PTT,          	"PTT",		MIDI_KEY},
	{ MIDI_PS,    		"PURESIGNAL",	MIDI_KEY},
	{ MIDI_RF_GAIN, 	"RFGAIN",	MIDI_KNOB|MIDI_WHEEL},
        { TX_DRIVE,     	"DRIVE",	MIDI_KNOB|MIDI_WHEEL},
	{ MIDI_RIT_CLEAR,	"RIT CLEAR",	MIDI_KEY},
	{ RIT_STEP, 		"RIT STEP",	MIDI_KNOB|MIDI_WHEEL},
        { RIT_TOGGLE,   	"RIT TOGGLE",	MIDI_KEY},
        { RIT_VAL,      	"RIT VAL",	MIDI_KNOB|MIDI_WHEEL},
        { MIDI_SAT,     	"SAT",	MIDI_KEY},
        { SNB, 		    	"SNB",	MIDI_KEY},
	{ MIDI_SPLIT,  		"SPLIT",	MIDI_KEY},
	{ SWAP_RX,		"NEXT RX",	MIDI_KEY},
	{ SWAP_VFO,		"SWAP VFO",	MIDI_KEY},
        { MIDI_TUNE,    	"TUNE",	MIDI_KEY},
        { VFOA,         	"VFOA",	MIDI_WHEEL},
        { VFOB,         	"VFOB",	MIDI_WHEEL},
	{ VFO_STEP_UP,  	"VFO STEP UP",	MIDI_KEY},
	{ VFO_STEP_DOWN,	"VFO STEP DOWN",	MIDI_KEY},
	{ VOX,   		"VOX",	MIDI_KEY},
	{ VOXLEVEL,   		"VOX LEVEL",	MIDI_KNOB|MIDI_WHEEL},
	{ MIDI_XIT_CLEAR,  	"XIT CLEAR",	MIDI_KEY},
	{ XIT_VAL,  		"XIT VAL",	MIDI_KNOB|MIDI_WHEEL},
	{ MIDI_ZOOM,		"ZOOM",	MIDI_KNOB|MIDI_WHEEL},
	{ ZOOM_UP,		"ZOOM UP",	MIDI_KEY},
	{ ZOOM_DOWN,		"ZOOM DOWN",	MIDI_KEY},
        { ACTION_NONE,  	"NONE",	TYPE_NONE}
};

/*
 * Translation from keyword in midi.props file to MIDIaction
 */

static enum MIDIaction keyword2action(char *s) {
    int i=0;

    for (i=0; i< (sizeof(ActionTable) / sizeof(ActionTable[0])); i++) {
	if (!strcmp(s, ActionTable[i].str)) return ActionTable[i].action;
    }
    fprintf(stderr,"MIDI: action keyword %s NOT FOUND.\n", s);
    return ACTION_NONE;
}


int MIDIstop(RADIO *r) {
  r->midi_enabled=FALSE;
  close_midi_device();
  return 0;
}

/*
 * Here we read in a MIDI description file and fill the MidiCommandsTable
 * data structure
 */

int MIDIstartup(char *filename) {
    FILE *fpin;
    char zeile[255];
    char *cp,*cq;
    int key;
    enum MIDIaction action;
    int chan;
    int t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
    int onoff, delay;
    struct desc *desc,*dp;
    enum MIDItype type;
    enum MIDIevent event;
    int i;
    char c;

    for (i=0; i<128; i++) MidiCommandsTable.desc[i]=NULL;
    MidiCommandsTable.pitch=NULL;

    g_print("%s: %s\n",__FUNCTION__,filename);
    fpin=fopen(filename, "r");

    g_print("%s: fpin=%p\n",__FUNCTION__,fpin);
    if (!fpin) {
      g_print("%s: failed to open MIDI device\n",__FUNCTION__);
      return -1;
    }

    for (;;) {
      if (fgets(zeile, 255, fpin) == NULL) break;

      // ignore comments
      cp=index(zeile,'#');
      if (cp == zeile) continue;   // comment line
      if (cp) *cp=0;               // ignore trailing comment

      // change newline, comma, slash etc. to blanks
      cp=zeile;
      while ((c=*cp)) {
        switch (c) {
	  case '\n':
	  case '\r':
	  case '\t':
	  case ',':
	  case '/':
	    *cp=' ';
	    break;
        }
	cp++;
      }
      
g_print("\n%s:INP:%s\n",__FUNCTION__,zeile);

      if ((cp = strstr(zeile, "DEVICE="))) {
        // Delete comments and trailing blanks
	cq=cp+7;
	while (*cq != 0 && *cq != '#') cq++;
	*cq--=0;
	while (cq > cp+7 && (*cq == ' ' || *cq == '\t')) cq--;
	*(cq+1)=0;
//fprintf(stderr,"MIDI:REG:>>>%s<<<\n",cp+7);
	int result=register_midi_device(cp+7);
        continue; // nothing more in this line
      }
      chan=-1;  // default: any channel
      t1=t3=t5=t7= t9=t11=128;  // range that never occurs
      t2=t4=t6=t8=t10=t12=-1;   // range that never occurs
      onoff=0;
      event=EVENT_NONE;
      type=TYPE_NONE;
      key=0;
      delay=0;

      //
      // The KEY=, CTRL=, and PITCH= cases are mutually exclusive
      // If more than one keyword is in the line, PITCH wins over CTRL
      // wins over KEY.
      //
      if ((cp = strstr(zeile, "KEY="))) {
        sscanf(cp+4, "%d", &key);
        event=MIDI_NOTE;
	type=MIDI_KEY;
g_print("%s: MIDI:KEY:%d\n",__FUNCTION__, key);
      }
      if ((cp = strstr(zeile, "CTRL="))) {
        sscanf(cp+5, "%d", &key);
	event=MIDI_CTRL;
	type=MIDI_KNOB;
g_print("%s: MIDI:CTL:%d\n",__FUNCTION__, key);
      }
      if ((cp = strstr(zeile, "PITCH "))) {
        event=MIDI_PITCH;
	type=MIDI_KNOB;
g_print("%s: MIDI:PITCH\n",__FUNCTION__);
      }
      //
      // If event is still undefined, skip line
      //
      if (event == EVENT_NONE) {
//fprintf(stderr,"MIDI:ERR:NO_EVENT\n");
	continue;
      }

      //
      // beware of illegal key values
      //
      if (key < 0  ) key=0;
      if (key > 127) key=127;

      if ((cp = strstr(zeile, "CHAN="))) {
        sscanf(cp+5, "%d", &chan);
	chan--;
        if (chan<0 || chan>15) chan=-1;
g_print("%s:CHAN:%d\n",__FUNCTION__,chan);
      }
      if ((cp = strstr(zeile, "WHEEL")) && (type == MIDI_KNOB)) {
	// change type from MIDI_KNOB to MIDI_WHEEL
        type=MIDI_WHEEL;
g_print("%s:WHEEL\n",__FUNCTION__);
      }
      if ((cp = strstr(zeile, "ONOFF"))) {
        onoff=1;
g_print("%s:ONOFF\n",__FUNCTION__);
      }
      if ((cp = strstr(zeile, "DELAY="))) {
        sscanf(cp+6, "%d", &delay);
g_print("%s:DELAY:%d\n",__FUNCTION__,delay);
      }
      if ((cp = strstr(zeile, "THR="))) {
        sscanf(cp+4, "%d %d %d %d %d %d %d %d %d %d %d %d",
               &t1,&t2,&t3,&t4,&t5,&t6,&t7,&t8,&t9,&t10,&t11,&t12);
//fprintf(stderr,"MIDI:THR:%d/%d, %d/%d, %d/%d, %d/%d, %d/%d, %d/%d\n",t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12);
      }
      if ((cp = strstr(zeile, "ACTION="))) {
        // cut zeile at the first blank character following
        cq=cp+7;
        while (*cq != 0 && *cq != '\n' && *cq != ' ' && *cq != '\t') cq++;
	*cq=0;
        action=keyword2action(cp+7);
g_print("%s: MIDI:ACTION:%s (%d)\n",__FUNCTION__,cp+7, action);
      }
      //
      // All data for a descriptor has been read. Construct it!
      //
      desc = (struct desc *) malloc(sizeof(struct desc));
      desc->next = NULL;
      desc->action = action;
      desc->type = type;
      desc->event = event;
      desc->onoff = onoff;
      desc->delay = delay;
      desc->vfl1  = t1;
      desc->vfl2  = t2;
      desc->fl1   = t3;
      desc->fl2   = t4;
      desc->lft1  = t5;
      desc->lft2  = t6;
      desc->rgt1  = t7;
      desc->rgt2  = t8;
      desc->fr1   = t9;
      desc->fr2   = t10;
      desc->vfr1  = t11;
      desc->vfr2  = t12;
      desc->channel  = chan;
      //
      // insert descriptor into linked list.
      // We have a linked list for each key value to speed up searches
      //
      if (event == MIDI_PITCH) {
//fprintf(stderr,"MIDI:TAB:Insert desc=%p in PITCH table\n",desc);
	dp = MidiCommandsTable.pitch;
	if (dp == NULL) {
	  MidiCommandsTable.pitch = desc;
	} else {
	  while (dp->next != NULL) dp=dp->next;
	  dp->next=desc;
	}
      }
      if (event == MIDI_KEY || event == MIDI_CTRL) {
g_print("%s:TAB:Insert desc=%p in CMDS[%d] table\n",__FUNCTION__,desc,key);
	dp = MidiCommandsTable.desc[key];
	if (dp == NULL) {
	  MidiCommandsTable.desc[key]=desc;
	} else {
	  while (dp->next != NULL) dp=dp->next;
	  dp->next=desc;
	}
      }
    }

    midi_rx=0;
    return 0;
}
