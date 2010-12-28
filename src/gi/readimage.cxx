//
// "$Id: readimage.cxx,v 1.1 2005/02/05 00:26:11 spitzak Exp $"
//
// X11 image reading routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
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
// Please report all bugs and problems to "fltk-bugs@fltk.org".

#include <fltk/string.h>

#ifdef DEBUG
#  include <stdio.h>
#endif // DEBUG


using namespace fltk;

uchar *				// O - Pixel buffer or NULL if failed
fltk::readimage(uchar *p,	// I - Pixel buffer or NULL to allocate
	PixelType type,		// Type of pixels to store (RGB and RGBA only now)
	const Rectangle& r,	// area to read
		int linedelta	// pointer increment per line
) {
  int   X = r.x();
  int   Y = r.y();
  int   w = r.w();
  int   h = r.h();
  int delta = depth(type);
  gi_image_t *image;
#if 1 //fixme
  char *pptr = (char*)malloc(w*h*4);

  //char 

  //
  // Under X11 we have the option of the XGetImage() interface or SGI's
  // ReadDisplay extension which does all of the really hard work for
  // us...
  //


  if (!image) {
   // image = XGetImage(xdisplay, xwindow, X, Y, w, h, AllPlanes, ZPixmap);
   image = gi_get_window_image(xwindow, X, Y, w, h,GI_RENDER_x8r8g8b8,0,pptr);
  }

  if (!image) return 0;


  int	x, y;			// Looping vars

  // None of these read alpha yet, so set the alpha to 1 everywhere.
  if (type > 3) {
    for (int y = 0; y < h; y++) {
      uchar *line = p + y * linedelta + 3;
      for (int x = 0; x < w; x++) {*line = 0xff; line += delta;}
    }
  }


  // Grab all of the pixels in the image, one at a time...
  // MRS: there has to be a better way than this!
  for (y = 0; y < h; y ++) {
    uchar *ptr = p + y*linedelta;
    for (x = 0; x < w; x ++, ptr += delta) {
      gi_color_t c = gi_image_getpixel(image, X + x, Y + y);
      ptr[0] = (uchar)c;
      c >>= 8;
      ptr[1] = (uchar)c;
      c >>= 8;
      ptr[2] = (uchar)c;
    }
  }

  gi_destroy_image(image);
  return p;


#endif

	TRACE_HERE;
	return 0;
}

//
// End of "$Id: readimage.cxx,v 1.1 2005/02/05 00:26:11 spitzak Exp $".
//
