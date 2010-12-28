
#include <fltk/error.h>
#include <fltk/math.h>
#include <string.h>

using namespace fltk;

static int syncnumber = 1;

struct fltk::Picture {
  int w, h, linedelta;
  unsigned long n; // bytes used
  uchar* data;
  int alpha;

  gi_image_t* bitmap;
  gi_window_id_t rgb;
  int syncro;
  Picture(int w, int h) {
	  this->alpha = 0;
    this->w = w;
    this->h = h;
    linedelta = w*4;
    n = linedelta*h;
    

	 bitmap = gi_create_image_depth(w,h,GI_RENDER_a8r8g8b8);
	 rgb = gi_create_pixmap_window(xwindow,
                        w, h, GI_RENDER_a8r8g8b8);
	 data = (uchar*)bitmap->rgba;
    syncro=0;
  }

  void sync() {
    if (syncro == syncnumber) {++syncnumber; 
	//GdiFlush();
	}
  }

  ~Picture() {
    sync();
    //DeleteObject(bitmap);
    gi_destroy_image(bitmap);
	if (rgb) gi_destroy_pixmap( rgb);
  }

};

int Image::buffer_width() const {
  if (picture) return picture->w;
  return w();
}

int Image::buffer_height() const {
  if (picture) return picture->h;
  return h();
}

int Image::buffer_linedelta() const {
  if (picture) return picture->linedelta;
  return w_*4;
}

int Image::buffer_depth() const {
  return 4;
}

PixelType Image::buffer_pixeltype() const {
  return ARGB32;
}

void Image::set_forceARGB32() {
  flags |= FORCEARGB32;
  // This does nothing, as it always is ARGB32
}

void Image::clear_forceARGB32() {
  flags &= ~FORCEARGB32;
  // This does nothing, as it always is ARGB32
}

unsigned long Image::mem_used() const {
  if (picture) return picture->n;
  return 0;
}

const uchar* Image::buffer() const {
  if (picture) return picture->data;
  return const_cast<Image*>(this)->buffer();
}

uchar* Image::buffer() {
  if (picture) {
    picture->sync();
  } else {
    open_display();
    picture = new Picture(w_,h_);
    memused_ += picture->n;
  }

  if (pixeltype_==MASK || pixeltype_==RGBA || pixeltype_>=ARGB32){
      picture->alpha=1;
  }

  return picture->data;
}

void Image::destroy() {
  if (!picture) return;
  if (picture->n > memused_) memused_ -= picture->n; else memused_ = 0;
  delete picture;
  picture = 0;
  flags &= ~FETCHED;
}

void Image::setpixeltype(PixelType p) {
  pixeltype_ = p;
  
}

void Image::setsize(int w, int h) {
  if (picture && (w > picture->w || h > picture->h)) destroy();
  w_ = w;
  h_ = h;
}

uchar* Image::linebuffer(int y) {
  buffer();
  return picture->data+y*picture->linedelta;
}

// Convert fltk pixeltypes to ARGB32:
// Go backwards so the buffers can be shared
static void convert(uchar* to, const uchar* from, PixelType type, int w) {
  U32* t = (U32*)to;
  switch (type) {
  case MONO:
    t += w;
    from += w;
    while (t > (U32*)to)
      *--t = 0xff000000 | (*--from * 0x10101);
    break;
  case MASK: {
    uchar mr,mg,mb;
    fltk::split_color(fltk::getcolor(),mr,mg,mb);
    t += w;
    from += w;
    while (t > (U32*)to) {
      uchar a = 255 - *--from;
      *--t = (a<<24) | (((mr*a)<<8)&0xff0000) | ((mg*a)&0xff00) | ((mb*a)>>8);
    }
    break;}
  case RGBx:
    t += w;
    from += 4*w;
    while (t > (U32*)to) {
      from -= 4;
      *--t = 0xff000000 | (from[0]<<16) | (from[1]<<8) | from[2];
    }
    break;
  case RGB:
    t += w;
    from += 3*w;
    while (t > (U32*)to) {
      from -= 3;
      *--t = 0xff000000 | (from[0]<<16) | (from[1]<<8) | from[2];
    }
    break;
  case RGBA:
    t += w;
    from += 4*w;
    while (t > (U32*)to) {
      from -= 4;
      *--t = (from[3]<<24) | (from[0]<<16) | (from[1]<<8) | from[2];
    }
    break;
  case RGB32:
  case ARGB32:
    if (from != to) memcpy(to, from, 4*w);
    break;
  case RGBM:
    t += w;
    from += 4*w;
    while (t > (U32*)to) {
      from -= 4;
      uchar a = from[3];
      *--t = (a<<24) | (((from[0]*a)<<8)&0xff0000) | ((from[1]*a)&0xff00) | ((from[2]*a)>>8);
    }
    break;
  case MRGB32:
    t += w;
    from += 4*w;
    while (t > (U32*)to) {
      from -= 4;
      U32 v = *(U32*)from;
      uchar a = v>>24;
      *--t = (v&0xff000000) |
          ((((v&0xff0000)*a)>>8) & 0xff0000) |
          ((((v&0xff00)*a)>>8) & 0xff00) |
          ((((v&0xff)*a)>>8) & 0xff);
    }
    break;
  }
}

void Image::setpixels(const uchar* buf, int y) {
  buffer();
  flags &= ~COPIED;
  uchar* to = picture->data+y*picture->linedelta;
  convert(to, buf, pixeltype_, width());
}

