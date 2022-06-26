/* Copyright (C)
* 2020 - m5evt
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
#ifndef _HL2_H
#define _HL2_H

#include <gtk/gtk.h>

#include "peak_detect.h"
#include "ringbuffer.h"


#define ADDR_I2C2 0x3d
#define ADDR_I2C1 0x3c
#define ADDR_0X17 0x17

#define I2C1_WRITE 0x06
#define I2C1_READ  0x07

#define I2C2_WRITE 0x0E
#define I2C2_READ  0x0F

// HL2-MRF101 bias pot
#define ADDR_MCP4561 0x2e
// HL2 pa bias (x2 digi pots in same ic), also used for EEPROM
#define ADDR_MCP4662 0x2c
// HL2-MRF101 ADC for pa current and temperature
#define ADDR_MRF101 0x2A
// Versaclock 5, main HL2 clock generator/distribution
#define ADDR_VERSA5 0xD4


#define MCP4662_BIAS0 0x02
#define MCP4662_BIAS1 0x03

#define DUMMY_VALUE 0x00
//#define MCP_WRITE_EEPROM

#define HL2_I2C_QUEUESIZE 15

#define HL2_SYNC_MASK_PRIMARY 0x7D
#define HL2_SYNC_MASK_SECONDARY 0x7E

#define CURRENT_PEAK_BUF_SIZE 40

#define MAX_GAIN 31 

typedef struct _hermeslite2 {
  gint mrf101_bias_value;
  gboolean mrf101_bias_enable;
  gulong bias_signal_id;

  gint64 clock2_freq;
  gboolean xvtr;

  gboolean adc2_value_to_send;
  gint adc2_lna_gain;
  
  glong value_to_send;
  
  gint addr_waiting_for;
  gint command_waiting_for;
  
  gboolean queue_busy;
  
  gint hl2_tx_buffer_size;

  gint late_packets;
  gint ep6_error_ctr;

  gboolean overflow;
  gboolean underflow;

  gdouble mrf101_temp;
  gdouble mrf101_current;
  gdouble current_peak;
  PEAKDETECTOR *current_peak_buf;

  //fpga generated clocks - removes noise on 160m
  gboolean psu_clk;

  gint lna_gain_tx;
  
  gint qos_timer_id;
  gint mrf101data_timer_id;  

  RINGBUFFERL *one_shot_queue;

  gboolean cl2_enabled;
  gboolean cl2_integer_mode;

  GMutex i2c_mutex;  
} HERMESLITE2;

extern HERMESLITE2 *create_hl2(void);

extern void HL2i2cQueueWrite(HERMESLITE2 *hl2, int readwrite, unsigned int addr, unsigned int command, unsigned int value);
extern int HL2i2cWriteQueued(HERMESLITE2 *hl2);
extern int hl2_get_txbuffersize(HERMESLITE2 *hl2);

extern void HL2mrf101SetBias(HERMESLITE2 *hl2);
extern void HL2mrf101StoreBias(HERMESLITE2 *hl2);
extern void HL2mrf101ReadBias(HERMESLITE2 *hl2);

extern int HL2i2cSendRqst(HERMESLITE2 *hl2);
extern int HL2i2cReadWrite(HERMESLITE2 *hl2);  
extern unsigned int HL2i2cSendTargetAddr(HERMESLITE2 *hl2);
extern unsigned int HL2i2cSendCommand(HERMESLITE2 *hl2);
extern int HL2i2cSendValue(HERMESLITE2 *hl2);

extern void HL2clock2Status(HERMESLITE2 *hl2, gboolean xvtr_on, const long int *clock_freq);

extern void HL2i2cProcessReturnValue(HERMESLITE2 *hl2, unsigned char c0,
                                     unsigned char c1, unsigned char c2, unsigned char c3, unsigned char c4);

extern long long HL2cl2CalculateNearest(HERMESLITE2 *hl2, long long lo_freq);

extern int hl2_get_tx_attenuation(HERMESLITE2 *hl2);
extern void hl2_set_tx_attenuation(HERMESLITE2 *hl2, int new_att);

extern void hl2_init(HERMESLITE2 *hl2);
#endif
