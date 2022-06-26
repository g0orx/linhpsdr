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

#include <gtk/gtk.h>

#include "hl2.h"


#include "agc.h"
#include "mode.h"
#include "filter.h"
#include "bandstack.h"
#include "band.h"
#include "discovered.h"
#include "bpsk.h"
#include "peak_detect.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "vfo.h"
#include "meter.h"
#include "radio_info.h"
#include "rx_panadapter.h"
#include "tx_panadapter.h"
#include "waterfall.h"
#include "protocol1.h"
#include "protocol2.h"
#include "audio.h"
#include "receiver_dialog.h"
#include "configure_dialog.h"
#include "property.h"
#include "rigctl.h"
#include "subrx.h"
#include "audio.h"
#include "math.h"

// These functions mainly support i2c read/write for the hl2.
// See https://github.com/softerhardware/Hermes-Lite2/wiki/Protocol

// Multiple requests could be made by various threads to read/write
// i2c data from the HL2. This ensures all requests get through by using 
// a ring buffer.
static gboolean qos_timer_cb(void *data) {
  radio->qos_flag = FALSE;


  HERMESLITE2 *hl2=(HERMESLITE2 *)data;

  if ((hl2->late_packets > 7) && (radio->cwdaemon)) {
    g_print("Reset audio\n");    
    RECEIVER *tx_receiver = radio->transmitter->rx;
    if(tx_receiver!=NULL) {    
      audio_close_output(tx_receiver);
      audio_open_output(tx_receiver);
      radio->qos_flag = TRUE;
    }
  }
  
  if (hl2->ep6_error_ctr > 0) radio->qos_flag = TRUE;

  hl2->late_packets = 0;
  hl2->ep6_error_ctr = 0;
  
  return TRUE;
}

static gboolean mrf101data_timer_timer_cb(void *data) {
  HERMESLITE2 *hl2=(HERMESLITE2 *)data; 
  HL2i2cQueueWrite(hl2, I2C2_READ, ADDR_MRF101, 0x01, 0x00);
  
  return TRUE;
}

void HL2mrf101DataInit(HERMESLITE2 *hl2) {
  hl2->mrf101data_timer_id = g_timeout_add(200,mrf101data_timer_timer_cb,(gpointer)hl2); 
  HL2i2cQueueWrite(hl2, I2C2_WRITE, ADDR_MRF101, 0x02, 0x1E);
}

void hl2_init(HERMESLITE2 *hl2) {
  hl2->qos_timer_id = g_timeout_add(5000,qos_timer_cb,(gpointer)hl2);

  if (radio->filter_board == HL2_MRF101) {
    HL2mrf101DataInit(hl2);      
  }
}

void HL2mrf101SetBias(HERMESLITE2 *hl2) {
  // Write value without saving to EEPROM
  int addr = MCP4662_BIAS0;
  addr |= ((addr << 4) & 0xff);  
  HL2i2cQueueWrite(hl2, I2C2_WRITE, ADDR_MCP4561, 0, hl2->mrf101_bias_value);
}
  
void HL2mrf101StoreBias(HERMESLITE2 *hl2) {  
    // I2C address for the digital pot 
  int addr = MCP4662_BIAS0;
  addr |= ((addr << 4) & 0xff);
  g_print("Using addr %x", addr);
  HL2i2cQueueWrite(hl2, I2C2_WRITE, ADDR_MCP4561, addr, hl2->mrf101_bias_value);
}
  
void HL2mrf101ReadBias(HERMESLITE2 *hl2) {
  int addr = MCP4662_BIAS0;
  g_print("addr %d\n", addr);
  addr = ((addr << 4) & 0xff) | 0x0c;
  g_print("addr %d\n", addr);    
  HL2i2cQueueWrite(hl2, I2C2_READ, ADDR_MCP4561, addr, DUMMY_VALUE);  
}