void Image::setpixels(const uchar* buf, const fltk::Rectangle& r, int linedelta)
{
  if (r.empty()) return;
  buffer();
  flags &= ~COPIED;
  uchar* to = picture->data+r.y()*picture->linedelta+r.x()*buffer_depth();
  // see if we can do it all at once:
  if (r.w() == picture->w && (r.h()==1 || linedelta == picture->linedelta)) {
    convert(to, buf, pixeltype_, r.w()*r.h());
  } else {
    for (int y = 0; y < r.h(); y++) {
      convert(to, buf, pixeltype_, r.w());
      buf += linedelta;
      to += picture->linedelta;
    }
  }
}

void Image::fetch_if_needed() const {
  if (pixeltype_==MASK) {
    int fg = fltk::get_color_index(fltk::getcolor())&0xffffff00;
    if ((flags & 0xffffff00) != fg)
      (const_cast<Image*>(this))->flags = (flags&0xff&~FETCHED)|fg;
  }
  if (!(flags&FETCHED)) {
    Image* thisimage = const_cast<Image*>(this);
    thisimage->fetch();
    thisimage->flags |= FETCHED;
  }
}


#if 1
void fl_restore_clip(); // in clip.cxx
void Image::draw(const fltk::Rectangle& from, const fltk::Rectangle& to) const {
	//gi_gc_ptr_t tmpgc;
	gi_image_t i;
	 gi_gc_ptr_t copygc=0;

	 fetch_if_needed();
  if (!picture) {fillrect(to); return;}


	 if (!(flags & COPIED)) {

    if (picture->rgb) {

      i.w = w();
      i.h = h();
      i.rgba = (gi_color_t*)picture->data;
	  if (picture->alpha ){
		   i.format = GI_RENDER_a8r8g8b8;
	  }
	  else
		 i.format = GI_RENDER_x8r8g8b8;
      //i.bytes_per_line = picture->linedelta;
     
      if (!copygc) copygc = gi_create_gc( picture->rgb,NULL);

	  i.pitch = gi_image_get_pitch((gi_format_code_t)i.format, i.w);

	  //gi_gc_attch_window(gc,xwindow);

	gi_bitblt_bitmap( copygc,0,0,w(),h(),&i,0, 0);
	gi_destroy_gc(copygc);

      //XPutImage(xdisplay, picture->rgb, copygc, &i, 0,0,0,0,w(),h());
    }

	((Image*)this)->flags |= COPIED;
	 }
	

  // XLib version:
  // unfortunately scaling does not work, so I just center and clip
  // to the transformed rectangle.
  // This is the rectangle I want to fill:
  Rectangle r2; transform(to,r2);
  // Center the image in that rectangle:
  Rectangle r1(r2,from.w(),from.h());
  // now figure out what area we will draw:
  Rectangle r(r1);
  if (r.w() >= r2.w()) {r.x(r2.x()); r.w(r2.w());}
  if (r.h() >= r2.h()) {r.y(r2.y()); r.h(r2.h());}
  // We must clip it because otherwise we can't do the alpha:
  if (!intersect_with_clip(r)) return;
  // fix for corner of from rectangle:
  r1.move(-from.x(), -from.y());


  if (picture->rgb) {
	  
    // RGB picture with no alpha
    gi_copy_area( picture->rgb, xwindow, gc,
              r.x()-r1.x(), r.y()-r1.y(), r.w(), r.h(), r.x(), r.y());
  }

  if (pixeltype_==MASK || pixeltype_==RGBA || pixeltype_>=ARGB32){
		 // fl_restore_clip();
	  }


  //picture->syncro = syncnumber;
 // DeleteDC(tempdc);
 //gi_destroy_gc(tmpgc);
  
}
#else



#endif

void Image::setimage(const uchar* d, PixelType p, int w, int h, int ld) {
  setsize(w,h);
  setpixeltype(p);
  setpixels(d, Rectangle(w,h), ld); flags = 0;
}

void Image::make_current() {
  buffer();


  //fixme
  draw_into(picture->rgb, picture->w, picture->h);
}

////////////////////////////////////////////////////////////////

// drawimage() calls this to see if a direct draw will work. Returns
// true if successful, false if an Image must be used to emulate it.

static bool innards(const uchar *buf, PixelType type,
		    const fltk::Rectangle& r1,
		    int linedelta,
		    DrawImageCallback cb, void* userdata)
{
	gi_image_t i;
	gi_gc_ptr_t tmpgc;
  if (type==MASK || type==RGBA || type>=ARGB32) return false;
  return false;


  int dx,dy,x,y,w,h;

  // because scaling is not supported, I just draw the image centered:
  // This is the rectangle I want to fill:
  Rectangle tr; transform(r1,tr);
  // Center the image in that rectangle:
  Rectangle r(tr, r1.w(), r1.h());
  // Clip image if it is bigger than destination rectangle:
  Rectangle cr(r);
  if (r1.w() >= tr.w()) {cr.x(tr.x()); cr.w(tr.w());}
  if (r1.h() >= tr.h()) {cr.y(tr.y()); cr.h(tr.h());}
  // Clip with current clip region,
  // otherwise the alpha mask will allow it to draw outside:
  if (!intersect_with_clip(cr)) return true;
  x = cr.x();
  y = cr.y();
  w = cr.w();
  h = cr.h();
  dx = x-r.x();
  dy = y-r.y();

  const int delta = depth(type);
  if (buf) buf += dx*delta + dy*linedelta;

  i.w = w;
  i.h = h;

  TRACE_HERE;
  return false;


  
  return true;
}

