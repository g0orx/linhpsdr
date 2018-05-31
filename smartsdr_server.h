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

#ifndef SMARTSDR_SERVER_H
#define SMARTSDR_SERVER_H

typedef struct _server {
  gint port;
} SERVER;

typedef enum {
    RECEIVER_DETACHED, RECEIVER_ATTACHED
} CLIENT_STATE;

#define VITA_PACKET_TYPE_EXT_DATA_WITH_STREAM_ID (0x03<<28)
#define VITA_HEADER_CLASS_ID_PRESENT (0x01<<27)
#define VITA_TSI_OTHER (0x03<<22)
#define VITA_TSF_SAMPLE_COUNT (0x01<<20)

#define MAX_DISCOVERY_PAYLOAD_SIZE 256

typedef struct _vita_ext_data_discovery {
  long header;
  long stream_id;
  long long class_id;
  long timestamp_int;
  long timestamp_frac_h;
  long timestamp_frac_l;
  char	payload[MAX_DISCOVERY_PAYLOAD_SIZE];
} VITA_EXT_DATA_DISCOVERY;

typedef struct _client {
    int socket;
    int address_length;
    struct sockaddr_in address;
    GThread *thread_id;
    CLIENT_STATE state;
    int receiver;
    int iq_port;
    int bs_port;
} CLIENT;


extern int create_smartsdr_server();

#endif
