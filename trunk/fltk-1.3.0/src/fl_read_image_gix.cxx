
uchar *				// O - Pixel buffer or NULL if failed
fl_read_image(uchar *p,		// I - Pixel buffer or NULL to allocate
              int   X,		// I - Left position
	      int   Y,		// I - Top position
	      int   w,		// I - Width of area to read
	      int   h,		// I - Height of area to read
	      int   alpha) {	// I - Alpha value for image (0 for none)

  int	d;			// Depth of image
  int err;

  // Allocate the image data array as needed...
  d = alpha ? 4 : 3;

  if (!p) p = new uchar[w * h * d];
  memset(p, alpha, w * h * d); 

  int ww = w; // We need the original width for output data line size

  int shift_x = 0; // X target shift if X modified
  int shift_y = 0; // Y target shift if X modified

  if (X < 0) {
    shift_x = -X;
    w += X;
    X = 0;
  }
  if (Y < 0) {
    shift_y = -Y;
    h += Y;
    Y = 0;
  }

  if (h < 1 || w < 1) return p;		// nothing to copy

  int line_size = ((3*w+3)/4) * 4;	// each line is aligned on a DWORD (4 bytes)
  uchar *dib = new uchar[line_size*h];	// create temporary buffer to read DIB

#if 0
  Fl_Window *win;
  int dx, dy, sx, sy, sw, sh;
  gi_window_id_t child_win;

  if (allow_outside) win = (Fl_Window*)1;
	else win = fl_find(fl_window);
  if (win) {
	  gi_translate_coordinates( fl_window,
		  GI_DESKTOP_WINDOW_ID, X, Y, &dx, &dy, &child_win);
	  // screen dimensions
	  Fl::screen_xywh(sx, sy, sw, sh, fl_screen);
  }
#endif
  if (fl_window)
  {
	printf("fl_read_image got ################\n");
	err = gi_get_window_buffer(fl_window,X,Y,w,h,
		line_size<<16 | FALSE,GI_RENDER_r8g8b8,dib);
  }
  else{
	  printf("%s: calling,get window image fail",__FUNCTION__);
  }

  // finally copy the image data to the user buffer

  for (int j = 0; j<h; j++) {
    const uchar *src = dib + j * line_size;			// source line
    uchar *tg = p + (j + shift_y) * d * ww + shift_x * d;	// target line
    for (int i = 0; i<w; i++) {
      uchar b = *src++;
      uchar g = *src++;
      *tg++ = *src++;	// R
      *tg++ = g;	// G
      *tg++ = b;	// B
      if (alpha)
	*tg++ = alpha;	// alpha
    }
  }

   delete[] dib;		// delete DIB temporary buffer

  return p;
}

