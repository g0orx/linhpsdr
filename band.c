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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "agc.h"
#include "mode.h"
#include "filter.h"
#include "bandstack.h"
#include "band.h"
#include "discovered.h"
#include "bpsk.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "alex.h"
#include "property.h"


int band=band20;
int xvtr_band=BANDS;


/* --------------------------------------------------------------------------*/
/**
* @brief bandstack
*/
/* ----------------------------------------------------------------------------*/
BANDSTACK_ENTRY bandstack_entries2200[] =
    {{135800,CWL,F6,-2800,-200,-2800,-200},
     {137100,CWL,F6,-2800,-200,-2800,-200}};

BANDSTACK_ENTRY bandstack_entries630[] =
    {{472100LL,CWL,F6,-2800,-200,-2800,-200},
     {475100LL,CWL,F6,-2800,-200,-2800,-200}};

BANDSTACK_ENTRY bandstack_entries160[] =
    {{1810000LL,CWL,F6,-2800,-200,-2800,-200},
     {1835000LL,CWU,F6,-2800,-200,-2800,-200},
     {1845000LL,USB,F5,-2800,-200,-2800,-200}};

BANDSTACK_ENTRY bandstack_entries80[] =
    {{3501000LL,CWL,F6,-2800,-200,-2800,-200},
     {3751000LL,LSB,F5,-2800,-200,-2800,-200},
     {3850000LL,LSB,F5,-2800,-200,-2800,-200}};

BANDSTACK_ENTRY bandstack_entries60_OTHER[] =
    {{5332000LL,USB,F5,-2800,-200,-2800,-200},
     {5348000LL,USB,F5,-2800,-200,-2800,-200},
     {5358500LL,USB,F5,-2800,-200,-2800,-200},
     {5373000LL,USB,F5,-2800,-200,-2800,-200},
     {5405000LL,USB,F5,-2800,-200,-2800,-200}};

BANDSTACK_ENTRY bandstack_entries60_UK[] =
    {{5261250LL,USB,F5,-2800,-200,-2800,-200},
     {5280000LL,USB,F5,-2800,-200,-2800,-200},
     {5290250LL,USB,F5,-2800,-200,-2800,-200},
     {5302500LL,USB,F5,-2800,-200,-2800,-200},
     {5318000LL,USB,F5,-2800,-200,-2800,-200},
     {5335500LL,USB,F5,-2800,-200,-2800,-200},
     {5356000LL,USB,F5,-2800,-200,-2800,-200},
     {5368250LL,USB,F5,-2800,-200,-2800,-200},
     {5380000LL,USB,F5,-2800,-200,-2800,-200},
     {5398250LL,USB,F5,-2800,-200,-2800,-200},
     {5405000LL,USB,F5,-2800,-200,-2800,-200}};

BANDSTACK_ENTRY bandstack_entries40[] =
    {{7001000LL,CWL,F6,-2800,-200,-2800,-200},
     {7152000LL,LSB,F5,-2800,-200,-2800,-200},
     {7255000LL,LSB,F5,-2800,-200,-2800,-200}};

