
#define X_UTF8_FONT 0

#include <fltk/draw.h>
#include <fltk/error.h>
#include <fltk/x.h>
#include <fltk/math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fltk/utf.h>
#include <gi/user_font.h>
#include "IFont.h"
using namespace fltk;

//typedef struct {    /* normal 16 bit characters are two bytes */
//    unsigned char byte1;
//    unsigned char byte2;
//} XChar2b;

// One of these is created for each font+size+encoding combination we use:
struct FontSize {
  FontSize* next;	// linked list for a single Font
  gi_ufont_t* font;
  const char* encoding;
  const char* name;
  unsigned size;
  //unsigned minsize;	// smallest point size that should use this
  //unsigned maxsize;	// largest point size that should use this
  unsigned opengl_id; // for OpenGL display lists
  FontSize(const char* xfontname, unsigned size);
    ~FontSize();
};



fltk::Font* fltk::Font::plus(int x) {
  IFont* font = (IFont*)this;
  //if (x & BOLD) font = font->bold;
  //if (x & ITALIC) font = font->italic;
  return &(font->f);
}

const char* fltk::Font::system_name() {
  return ((IFont*)this)->system_name;
}

static FontSize *current;
static gi_gc_ptr_t font_gc; // which gc the font was set in last time

FL_API unsigned fl_font_opengl_id() {return current->opengl_id;}
FL_API void fl_set_font_opengl_id(unsigned v) {current->opengl_id = v;}

FontSize::~FontSize() {
  if (this == current) current = 0;
  gi_delete_ufont(font);
}



/*! Returns the operating-system dependent structure defining the
  current font. You must include <fltk/x.h> to use this. */
//gi_ufont_t* fltk::xfont() {return current->font;}

/*!
  Return the full X11 name for the currently selected font+size+encoding.
  This is mostly useful for debugging, though maybe you need to copy
  it to another program.

  On non-X systems, and on modern X systems with Xft (antialiased fonts)
  this returns current_font()->system_name().
*/
const char* fltk::Font::current_name() {return current->name;}

FontSize::FontSize(const char* name, unsigned size) {
	char fontnamebuf[512];

	if (!name)
	{
  this->name =  ("heiti");
	}
	else{
  this->name =  (name);
	}

  snprintf(fontnamebuf,512,"%s/%d",this->name,size);

  //printf("FontSize::FontSize begion %s  %s\n",this->name,fontnamebuf);
  font = gi_create_ufont( fontnamebuf);
  if (!font) {
    warning("bad font: %s", fontnamebuf);
    //font = gi_create_ufont( "/system/font/arial.ttf"); // if fixed fails we crash
    font = gi_create_ufont( "Vera/9"); // if fixed fails we crash
	
	//current_xpixel
  }
  if (font)
  {
	  this->size=size;
	 // gi_ufont_set_size(font,size);
	gi_ufont_set_text_attr(gc,font,FALSE,GI_RGB(0,0,0),0);
  }
  encoding = "UTF-8";
  opengl_id = 0;
}


void fltk::drawtext_transformed(const char *text, int n, float x, float y) {
	int dx,dy;
	int R,G,B;
	//gi_screen_info_t si;
	gi_pixel_to_color(NULL,(gi_color_t)current_xpixel,&R,&G,&B);
	//SetTextColor(dc, current_xpixel);	


  if (font_gc != gc) {
    // I removed this, the user MUST set the font before drawing: (was)
    // if (!current) setfont(HELVETICA, NORMAL_SIZE);
    font_gc = gc;
    //XSetFont(xdisplay, gc, current->font->fid);
  }

	gi_ufont_set_text_attr(gc,current->font,FALSE,GI_RGB(R,G,B),0);
	  {
	  dx=int(floorf(x+.5f));
	  dy=int(floorf(y+.5f));
	  gi_gc_attch_window(gc,xwindow);
	  if (dy>getascent())
	  {
		  dy-=getascent();
	  }
	
	//printf("%s: %s,%d at %d,%d\n",__FUNCTION__,text,n,(int)dx,(int)dy);
    gi_ufont_draw( gc,current->font,text,dx,	dy,  n);
  }
}

float fltk::getascent()  { 
	//printf("current->font->metrics.ascent= %d\n",current->font->metrics.ascent);
	return gi_ufont_ascent_get(current->font);
	}

float fltk::getdescent() {
	//printf("current->font->metrics.descent= %d\n",current->font->metrics.descent);
	return gi_ufont_descent_get(current->font);
	}

float fltk::getwidth(const char *text, int n) {
  int w,h;
  float r=0;

	gi_ufont_param(current->font, text, n,&w,&h);
	r=w;
	//printf("%s: %s,%d at %d,%d\n",__FUNCTION__,text,n,(int)w,h);
	return r;
}

////////////////////////////////////////////////////////////////
// The rest of this file is the enormous amount of crap you have to
// do to get a font & size out of X.  To select a font+size, all
// matchine X fonts are listed with XListFonts, and then the first
// of the following is found and used:
//
//	pixelsize == size
//	pixelsize == 0 (which indicates a scalable font)
//	the largest pixelsize < size
//	the smallest pixelsize > size
//
// If any fonts match the encoding() then the search is limited
// to those matching fonts. Otherwise all fonts are searched and thus
// a random encoding is chosen.
//
// I have not been able to find any other method than a search
// that will reliably return a bitmap version of the font if one is
// available at the correct size.  This is because X will not use a
// bitmap font unless all the extra fields are filled in correctly.
//
// Fltk uses pixelsize, not "pointsize".  This is what everybody wants!

