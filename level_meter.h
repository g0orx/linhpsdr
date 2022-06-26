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

extern gboolean level_meter_draw(cairo_t *cr, double x, int width, int height, const int fill);

extern void SetColour(cairo_t *cr, const int colour);

enum {
  BACKGROUND=0,
  OFF_WHITE=1,
  BOX_ON = 2,
  BOX_OFF = 3,
  TEXT_A = 4,
  TEXT_B = 5,
  TEXT_C = 6,
  WARNING = 7,
  DARK_LINES = 8,
  DARK_TEXT = 9,
  INFO_ON = 10,
  INFO_OFF = 11
};

enum {
  CLICK_ON=0,
  CLICK_OFF=1,
  INFO_TRUE = 2,
  INFO_FALSE = 3,
  WARNING_ON = 4
};
