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
#include <math.h>
#include "level_meter.h"

void SetColour(cairo_t *cr, const int colour) {

  switch(colour) {
    case BACKGROUND: {
      cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
      break;
    }
    case OFF_WHITE: {
      cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
      break;
    }
    case BOX_ON: {
      cairo_set_source_rgb(cr, 0.624, 0.427, 0.690);
      break;
    }
    case BOX_OFF: {
      cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
      break;
    }
    case TEXT_A: {
      cairo_set_source_rgb(cr, 0.929, 0.616, 0.502);
      break;
    }
    case TEXT_B: {
      //light blue
      cairo_set_source_rgb(cr, 0.639, 0.800, 0.820);
      break;
    }
    case TEXT_C: {
      // Pale orange
      cairo_set_source_rgb(cr, 0.929, 0.616, 0.502);
      break;
    }
    case WARNING: {
      // Pale red
        cairo_set_source_rgb(cr, 0.851, 0.271, 0.271);
      break;
    }
    case DARK_LINES: {
      // Dark grey
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
      break;
    }
    case DARK_TEXT: {
      cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
      break;
    }
    case INFO_ON: {
      cairo_set_source_rgb(cr, 0.15, 0.58, 0.6);
      break;
    }
    case INFO_OFF: {
      cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
      break;
    }
  }
}

void set_stop_pattern(cairo_pattern_t *pat, const int colour, const double pc) {
  switch(colour) {
    case BACKGROUND: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.1, 0.1, 0.1);
      break;
    }
    case OFF_WHITE: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.9, 0.9, 0.9);
      break;
    }
    case BOX_ON: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.624, 0.427, 0.690);
      break;
    }
    case BOX_OFF: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.2, 0.2, 0.2);
      break;
    }
    case TEXT_A: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.929, 0.616, 0.502);
      break;
    }
    case TEXT_B: {
      //light blue
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.639, 0.800, 0.820);
      break;
    }
    case TEXT_C: {
      // Pale orange
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.929, 0.616, 0.502);
      break;
    }
    case WARNING: {
      // Pale red
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.851, 0.271, 0.271);
      break;
    }
    case DARK_LINES: {
      // Dark grey
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.3, 0.3, 0.3);
      break;
    }
    case DARK_TEXT: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.7, 0.7, 0.7);
      break;
    }
    case INFO_ON: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.15, 0.58, 0.6);
      break;
    }
    case INFO_OFF: {
      cairo_pattern_add_color_stop_rgb(pat, pc, 0.2, 0.2, 0.2);
      break;
    }
  }
}

gboolean level_meter_draw(cairo_t *cr, double x, int width, int height, const int fill) {
  SetColour(cr, BACKGROUND);
  cairo_rectangle(cr,0,0,width,height);
  cairo_fill(cr);  
  
  double bar_width=(double)width-10;

  
  cairo_pattern_t *pat3 = cairo_pattern_create_linear(0, (height/2)-1, width, (height/2)-1);

  cairo_pattern_add_color_stop_rgb(pat3, 0.05, 0.12, 0.12, 0.12);
  set_stop_pattern(pat3, fill, 0.8);
  cairo_pattern_add_color_stop_rgb(pat3, 0.95, 1, 0, 0);
  cairo_set_source(cr, pat3);
	cairo_pattern_destroy(pat3);
  
  cairo_set_line_width(cr, 10);  
  cairo_move_to(cr, 5, (height/4));
          
  static const double dashed2[] = {5.0, 2.0};
  static int len2  = sizeof(dashed2) / sizeof(dashed2[0]);        
  cairo_set_dash(cr, dashed2, len2, 0);          
          
  cairo_line_to(cr, width, (height/4));
  cairo_close_path(cr);
  cairo_stroke(cr);

  cairo_set_line_width(cr, 1);  
  
  cairo_set_dash(cr, 0, 0, 0);
  
  SetColour(cr, BACKGROUND);
  cairo_rectangle(cr,x,0,width,(height/2)-1);
  
  cairo_fill(cr);
  
  SetColour(cr, TEXT_B);
  cairo_move_to(cr,5,height/2);
  cairo_line_to(cr,width-5,height/2);
  cairo_stroke(cr);  
  
  SetColour(cr, TEXT_B);
  for(int i = 0; i <= 100; i += 25) {
    x=((double)i/100.0)*(double)bar_width;
    if((i%50)==0) {
      cairo_move_to(cr,x+5.0,(double)(height/2)-8.0);
    } else {
      cairo_move_to(cr,x+5.0,(double)(height/2)-3.0);
    }
    cairo_line_to(cr,x+5.0,height/2-1);
    cairo_stroke(cr);
  }

  return TRUE;  
}

