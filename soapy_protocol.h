/* Copyright (C)
* 2015 - John Melton, G0ORX/N6LYT
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

#ifndef _LIME_PROTOCOL_H
#define _LIME_PROTOCOL_H

#define BUFFER_SIZE 1024

SoapySDRDevice *get_soapy_device();

void soapy_protocol_create_receiver(RECEIVER *rx);
void soapy_protocol_start_receiver(RECEIVER *rx);

void soapy_protocol_init(RADIO *r,int rx);
void soapy_protocol_stop();
void soapy_protocol_set_frequency(RECEIVER *rx);
void soapy_protocol_set_antenna(RECEIVER *rx,int ant);
void soapy_protocol_set_lna_gain(RECEIVER *rx,int gain);
void soapy_protocol_set_gain(RECEIVER *rx,char *name,int gain);
int soapy_protocol_get_gain(RECEIVER *rx,char *name);
void soapy_protocol_set_attenuation(RECEIVER *rx,int attenuation);
void soapy_protocol_change_sample_rate(RECEIVER *rx,int rate);
gboolean soapy_protocol_get_automatic_gain(RECEIVER *rx);
void soapy_protocol_set_automatic_gain(RECEIVER *rx,gboolean mode);
void soapy_protocol_start_transmitter(TRANSMITTER *tx);
void soapy_protocol_stop_transmitter(TRANSMITTER *tx);

#endif
