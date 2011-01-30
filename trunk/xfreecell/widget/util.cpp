#include "widget.h"

//STATIC_FUNCTIONS;

unsigned long allocColor( unsigned int red, unsigned int green, unsigned int blue)
{
  //XColor col = { 0, red, green, blue};
  //if (XAllocColor(NSdpy, DefaultColormap(NSdpy, 0), &col) == 0) {
    //printf(" Warning : Color map full (%x,%x,%x) \n",red,green,blue); 
    //return WhitePixel(NSdpy, 0);
  //}
  //return col.pixel;
  return GI_RGB(red,green,blue);
}

unsigned long nameToPixel(const char* name)
{
  //Colormap cmap;
  //XColor c0, c1;

  //cmap = DefaultColormap(NSdpy, 0);
  //XAllocNamedColor(NSdpy, cmap, name, &c1, &c0);

  //return c1.pixel;
printf("nameToPixel %s\n", name);
  return XC_BLACK;		// FIXME
}

void NSInitialize()
{
  if ( gi_init() < 0) {
    fprintf(stderr, "Cannot open display\n");
    exit(1);
  }
  font = gi_create_ufont( defaultFont);
  //gi_ufont_set_text_attr(gc,font,FALSE,GI_RGB(0,0,0),0);
}
 
void NSMainLoop()
{
  gi_msg_t ev;
  NSWindow* win;
  while (true) {
    gi_get_message(&ev, MSG_ALWAYS_WAIT);
    //win = NSWindow::windowToNSWindow(ev.xany.window);
    win = NSWindow::windowToNSWindow(ev.ev_window);
    if (win != 0) win->dispatchEvent(ev);
  }
}

void NSNextEvent(gi_msg_t* ev)
{
  gi_get_message(ev, MSG_ALWAYS_WAIT);
}

void NSDispatchEvent(const gi_msg_t& ev)
{
  NSWindow* win;

  //win = NSWindow::windowToNSWindow(ev.xany.window);
  win = NSWindow::windowToNSWindow(ev.ev_window);
  if (win != 0) win->dispatchEvent(ev);
}

//Display* NSdisplay() { return NSdpy; }


static const uint8_t mirror[256] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};



/*
 * Create a GdBitmap-compatible bitmap (16-bit short array) from data bits
 *     flags specify input format
 *     caller must free return buffer
 *
 * Currently only works if width/height < bits_width/bits_height
 */
uint16_t *
gi_create_bitmap_from_data(int width, int height, int bits_width,
        int bits_height, void *bits, gi_bool_t brev,gi_bool_t bswap)
{
	int x, y;
	int xb;
	//int brev = flags & GR_BMDATA_BYTEREVERSE;
	//int bswap = flags & GR_BMDATA_BYTESWAP;
	uint8_t *inbuf = (uint8_t *)bits;
	uint8_t *p;
	uint16_t *buf;

	/*
	* bit reverse or byte-swap short words in image
	* and pad to 16 bits for GrBitmap()
	*/
	xb = (width + 7) / 8;   /* FIXME: may not be in packed-8bit format */
	buf = (uint16_t *) malloc(((xb + 1) & ~01) * height);
	if (!buf)
	return NULL;
	p = (uint8_t *) buf;

  for (y = 0; y < height; ++y) {
	for (x = 0; x < xb;) {
		/* This handles the situation where we may not be 16 byte aligned */
		if ((xb - x) == 1) {
				uint8_t c = *inbuf++;
				*p++ = brev ? mirror[c] : c;
				x++;
		} else {
			if (bswap) {
					/* byte-swap short words */
					uint8_t c = *inbuf++;
					p[1] = brev ? mirror[c] : c;
					if (x < xb - 1) {
							c = *inbuf++;
							p[0] = brev ? mirror[c] : c;
					} else
							p[0] = 0;
			} else {
					/* no byte swapping */
					uint8_t c = *inbuf++;
					p[0] = brev ? mirror[c] : c;
					if (x < xb - 1) {
							c = *inbuf++;
							p[1] = brev ? mirror[c] : c;
					} else
							p[1] = 0;
			}

			x += 2;
			p += 2;
		}
	}
	inbuf += (bits_width + 7) / 8 - (width + 7) / 8;        /* FIXME */
	/* pad to 16 bits */
	if (xb & 1) {
			uint8_t c = *(p-1);
			if (bswap) {
					*(p-1) = 0;
					*p++ = c;
			} else {
					*p++ = 0;
			}
	}
  }
  return buf;
}


#if 0
/*
 * Create a bitmap from a specified pixmap
 * This function may not be needed if Microwindows implemented a depth-1 pixmap.
 */
uint16_t *
GrNewBitmapFromPixmap(gi_window_id_t pixmap, MWCOORD x, MWCOORD y,
        int width, int height)
{
        unsigned int w, h;
        unsigned int bwidth, rowsize;
        GR_PIXELVAL *pixel, *buffer;
        uint16_t *bitmap;

        buffer = malloc(width * height * sizeof(GR_PIXELVAL));
        if (!buffer)
                return NULL;

        bwidth = (width + 15) / 16;
        rowsize = sizeof(uint16_t) * ((bwidth + 1) & ~01);
        bitmap = (uint16_t *) calloc(rowsize, height);
        if (!bitmap) {
                free(buffer);
                return NULL;
        }

        /* Now, read the pixmap and set bitmap bit if pixel non-zero */
        GrReadArea(pixmap, x, y, width, height, (void *) buffer);

        pixel = buffer;
        for (h = 0; h < height; h++) {
                for (w = 0; w < width; w++) {
                        if (*pixel++)
                                bitmap[(h * bwidth) + (w >> 4)] |=
                                        (1 << (15 - (w % 16)));
                }
        }
        free(buffer);
        return bitmap;
}

#endif

