/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "gdk.h"
#include "gdkprivate.h"
#include "gdkcursorfont.h"

#define PRINTFILE(s)	printf("gdkcursor.c:%s\n", s);
extern gi_screen_info_t gdk_screen_info;

GdkCursor*
gdk_cursor_new (GdkCursorType cursor_type)
{
  GdkCursorPrivate *private;
  GdkCursor *cursor;
  

  private = g_new (GdkCursorPrivate, 1);
  private->fg = gi_color_to_pixel(&gdk_screen_info,255, 255, 255);//dpp WHITE;
  private->bg = gi_color_to_pixel(&gdk_screen_info,0, 0, 0);;//dpp BLACK;
  switch(cursor_type)
  {
	case GDK_SB_H_DOUBLE_ARROW:
	case GDK_DOUBLE_ARROW:
               private->width= private->height = double_arrow_width;
                private->bitmap1fg = double_arrow_bits;
                private->bitmap1bg = double_arrow_bits;
                private->hot_x = double_arrow_x_hot;
                private->hot_y = double_arrow_y_hot;
		break;

	case GDK_SB_V_DOUBLE_ARROW:
               private->width= private->height = double_v_arrow_width;
                private->bitmap1fg = double_v_arrow_bits;
                private->bitmap1bg = double_v_arrow_bits;
                private->hot_x = double_v_arrow_x_hot;
                private->hot_y = double_v_arrow_y_hot;
		break;

	case GDK_XTERM:
  		private->height = private->width = xterm_width;
		private->bitmap1fg = xterm_bits;
		private->bitmap1bg = xterm_mask;
		private->hot_x = 0;
		private->hot_y = 0;
		break;

	case GDK_RIGHT_PTR:
                private->width= private->height = right_ptr_width;
                private->bitmap1fg = right_ptr_bits;
                private->bitmap1bg = right_ptrmsk_bits;
                private->hot_x = right_ptr_x_hot;
                private->hot_y = right_ptr_y_hot;
		break;

	case GDK_RIGHT_SIDE:
	case GDK_SB_RIGHT_ARROW:
                private->width= private->height = Right_width;
                private->bitmap1fg = Right_bits;
                private->bitmap1bg = Right_bits;
                private->hot_x = Right_x_hot;
                private->hot_y = Right_y_hot;
		break;

	case GDK_CENTER_PTR:
                private->width= private->height = cntr_ptr_width;
                private->bitmap1fg = cntr_ptr_bits;
                private->bitmap1bg = cntr_ptrmsk_bits;
                private->hot_x = cntr_ptr_x_hot;
                private->hot_y = cntr_ptr_y_hot;
		break;

	case GDK_SB_UP_ARROW:
	case GDK_TOP_SIDE:
	case GDK_BASED_ARROW_UP:
                private->width= private->height = cntr_ptr_width;
                private->bitmap1fg = cntr_ptr_bits;
                private->bitmap1bg = cntr_ptrmsk_bits;
                private->hot_x = cntr_ptr_x_hot;
                private->hot_y = 0;
		break;

	case GDK_LEFT_SIDE:
	case GDK_SB_LEFT_ARROW:
                private->width= private->height = Left_width;
                private->bitmap1fg = Left_bits;
                private->bitmap1bg = Left_bits;
                private->hot_x = Left_x_hot;
                private->hot_y = Left_y_hot;
		break;

	case GDK_SB_DOWN_ARROW:
	case GDK_BASED_ARROW_DOWN:
		private->width= private->height = Down_width;
                private->bitmap1fg = Down_bits;
                private->bitmap1bg =Down_bits;
                private->hot_x = Down_x_hot;
                private->hot_y = Down_y_hot;
		break;

	case GDK_STAR:
		private->width= private->height = star_width;
                private->bitmap1fg = star_bits;
                private->bitmap1bg = starMask_bits;
                private->hot_x = star_x_hot;
                private->hot_y = star_x_hot;
		break;

	case GDK_HAND2:
		private->width= private->height = hand1_width;
                private->bitmap1fg = hand1_bits;
                private->bitmap1bg = hand1Mask_bits;
                private->hot_x = hand1_x_hot;
                private->hot_y = hand1_y_hot;
		break;

	case GDK_LEFT_PTR:
	default:
		private->width= private->height = left_ptr_width;
         	private->bitmap1fg = left_ptr_bits;
 		private->bitmap1bg = left_ptrmsk_bits;
		private->hot_x = 0;
		private->hot_y = 0;
		break;
  }
  cursor = (GdkCursor*) private;
  cursor->type = cursor_type;

  return cursor;

}

GdkCursor*
gdk_cursor_new_from_pixmap (GdkPixmap *source, GdkPixmap *mask, GdkColor *fg, GdkColor *bg, gint x, gint y)
{
#if 0
  GdkCursorPrivate *private;
  GdkCursor *cursor;
  gi_window_id_t source_pixmap, mask_pixmap;

  source_pixmap = GDK_DRAWABLE_XID(source);
  mask_pixmap   = GDK_DRAWABLE_XID(mask);

  private->fg = GI_RGB(fg->red, fg->green, fg->blue);
  private->bg = GI_RGB(bg->red, bg->green, bg->blue);

  private = g_new (GdkCursorPrivate, 1);
  cursor = (GdkCursor *) private;
  private->width= private->height = star_width;
  private->bitmap1fg = star_bits;
  private->bitmap1bg = starMask_bits;
  private->hot_x = x;
  private->hot_y = y;
  cursor->type = GDK_CURSOR_IS_PIXMAP;

  return cursor;
#else
  return gdk_cursor_new(GDK_LEFT_PTR);
#endif
}

void
gdk_cursor_destroy (GdkCursor *cursor)
{
  GdkCursorPrivate *private;

  g_return_if_fail (cursor != NULL);

  private = (GdkCursorPrivate *) cursor;

  g_free (private);
}