void HL2i2cQueueWrite(HERMESLITE2 *hl2, int readwrite, unsigned int addr, unsigned int command, unsigned int value) {
  //g_print("value %2x\n", value);  
  unsigned long combined = 0;
  
  combined |= (readwrite << 24) & 0xFFFFFFFF;
  
  //g_print("%x\n", addr);
  addr |= 0x80;
  //g_print("%x\n", addr);
  combined |= (addr << 16) & 0xFFFFFFFF;
  combined |= (command << 8) & 0xFFFFFFFF;
  //g_print("command %6lx\n", combined); 
  value = value & 0xFF; 
  //g_print("value %2x\n", value);    
  combined |= (value) & 0xFFFFFFFF;
    
  //g_print("add to buffer %6lx\n", combined);
  
  g_mutex_lock(&hl2->i2c_mutex);
  queue_put(hl2->one_shot_queue, combined);
  g_mutex_unlock(&hl2->i2c_mutex);
}

void adc2_set_lna_gain(HERMESLITE2 *hl2, int gain) {
  hl2->adc2_lna_gain = gain;
  hl2->adc2_value_to_send = TRUE;
}
  
int adc2_check_send(HERMESLITE2 *hl2) {
  if (hl2->adc2_value_to_send) {
    hl2->adc2_value_to_send = FALSE;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

//Check if there is a value waiting to be sent
int HL2i2cWriteQueued(HERMESLITE2 *hl2) {
/*
  if (hl2->queue_busy == TRUE) {
    g_print("HL2i2cWriteQueued: queue busy\n");
    return FALSE;
  }
  */
  g_mutex_lock(&hl2->i2c_mutex);  
  if (queue_get(hl2->one_shot_queue, &hl2->value_to_send) == -1) {
    g_mutex_unlock(&hl2->i2c_mutex);
    //g_print("HL2i2cWriteQueued: nothing in queue\n");
    return FALSE;
  }
  else {
    //g_print("HL2i2cWriteQueued: something in queue\n");    
    g_mutex_unlock(&hl2->i2c_mutex);    
    return TRUE;
  }
}

// Is this command an i2c read or write?
int HL2i2cReadWrite(HERMESLITE2 *hl2) {
  //return hl2->value_to_send >> 24 & 0xFF;
  return hl2->value_to_send >> 24 & 0x7;  
  
}

// We need to know which address we are waiting for the HL2 to send back
// data for (when it is ready) 
unsigned int  HL2i2cSendTargetAddr(HERMESLITE2 *hl2) {
  hl2->addr_waiting_for = hl2->value_to_send >> 16 & 0xFF;
  //g_print("Waiting for addr %x\n", hl2->addr_waiting_for);
  return hl2->addr_waiting_for;
}

// We need to know which command we are waiting for the HL2 to send back
// data for (when it is ready)
unsigned int HL2i2cSendCommand(HERMESLITE2 *hl2) {
  hl2->command_waiting_for = hl2->value_to_send >> 8 & 0xFF;
  //g_print("Waiting for command %i\n", hl2->command_waiting_for);  
  return hl2->command_waiting_for;
}

int HL2i2cSendValue(HERMESLITE2 *hl2) {
  //hl2->queue_busy = TRUE;
  return hl2->value_to_send & 0xFF;  
}

int HL2i2cSendRqst(HERMESLITE2 *hl2) {
  int rqst;
  
  //g_print("send: %6lx\n", hl2->value_to_send);  
  //g_print("i2c flag: %lx\n", (hl2->value_to_send & 0x8000000));  
  //g_print("i2c flag: %lx\n", hl2->value_to_send);
  //g_print("i2c flag: %lx\n", (hl2->value_to_send & 0x8000000) >> 27);

  // 19th bit of the command to send denotes if it is an i2c2 or i2c1 command
  // i2c1 -> 0 or i2c2 -> 1
  if ((hl2->value_to_send & 0x8000000) >> 27) {
    rqst = (ADDR_I2C2 << 1) & 0xFF; 
    //g_print("i2c2\n");
  }
  else {
    rqst = (ADDR_I2C1 << 1) & 0xFF; 
    //g_print("i2c1\n");    
  }
    
  //g_print("rqst %x\n", rqst);
  if ((hl2->value_to_send >> 24 & 0x7) == I2C1_READ) {
    //g_print("Read\n");
    // Set bit 7 of metis C[0] for a rqst
    rqst |= 0x80;
  }
  //g_print("rqst mod %x\n", rqst);  
  return rqst;
}

void HL2i2cStoreValue(HERMESLITE2 *hl2, int raddr, long rdata) {
  //g_print("HL2i2cStoreValue raddr %x \n", (hl2->addr_waiting_for & 0x3F));  
  
  int bias = 0; 
  long pa_temp_hex = 0; 
  long pa_current_hex = 0;
  switch(hl2->addr_waiting_for & 0x3F) {
    case ADDR_MCP4662:
      switch(((hl2->command_waiting_for >> 4) & 0xff)) {
        case MCP4662_BIAS0:
          g_print("Bias0\n");
          bias = (rdata >> 24) & 0xFF;
          break;
        
        case MCP4662_BIAS1:
          g_print("Bias1\n");        
          bias = (rdata >> 24) & 0xFF;
          break;
          
        default:
          g_print("i2c command not known %i\n", ((hl2->command_waiting_for >> 4) & 0xff));          
          break;
      }
      g_print("HL2 PA Bias %d\n", bias);
      break;
    case ADDR_MCP4561:
      switch(((hl2->command_waiting_for >> 4) & 0xff)) {
        case MCP4662_BIAS0:
          g_print("Bias0\n");
          hl2->mrf101_bias_value = (rdata >> 24) & 0xFF;
          break;
                  
        default:
          g_print("i2c command not known %i\n", ((hl2->command_waiting_for >> 4) & 0xff));          
          break;
      }    
      g_print("HL2-MRF101 Bias %d\n", hl2->mrf101_bias_value );
      break;
    case ADDR_MRF101:
      //g_print("ADC return val %x\n", rdata);
       
      pa_temp_hex = rdata  & 0xFF;
      pa_current_hex = (rdata >> 8)  & 0xFF;
      
      double this_temperature = (double)pa_temp_hex;
      hl2->mrf101_temp = this_temperature; 

      double this_current = ((double)pa_current_hex / 255) *5;
      hl2->mrf101_current = this_current; 

      hl2->current_peak = get_peak(hl2->current_peak_buf, hl2->mrf101_current);
      break;
      
    default:
      g_print("i2c read error\n");
  }
}

int hl2_get_txbuffersize(HERMESLITE2 *hl2) {
  return hl2->hl2_tx_buffer_size;
}

void HL2i2cProcessReturnValue(HERMESLITE2 *hl2, unsigned char c0, 
                              unsigned char c1, unsigned char c2, 
                              unsigned char c3, unsigned char c4) {
  //hl2->queue_busy = FALSE;
  // i2c read addr = c0[6:1]
  //g_print("c0 %x \n", c0);  
  int raddr = (c0 >> 1) & 0xFF;
  //g_print("raddr %x \n", raddr);
  long rdata = 0;
  rdata |= (c1 << 24) & 0xFFFFFFFF;
  rdata |= (c2 << 16) & 0xFFFFFFFF;
  rdata |= (c3 << 8) & 0xFFFFFFFF;
  rdata |= (c4) & 0xFFFFFFFF;
  
  //g_print("Read i2c %x %6lx\n", raddr, rdata);  
  
  HL2i2cStoreValue(hl2, raddr, rdata); 
}


long long HL2cl2CalculateNearest(HERMESLITE2 *hl2, long long lo_freq) {
  g_print("HL2: Calculate nearest LO %lld \n", lo_freq);
  // VCO = 38.4*68 = 2611.2 MHz 
  //unsigned int divisor = (2611200000 / (double)hl2->clock2_freq) / 2;
  int divisor = (2611200000 / (double)lo_freq) / 2;
  g_print("----- Use divisor CL2 %i \n", divisor);  

  long long new_lo = (2611200000 / divisor) / 2;
  
  g_print("HL2: New LO %lld\n", new_lo);
  return new_lo;
}

// Versaclock CL2 output enable
void HL2cl2Enable(HERMESLITE2 *hl2) {
  g_print("HL2: Enable CL2 %ld \n", hl2->clock2_freq);
  // VCO = 38.4*68 = 2611.2 MHz 
  
  unsigned int div = 1;
  if (hl2->cl2_integer_mode) {
    unsigned int divisor = (2611200000 / (double)hl2->clock2_freq) / 2;
    div = (unsigned int)(divisor * pow(2, 24) + 0.1);
    g_print("-----Divisor CL2 %i \n", divisor);  
  } 
  else {
    double divisor = (2611200000 / (double)hl2->clock2_freq) / 2;
    div = (unsigned int)(divisor * pow(2, 24) + 0.1);
    g_print("-----Divisor CL2 %f \n", divisor);  
  }
  
  unsigned int addr = ADDR_VERSA5 >> 1;
  unsigned int intgr = div >> 24;
  unsigned int frac = (div & 0xFFFFFF) << 2;
  
  printf("frac %i\n", frac);
  // Clock2 CMOS1 output, 3.3V
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x62, 0x3b);  
  // Disable aux output on clock 1
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x2c, 0x00);   
  // Use divider for clock2
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x31, 0x81);  
  
  // Integer portion
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x3d, intgr >> 4);  
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x3e, ((intgr << 4) & 0xFF));    
    
  // Fractional portion
  // [29:22]
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x32, frac >> 24);    
  // [21:14]
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x33, frac >> 16);    
  // [13:6]
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x34, frac >> 8);    
  // [5:0] and disable ss
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x35, (frac & 0xFF)<<2);    
  
  //self.WriteVersa5(0x63,0x01)		# Enable clock2
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x63, 0x01);    
}