BANDSTACK_ENTRY bandstack_entries30[] =
    {{10120000LL,CWU,F6,200,2800,200,2800},
     {10130000LL,CWU,F6,200,2800,200,2800},
     {10140000LL,CWU,F6,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries20[] =
    {{14010000LL,CWU,F6,200,2800,200,2800},
     {14150000LL,USB,F5,200,2800,200,2800},
     {14230000LL,USB,F5,200,2800,200,2800},
     {14336000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries17[] =
    {{18068600LL,CWU,F6,200,2800,200,2800},
     {18125000LL,USB,F5,200,2800,200,2800},
     {18140000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries15[] =
    {{21001000LL,CWU,F6,200,2800,200,2800},
     {21255000LL,USB,F5,200,2800,200,2800},
     {21300000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries12[] =
    {{24895000LL,CWU,F6,200,2800,200,2800},
     {24900000LL,USB,F5,200,2800,200,2800},
     {24910000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries10[] =
    {{28010000LL,CWU,F6,200,2800,200,2800},
     {28300000LL,USB,F5,200,2800,200,2800},
     {28400000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries6[] =
    {{50010000LL,CWU,F6,200,2800,200,2800},
     {50125000LL,USB,F5,200,2800,200,2800},
     {50200000LL,USB,F5,200,2800,200,2800}};

#ifdef SOAPYSDR
BANDSTACK_ENTRY bandstack_entries70[] =
    {{70010000LL,CWU,F6,200,2800,200,2800},
     {70200000LL,USB,F5,200,2800,200,2800},
     {70250000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries144[] =
    {{144010000LL,CWU,F6,200,2800,200,2800},
     {144200000LL,USB,F5,200,2800,200,2800},
     {144250000LL,USB,F5,200,2800,200,2800},
     {145600000LL,FMN,F1,200,2800,200,2800},
     {145725000LL,FMN,F1,200,2800,200,2800},
     {145900000LL,FMN,F1,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries220[] =
    {{220010000LL,CWU,F6,200,2800,200,2800},
     {220200000LL,USB,F5,200,2800,200,2800},
     {220250000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries430[] =
    {{430010000LL,CWU,F6,200,2800,200,2800},
     {432100000LL,USB,F5,200,2800,200,2800},
     {432300000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries902[] =
    {{902010000LL,CWU,F6,200,2800,200,2800},
     {902100000LL,USB,F5,200,2800,200,2800},
     {902300000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries1240[] =
    {{1240010000LL,CWU,F6,200,2800,200,2800},
     {1240100000LL,USB,F5,200,2800,200,2800},
     {1240300000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries2300[] =
    {{2300010000LL,CWU,F6,200,2800,200,2800},
     {2300100000LL,USB,F5,200,2800,200,2800},
     {2300300000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entries3400[] =
    {{3400010000LL,CWU,F6,200,2800,200,2800},
     {3400100000LL,USB,F5,200,2800,200,2800},
     {3400300000LL,USB,F5,200,2800,200,2800}};

BANDSTACK_ENTRY bandstack_entriesAIR[] =
    {{118800000LL,AM,F3,200,2800,200,2800},
     {120000000LL,AM,F3,200,2800,200,2800},
     {121700000LL,AM,F3,200,2800,200,2800},
     {124100000LL,AM,F3,200,2800,200,2800},
     {126600000LL,AM,F3,200,2800,200,2800},
     {136500000LL,AM,F3,200,2800,200,2800}};
#endif

BANDSTACK_ENTRY bandstack_entriesGEN[] =
    {{909000LL,AM,F3,-6000,6000,-6000,60000},
     {5975000LL,AM,F3,-6000,6000,-6000,60000},
     {13845000LL,AM,F3,-6000,6000,-6000,60000}};

BANDSTACK_ENTRY bandstack_entriesWWV[] =
    {{2500000LL,SAM,F3,200,2800,200,2800},
     {5000000LL,SAM,F3,200,2800,200,2800},
     {10000000LL,SAM,F3,200,2800,200,2800},
     {15000000LL,SAM,F3,200,2800,200,2800},
     {20000000LL,SAM,F3,200,2800,200,2800}};

BANDSTACK bandstack2200={2,0,bandstack_entries2200};
BANDSTACK bandstack630={2,0,bandstack_entries630};
BANDSTACK bandstack160={3,1,bandstack_entries160};
BANDSTACK bandstack80={3,1,bandstack_entries80};
BANDSTACK bandstack60={5,1,bandstack_entries60_OTHER};
BANDSTACK bandstack40={3,1,bandstack_entries40};
BANDSTACK bandstack30={3,1,bandstack_entries30};
BANDSTACK bandstack20={4,1,bandstack_entries20};
BANDSTACK bandstack17={3,1,bandstack_entries17};
BANDSTACK bandstack15={3,1,bandstack_entries15};
BANDSTACK bandstack12={3,1,bandstack_entries12};
BANDSTACK bandstack10={3,1,bandstack_entries10};
BANDSTACK bandstack6={3,1,bandstack_entries6};
#ifdef SOAPYSDR
BANDSTACK bandstack70={3,1,bandstack_entries70};
BANDSTACK bandstack144={6,1,bandstack_entries144};
BANDSTACK bandstack220={3,1,bandstack_entries220};
BANDSTACK bandstack430={3,1,bandstack_entries430};
BANDSTACK bandstack902={3,1,bandstack_entries902};
BANDSTACK bandstack1240={3,1,bandstack_entries1240};
BANDSTACK bandstack2300={3,1,bandstack_entries2300};
BANDSTACK bandstack3400={3,1,bandstack_entries3400};
BANDSTACK bandstackAIR={6,1,bandstack_entriesAIR};
#endif
BANDSTACK bandstackWWV={5,1,bandstack_entriesWWV};
BANDSTACK bandstackGEN={3,1,bandstack_entriesGEN};
/* --------------------------------------------------------------------------*/
/**
* @brief xvtr
*/
/* ----------------------------------------------------------------------------*/

BANDSTACK_ENTRY bandstack_entries_xvtr_0[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};
BANDSTACK_ENTRY bandstack_entries_xvtr_1[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};
BANDSTACK_ENTRY bandstack_entries_xvtr_2[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};
BANDSTACK_ENTRY bandstack_entries_xvtr_3[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};
BANDSTACK_ENTRY bandstack_entries_xvtr_4[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};
BANDSTACK_ENTRY bandstack_entries_xvtr_5[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};
BANDSTACK_ENTRY bandstack_entries_xvtr_6[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};
BANDSTACK_ENTRY bandstack_entries_xvtr_7[] =
    {{0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550},
     {0LL,USB,F6,150,2550,150,2550}};

BANDSTACK bandstack_xvtr_0={3,0,bandstack_entries_xvtr_0};
BANDSTACK bandstack_xvtr_1={3,0,bandstack_entries_xvtr_1};
BANDSTACK bandstack_xvtr_2={3,0,bandstack_entries_xvtr_2};
BANDSTACK bandstack_xvtr_3={3,0,bandstack_entries_xvtr_3};
BANDSTACK bandstack_xvtr_4={3,0,bandstack_entries_xvtr_4};
BANDSTACK bandstack_xvtr_5={3,0,bandstack_entries_xvtr_5};
BANDSTACK bandstack_xvtr_6={3,0,bandstack_entries_xvtr_6};
BANDSTACK bandstack_xvtr_7={3,0,bandstack_entries_xvtr_7};



BAND bands[BANDS+XVTRS] = 
    {{"2200",&bandstack2200,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,135700LL,137800LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"630",&bandstack630,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,472000LL,479000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"160",&bandstack160,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,1800000LL,2000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"80",&bandstack80,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,3500000LL,4000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"60",&bandstack60,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,5330500LL,5403500LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"40",&bandstack40,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,7000000LL,7300000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"30",&bandstack30,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,10100000LL,10150000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"20",&bandstack20,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,14000000LL,14350000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"17",&bandstack17,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,18068000LL,18168000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"15",&bandstack15,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,21000000LL,21450000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"12",&bandstack12,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,24890000LL,24990000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"10",&bandstack10,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,28000000LL,29700000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"6",&bandstack6,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,50000000LL,54000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
#ifdef SOAPYSDR
     {"70",&bandstack70,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"144",&bandstack144,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,144000000LL,148000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"220",&bandstack144,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,222000000LL,224980000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"430",&bandstack430,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,420000000LL,450000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"902",&bandstack430,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,902000000LL,928000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"1240",&bandstack1240,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,1240000000LL,1300000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"2300",&bandstack2300,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,2300000000LL,2450000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"3400",&bandstack3400,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,3400000000LL,3410000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"AIR",&bandstackAIR,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,10800000LL,137000000LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
#endif
     {"GEN",&bandstackGEN,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"WWV",&bandstackWWV,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
// XVTRS
     {"",&bandstack_xvtr_0,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"",&bandstack_xvtr_1,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"",&bandstack_xvtr_2,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"",&bandstack_xvtr_3,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"",&bandstack_xvtr_4,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"",&bandstack_xvtr_5,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"",&bandstack_xvtr_6,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1},
     {"",&bandstack_xvtr_7,0,0,0,0,0,ALEX_ATTENUATION_0dB,53.0,0LL,0LL,0LL,0LL,0,-140,-60,20,-145,-65,1}
    };

CHANNEL band_channels_60m_UK[UK_CHANNEL_ENTRIES] =
     {{5261250LL,5500LL},
      {5280000LL,8000LL},
      {5290250LL,3500LL},
      {5302500LL,9000LL},
      {5318000LL,10000LL},
      {5335500LL,5000LL},
      {5356000LL,4000LL},
      {5368250LL,12500LL},
      {5380000LL,4000LL},
      {5398250LL,6500LL},
      {5405000LL,3000LL}};

CHANNEL band_channels_60m_OTHER[OTHER_CHANNEL_ENTRIES] =
     {{5332000LL,2800LL},
      {5348000LL,2800LL},
      {5358500LL,2800LL},
      {5373000LL,2800LL},
      {5405000LL,2800LL}};

int channel_entries;
CHANNEL *band_channels_60m;

BANDSTACK *bandstack_get_bandstack(int band) {
    return bands[band].bandstack;
}

BANDSTACK_ENTRY *bandstack_get_bandstack_entry(int band,int entry) {
    BANDSTACK *bandstack=bands[band].bandstack;
    return &bandstack->entry[entry];
}

BANDSTACK_ENTRY *bandstack_entry_get_current() {
    BANDSTACK *bandstack=bands[band].bandstack;
    BANDSTACK_ENTRY *entry=&bandstack->entry[bandstack->current_entry];
    return entry;
}

BANDSTACK_ENTRY *bandstack_entry_next() {
    BANDSTACK *bandstack=bands[band].bandstack;
    bandstack->current_entry++;
    if(bandstack->current_entry>=bandstack->entries) {
        bandstack->current_entry=0;
    }
    BANDSTACK_ENTRY *entry=&bandstack->entry[bandstack->current_entry];
    return entry;
}

BANDSTACK_ENTRY *bandstack_entry_previous() {
    BANDSTACK *bandstack=bands[band].bandstack;
    bandstack->current_entry--;
    if(bandstack->current_entry<0) {
        bandstack->current_entry=bandstack->entries-1;
    }
    BANDSTACK_ENTRY *entry=&bandstack->entry[bandstack->current_entry];
    return entry;
}


int band_get_current() {
    return band;
}

BAND *band_get_current_band() {
    BAND *b=&bands[band];
    return b;
}

BAND *band_get_band(int b) {
    BAND *band=&bands[b];
    return band;
}

BAND *band_set_current(int b) {
    band=b;
    return &bands[b];
}

void bandSaveState() {
    char name[128];
    char value[128];
    BANDSTACK_ENTRY* entry;

    int b;
    int stack;
    for(b=0;b<BANDS+XVTRS;b++) {
      if(strlen(bands[b].title)>0) {
        sprintf(name,"band.%d.title",b);
        setProperty(name,bands[b].title);

        sprintf(value,"%d",bands[b].bandstack->entries);
        sprintf(name,"band.%d.entries",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].bandstack->current_entry);
        sprintf(name,"band.%d.current",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].preamp);
        sprintf(name,"band.%d.preamp",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].alexRxAntenna);
        sprintf(name,"band.%d.alexRxAntenna",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].alexTxAntenna);
        sprintf(name,"band.%d.alexTxAntenna",b);
        sprintf(name,"band.%d.alexTxAntenna",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].alexAttenuation);
        sprintf(name,"band.%d.alexAttenuation",b);
        setProperty(name,value);

        sprintf(value,"%f",bands[b].pa_calibration);
        sprintf(name,"band.%d.pa_calibration",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].OCrx);
        sprintf(name,"band.%d.OCrx",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].OCtx);
        sprintf(name,"band.%d.OCtx",b);
        setProperty(name,value);

        sprintf(value,"%lld",bands[b].frequencyMin);
        sprintf(name,"band.%d.frequencyMin",b);
        setProperty(name,value);

        sprintf(value,"%lld",bands[b].frequencyMax);
        sprintf(name,"band.%d.frequencyMax",b);
        setProperty(name,value);

        sprintf(value,"%lld",bands[b].frequencyLO);
        sprintf(name,"band.%d.frequencyLO",b);
        setProperty(name,value);

        sprintf(value,"%lld",bands[b].errorLO);
        sprintf(name,"band.%d.errorLO",b);
        setProperty(name,value);

        sprintf(value,"%d",bands[b].disablePA);
        sprintf(name,"band.%d.disablePA",b);
        setProperty(name,value);

        for(stack=0;stack<bands[b].bandstack->entries;stack++) {
            entry=bands[b].bandstack->entry;
            entry+=stack;

            sprintf(value,"%lld",entry->frequency);
            sprintf(name,"band.%d.stack.%d.a",b,stack);
            setProperty(name,value);

            sprintf(value,"%d",entry->mode);
            sprintf(name,"band.%d.stack.%d.mode",b,stack);
            setProperty(name,value);

            sprintf(value,"%d",entry->filter);
            sprintf(name,"band.%d.stack.%d.filter",b,stack);
            setProperty(name,value);

            sprintf(value,"%d",entry->var1Low);
            sprintf(name,"band.%d.stack.%d.var1Low",b,stack);
            setProperty(name,value);

            sprintf(value,"%d",entry->var1High);
            sprintf(name,"band.%d.stack.%d.var1High",b,stack);
            setProperty(name,value);

            sprintf(value,"%d",entry->var2Low);
            sprintf(name,"band.%d.stack.%d.var2Low",b,stack);
            setProperty(name,value);

            sprintf(value,"%d",entry->var2High);
            sprintf(name,"band.%d.stack.%d.var2High",b,stack);
            setProperty(name,value);

        }
      }
    }

    sprintf(value,"%d",band);
    setProperty("band",value);
}

void bandRestoreState() {
    char* value;
    int b;
    char name[128];

    for(b=0;b<BANDS+XVTRS;b++) {
        sprintf(name,"band.%d.title",b);
        value=getProperty(name);
        if(value) strcpy(bands[b].title,value);

        sprintf(name,"band.%d.entries",b);
        value=getProperty(name);
        if(value) bands[b].bandstack->entries=atoi(value);

        sprintf(name,"band.%d.current",b);
        value=getProperty(name);
        if(value) bands[b].bandstack->current_entry=atoi(value);

        sprintf(name,"band.%d.preamp",b);
        value=getProperty(name);
        if(value) bands[b].preamp=atoi(value);

        sprintf(name,"band.%d.alexRxAntenna",b);
        value=getProperty(name);
        if(value) bands[b].alexRxAntenna=atoi(value);

        sprintf(name,"band.%d.alexTxAntenna",b);
        value=getProperty(name);
        if(value) bands[b].alexTxAntenna=atoi(value);

// fix bug so props file does not have to be deleted
        if(bands[b].alexTxAntenna>2) bands[b].alexTxAntenna=0;

        sprintf(name,"band.%d.alexAttenuation",b);
        value=getProperty(name);
        if(value) bands[b].alexAttenuation=atoi(value);

        sprintf(name,"band.%d.pa_calibration",b);
        value=getProperty(name);
        if(value) {
          bands[b].pa_calibration=strtod(value,NULL);
          if(bands[b].pa_calibration<38.8 || bands[b].pa_calibration>100.0) {
            bands[b].pa_calibration=38.8;
          }
        }

        sprintf(name,"band.%d.OCrx",b);
        value=getProperty(name);
        if(value) bands[b].OCrx=atoi(value);

        sprintf(name,"band.%d.OCtx",b);
        value=getProperty(name);
        if(value) bands[b].OCtx=atoi(value);

        sprintf(name,"band.%d.frequencyMin",b);
        value=getProperty(name);
        if(value) bands[b].frequencyMin=atoll(value);

        sprintf(name,"band.%d.frequencyMax",b);
        value=getProperty(name);
        if(value) bands[b].frequencyMax=atoll(value);

        sprintf(name,"band.%d.frequencyLO",b);
        value=getProperty(name);
        if(value) bands[b].frequencyLO=atoll(value);

        sprintf(name,"band.%d.errorLO",b);
        value=getProperty(name);
        if(value) bands[b].errorLO=atoll(value);

        sprintf(name,"band.%d.disablePA",b);
        value=getProperty(name);
        if(value) bands[b].disablePA=atoi(value);
    }

    value=getProperty("band");
    if(value) band=atoi(value);
}

int get_band_from_frequency(gint64 f) {
  int b;
  int found=-1;

  for(b=0;b<BANDS+XVTRS;b++) {
    BAND *band=band_get_band(b);
    if(strlen(band->title)>0) {
      if(f>=band->frequencyMin && f<=band->frequencyMax) {
        found=b;
        break;
      }
    }
  }
  if (found < 0) found=bandGen;
  return found;  
}

int next_band(int current_band) {
  int b=current_band+1;
  if(b>=BANDS) {
    b=bandWWV;
  } else {
    BAND *band=&bands[b];
    if(band->frequencyMin>radio->discovered->frequency_max) {
      b=bandGen;
    }
  }
  return b;
}

int previous_band(int current_band) {
  int b=current_band-1;
  if(b<0) {
    b=0;
  } else {
    BAND *band=&bands[b];
    while(band->frequencyMin>radio->discovered->frequency_max && b>0) {
      b--;
      band=&bands[b];
    }
  }
  return b;
}

void set_band(RECEIVER *rx,int band,int bs_entry) {
  // save current bandstack 
  BAND *b=&bands[rx->band_a];
  BANDSTACK *stack=b->bandstack;
  BANDSTACK_ENTRY *entry=&stack->entry[stack->current_entry];

  if(band!=rx->band_a || (band==rx->band_a && bs_entry!=stack->current_entry)) {
    entry->frequency=rx->frequency_a;
    entry->mode=rx->mode_a;
    entry->filter=rx->filter_a;
    b->frequencyLO=rx->lo_a;
    b->errorLO=rx->error_a;
  }

  // get the new bandstack
  if(band!=rx->band_a) {
    b->panadapter_low=rx->panadapter_low;
    b->panadapter_high=rx->panadapter_high;
    b->panadapter_step=rx->panadapter_step;
    b->waterfall_low=rx->waterfall_low;
    b->waterfall_high=rx->waterfall_high;
    b->waterfall_automatic=rx->waterfall_automatic;
    b=&bands[band];
    stack=b->bandstack;
  } else {
    stack->current_entry++;
    if(stack->current_entry>=stack->entries) stack->current_entry=0;
  }
  if(bs_entry!=-1) {
    stack->current_entry=bs_entry;
  }
  entry=&stack->entry[stack->current_entry];
  rx->frequency_a=entry->frequency;
  rx->mode_a=entry->mode;
  rx->filter_a=entry->filter;
  rx->lo_a=b->frequencyLO;
  rx->error_a=b->errorLO;
  rx->ctun=FALSE;
  rx->ctun_offset=0;

  rx->panadapter_low=b->panadapter_low;
  rx->panadapter_high=b->panadapter_high;
  rx->panadapter_step=b->panadapter_step;
  rx->waterfall_low=b->waterfall_low;
  rx->waterfall_high=b->waterfall_high;
  rx->waterfall_automatic=b->waterfall_automatic;

  receiver_band_changed(rx,band);
  if(radio->transmitter) {
    if(radio->transmitter->rx==rx) {
      if(rx->split!=SPLIT_OFF) {
        transmitter_set_mode(radio->transmitter,rx->mode_b);
      } else {
        transmitter_set_mode(radio->transmitter,rx->mode_a);
      }
    }
  }
}
