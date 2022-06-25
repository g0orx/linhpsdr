/* Copyright (C)
* 2021 - m5evt
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
#ifndef TX_INFO_METER_H
#define TX_INFO_METER_H

typedef struct _txmeter {
  GtkWidget *tx_meter_drawing;
  cairo_surface_t *tx_info_meter_surface;  
  char *label;
  gdouble meter_max;
  gdouble meter_min;
  
} TXMETER;

extern TXMETER *create_tx_info_meter(void);
extern void update_tx_info_meter(TXMETER *tx_meter, gdouble value, gdouble peak);

extern void configure_meter(TXMETER *meter, char *title, gdouble max_val, gdouble min_val);
#endif