void HL2cl2Disable(HERMESLITE2 *hl2) {
  g_print("-----HL2: Disable CL2\n"); 
  
  int addr = ADDR_VERSA5 >> 1;
  // Disable divider output for clock2
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x31, 0x80);   
  // Disable clock2 output      
  HL2i2cQueueWrite(hl2, I2C1_WRITE, addr, 0x63, 0x00);     
}

void HL2clock2Status(HERMESLITE2 *hl2, gboolean xvtr_on, const long int *clock_freq) {

  if ((xvtr_on) && (hl2->cl2_enabled == FALSE)) { 
    hl2->clock2_freq = *clock_freq;
    HL2cl2Enable(hl2);
    hl2->cl2_enabled = TRUE;
  }
  else if ((xvtr_on == FALSE) && (hl2->cl2_enabled == TRUE)) {
    HL2cl2Disable(hl2);
    hl2->cl2_enabled = FALSE;
  }
}

void hl2_set_tx_attenuation(HERMESLITE2 *hl2, int new_att) {
  g_print("SET: new_att %i\n", new_att);
  hl2->lna_gain_tx =  new_att;
  g_print("SET: lna_tx %i\n", hl2->lna_gain_tx);
}

int hl2_get_tx_attenuation(HERMESLITE2 *hl2) {
  g_print("GET: lna_tx %i\n", hl2->lna_gain_tx);
  return MAX_GAIN - hl2->lna_gain_tx;
}