void fltk::setfont(Font* font, float psize) {
  FontSize* f = current;
  IFont* t = (IFont*)font;

  // only integers supported right now (this can be improved):
  psize = int(psize+.5);
  unsigned size = unsigned(psize);


  

  // See if the current font is correct:
  if (font == current_font_ && psize == current_size_ &&
      (f->encoding==encoding_ ||
	   0 )){
	  //printf("fltk::setfont to %d got line %d\n", size,__LINE__);
    return;
  }
  current_font_ = font; current_size_ = psize;

#if 0
  // search the FontSize we have generated already:
  for (f = t->first; f; f = f->next)
    if (f->size == size && (f->encoding==encoding_ ||
 	    !f->encoding ||
		0 )) {
	  //printf("fltk::setfont to %d got line %d\n", size,__LINE__);
      goto DONE;
    }
#endif

  // run XListFonts if it has not been done yet:
  if (!t->xlist) {
    open_display();
	//fixme
    t->xlist = NULL;// XListFonts(xdisplay, t->system_name, 100, &(t->n));
    if (!t->xlist || t->n <= 0) {	// use variable if no matching font...
      //t->first = f = new FontSize("simsum",size);
      t->first = f = new FontSize("arial",size);
	  //printf("fltk::setfont to %d got line %d\n", size,__LINE__);
      goto DONE;
    }
  }

	{

  char* name;
  if(t->xlist)
  name = t->xlist[0];
  else{
	  name = "heiti";
  }
  int ptsize=size;

  printf("fltk::setfont to %d got line %d\n", size,__LINE__);  

  // okay, we definately have some name, make the font:
  f = new FontSize(name,size);
  // we pretend it has the current encoding even if it does not, so that
  // it is quickly matched when searching for it again with the same
  // encoding:
  f->encoding = encoding_;
  f->size = ptsize;   
  f->next = t->first;
  t->first = f;}
 DONE:
  if (f != current) {
    current = f;
    font_gc = 0;
  }
}

////////////////////////////////////////////////////////////////

// The predefined fonts that fltk has:
static IFont fonts [] = {
  {{"helvetica",0}, "-*-helvetica-medium-r-normal--*"},
  {{"helvetica",1}, "-*-helvetica-bold-r-normal--*"},
  {{"helvetica",2}, "-*-helvetica-medium-o-normal--*"},
  {{"helvetica",3}, "-*-helvetica-bold-o-normal--*"},
  {{"courier", 0},  "-*-courier-medium-r-normal--*"},
  {{"courier", 1},  "-*-courier-bold-r-normal--*"},
  {{"courier", 2},  "-*-courier-medium-o-normal--*"},
  {{"courier", 3},  "-*-courier-bold-o-normal--*"},
  {{"times", 0},    "-*-times-medium-r-normal--*"},
  {{"times", 1},    "-*-times-bold-r-normal--*"},
  {{"times", 2},    "-*-times-medium-i-normal--*"},
  {{"times", 3},    "-*-times-bold-i-normal--*"},
  {{"symbol", 0},   "-*-symbol-*"},
  {{"lucidatypewriter", 0}, "-*-lucidatypewriter-medium-r-normal-sans-*"},
  {{"lucidatypewriter", 1}, "-*-lucidatypewriter-bold-r-normal-sans-*"},
  {{"zapf dingbats", 0}, "-*-*zapf dingbats-*"},
};

fltk::Font* const fltk::HELVETICA 		= &(fonts[0].f);
fltk::Font* const fltk::HELVETICA_BOLD		= &(fonts[1].f);
fltk::Font* const fltk::HELVETICA_ITALIC	= &(fonts[2].f);
fltk::Font* const fltk::HELVETICA_BOLD_ITALIC	= &(fonts[3].f);
fltk::Font* const fltk::COURIER 		= &(fonts[4].f);
fltk::Font* const fltk::COURIER_BOLD		= &(fonts[5].f);
fltk::Font* const fltk::COURIER_ITALIC		= &(fonts[6].f);
fltk::Font* const fltk::COURIER_BOLD_ITALIC	= &(fonts[7].f);
fltk::Font* const fltk::TIMES 			= &(fonts[8].f);
fltk::Font* const fltk::TIMES_BOLD		= &(fonts[9].f);
fltk::Font* const fltk::TIMES_ITALIC		= &(fonts[10].f);
fltk::Font* const fltk::TIMES_BOLD_ITALIC	= &(fonts[11].f);
fltk::Font* const fltk::SYMBOL_FONT		= &(fonts[12].f);
fltk::Font* const fltk::SCREEN_FONT		= &(fonts[13].f);
fltk::Font* const fltk::SCREEN_BOLD_FONT	= &(fonts[14].f);
fltk::Font* const fltk::ZAPF_DINGBATS		= &(fonts[15].f);

/*! For back-compatabilty with FLTK1, this turns an integer into one
  of the built-in fonts. 0 = HELVETICA. */
fltk::Font* fltk::font(int i) {return &(fonts[i%16].f);}

//
// End of "$Id: Font_xlfd.cxx 5955 2007-10-17 19:46:35Z spitzak $"
//



