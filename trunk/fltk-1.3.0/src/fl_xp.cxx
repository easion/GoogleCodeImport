//
// "$Id: fl_xp.cxx 446 2008-01-30 15:44:56Z sanel.z $"
//
// "XP" drawing routines for the Fast Light Tool Kit (FLTK).
//
// These box types provide a XP look and are heavily based 
// on GTK look by Michael Sweet.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// Box drawing code for an obscure box type.
// These box types are in seperate files so they are not linked
// in if not used.

#include <FL/Fl.H>
#include <FL/fl_draw.H>

extern void fl_internal_boxtype(Fl_Boxtype, Fl_Box_Draw_F*);

static void xp_color(Fl_Color c) {
  if (Fl::draw_box_active()) 
    fl_color(c);
  else 
    fl_color(fl_inactive(c));
}

static void xp_up_frame(int x, int y, int w, int h, Fl_Color c) {
  xp_color(fl_color_average(FL_WHITE, c, 0.9));
  fl_xyline(x + 2, y + 1, x + w - 3);
  fl_yxline(x + 1, y + 2, y + h - 3);

  xp_color(fl_color_average(fl_rgb_color(0, 60, 115), c, 0.5));

  fl_begin_loop();
    fl_vertex(x, y + 2);
    fl_vertex(x + 2, y);
    fl_vertex(x + w - 3, y);
    fl_vertex(x + w - 1, y + 2);
    fl_vertex(x + w - 1, y + h - 3);
    fl_vertex(x + w - 3, y + h - 1);
    fl_vertex(x + 2, y + h - 1);
    fl_vertex(x, y + h - 3);
  fl_end_loop();
}

static void xp_up_box(int x, int y, int w, int h, Fl_Color c) {
  xp_up_frame(x, y, w, h, c);

  // top shade
  float weight = 0.8f;
  for (int i = 2; i < 7; i++, weight -= 0.1f) {
    xp_color(fl_color_average(FL_WHITE, c, weight));
    fl_xyline(x + 2, y + i, x + w - 3);
  }

  // background
  xp_color(c);
  fl_rectf(x + 2, y + 7, w - 4, h - 9);

  // bottom shade
  xp_color(fl_color_average(FL_BLACK, c, 0.025f));
  fl_xyline(x + 2, y + h - 4, x + w - 3);
  xp_color(fl_color_average(FL_BLACK, c, 0.05f));
  fl_xyline(x + 2, y + h - 3, x + w - 3);
  xp_color(fl_color_average(FL_BLACK, c, 0.1f));
  fl_xyline(x + 2, y + h - 2, x + w - 3);
  fl_yxline(x + w - 2, y + 2, y + h - 3);
}

static void xp_down_frame(int x, int y, int w, int h, Fl_Color c) {
  xp_color(fl_color_average(fl_rgb_color(0, 60, 115), c, 0.5));
  fl_begin_loop();
    fl_vertex(x, y + 2);
    fl_vertex(x + 2, y);
    fl_vertex(x + w - 3, y);
    fl_vertex(x + w - 1, y + 2);
    fl_vertex(x + w - 1, y + h - 3);
    fl_vertex(x + w - 3, y + h - 1);
    fl_vertex(x + 2, y + h - 1);
    fl_vertex(x, y + h - 3);
  fl_end_loop();

  xp_color(fl_color_average(FL_BLACK, c, 0.1f));
  fl_xyline(x + 2, y + 1, x + w - 3);
  fl_yxline(x + 1, y + 2, y + h - 3);

  xp_color(fl_color_average(FL_BLACK, c, 0.05f));
  fl_yxline(x + 2, y + h - 2, y + 2, x + w - 2);
}

static void xp_down_box(int x, int y, int w, int h, Fl_Color c) {
  xp_down_frame(x, y, w, h, c);

  xp_color(c);
  fl_rectf(x + 3, y + 3, w - 5, h - 4);
  fl_yxline(x + w - 2, y + 3, y + h - 3);
}

static void xp_thin_up_frame(int x, int y, int w, int h, Fl_Color c) {
  xp_color(fl_color_average(fl_rgb_color(148, 166, 181), c, 0.6f));
  fl_xyline(x + 1, y, x + w - 2);
  fl_yxline(x, y + 1, y + h - 2);

  xp_color(fl_color_average(FL_BLACK, c, 0.4f));
  fl_xyline(x + 1, y + h - 1, x + w - 2);
  fl_yxline(x + w - 1, y + 1, y + h - 2);
}