HERMESLITE2 *create_hl2(void) {
  HERMESLITE2 *hl2=g_new0(HERMESLITE2,1);
  
  hl2->one_shot_queue = create_long_ringbuffer(HL2_I2C_QUEUESIZE, 0);

  // HL2 i2c read/write
  hl2->addr_waiting_for = 0;
  hl2->command_waiting_for = 0;
 
  // MRF101 PA related
  hl2->mrf101_bias_enable = FALSE;
  hl2->mrf101_bias_value = 0;
  hl2->mrf101_temp = 0;
  
  // Diversity RX related 
  hl2->adc2_value_to_send = FALSE;
  hl2->adc2_lna_gain = 20;
  
//  hl2->queue_busy = FALSE;  
 
  // HL2 QOS/QOS settings
  hl2->hl2_tx_buffer_size = 0x15;
  hl2->overflow = FALSE;
  hl2->underflow = FALSE;
  hl2->late_packets = 0;
  hl2->ep6_error_ctr = 0;
  
  hl2->mrf101_temp = 0;
  hl2->mrf101_current = 0;
  hl2->current_peak = 0;
  hl2->current_peak_buf = create_peak_detector(CURRENT_PEAK_BUF_SIZE, 0);

  
  hl2->psu_clk = TRUE;

  // LNA gain during transmit
  hl2->lna_gain_tx = 20;
  
  // XVTR/CL2 related 
  hl2->cl2_enabled = FALSE;
  hl2->xvtr = FALSE;
  hl2->clock2_freq = 1;
  hl2->cl2_integer_mode = FALSE;

  return hl2;
}