static void xp_thin_up_box(int x, int y, int w, int h, Fl_Color c) {
  xp_thin_up_frame(x, y, w, h, c);

  xp_color(fl_color_average(FL_WHITE, c, 0.4f));
  fl_xyline(x + 1, y + 1, x + w - 2);
  xp_color(fl_color_average(FL_WHITE, c, 0.2f));
  fl_xyline(x + 1, y + 2, x + w - 2);
  xp_color(fl_color_average(FL_WHITE, c, 0.1f));
  fl_xyline(x + 1, y + 3, x + w - 2);
  xp_color(c);
  fl_rectf(x + 1, y + 4, w - 2, h - 8);
  xp_color(fl_color_average(FL_BLACK, c, 0.025f));
  fl_xyline(x + 1, y + h - 4, x + w - 2);
  xp_color(fl_color_average(FL_BLACK, c, 0.05f));
  fl_xyline(x + 1, y + h - 3, x + w - 2);
  xp_color(fl_color_average(FL_BLACK, c, 0.1f));
  fl_xyline(x + 1, y + h - 2, x + w - 2);
}

static void xp_thin_down_frame(int x, int y, int w, int h, Fl_Color c) {
  xp_color(fl_color_average(fl_rgb_color(148, 166, 181), c, 0.6f));
  fl_xyline(x + 1, y, x + w - 2);
  fl_yxline(x, y + 1, y + h - 2);

  xp_color(fl_color_average(FL_WHITE, c, 0.6f));
  fl_xyline(x + 1, y + h - 1, x + w - 2);
  fl_yxline(x + w - 1, y + 1, y + h - 2);
}

static void xp_thin_down_box(int x, int y, int w, int h, Fl_Color c) {
  xp_thin_down_frame(x, y, w, h, c);

  xp_color(c);
  fl_rectf(x + 1, y + 1, w - 2, h - 2);
}

static void xp_round_up_box(int x, int y, int w, int h, Fl_Color c) {
  xp_color(c);
  fl_pie(x, y, w, h, 0.0, 360.0);
  xp_color(fl_color_average(FL_WHITE, c, 0.5f));
  fl_arc(x, y, w, h, 45.0, 180.0);
  xp_color(fl_color_average(FL_WHITE, c, 0.25f));
  fl_arc(x, y, w, h, 180.0, 405.0);
  xp_color(fl_color_average(FL_BLACK, c, 0.5f));
  fl_arc(x, y, w, h, 225.0, 360.0);
}

static void xp_round_down_box(int x, int y, int w, int h, Fl_Color c) {
  xp_color(c);
  fl_pie(x, y, w, h, 0.0, 360.0);
  xp_color(fl_color_average(FL_BLACK, c, 0.2));
  fl_arc(x + 1, y, w, h, 90.0, 210.0);
  xp_color(fl_color_average(FL_BLACK, c, 0.6));
  fl_arc(x, y, w, h, 0.0, 360.0);
}

Fl_Boxtype fl_define_FL_XP_UP_BOX() {
  fl_internal_boxtype(_FL_XP_UP_BOX, xp_up_box);
  fl_internal_boxtype(_FL_XP_DOWN_BOX, xp_down_box);
  fl_internal_boxtype(_FL_XP_UP_FRAME, xp_up_frame);
  fl_internal_boxtype(_FL_XP_DOWN_FRAME, xp_down_frame);
  fl_internal_boxtype(_FL_XP_THIN_UP_BOX, xp_thin_up_box);
  fl_internal_boxtype(_FL_XP_THIN_DOWN_BOX, xp_thin_down_box);
  fl_internal_boxtype(_FL_XP_THIN_UP_FRAME, xp_thin_up_frame);
  fl_internal_boxtype(_FL_XP_THIN_DOWN_FRAME, xp_thin_down_frame);
  fl_internal_boxtype(_FL_XP_ROUND_UP_BOX, xp_round_up_box);
  fl_internal_boxtype(_FL_XP_ROUND_DOWN_BOX, xp_round_down_box);

  return _FL_XP_UP_BOX;
}


//
// End of "$Id: fl_xp.cxx 446 2008-01-30 15:44:56Z sanel.z $".
//
