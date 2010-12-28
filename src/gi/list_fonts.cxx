
#include <fltk/x.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include "IFont.h"
using namespace fltk;

#include <gi/user_font.h>

////////////////////////////////////////////////////////////////
// Generate a list of every font known by X server:

// Sort X font names in alphabetical order of their "nice" names:
// This is horrendously slow, but only needs to be done once....
extern "C" {
static int sort_function(const void *aa, const void *bb) {
  char aname[128]; strncpy(aname, *(char**)aa,128);
  char bname[128]; strncpy(bname, *(char**)bb,128);
  int ret = strcasecmp(aname, bname); if (ret) return ret;
  return ret;
}
}

int fltk::list_fonts(fltk::Font**& arrayp) {
  static fltk::Font** font_array = 0;
  static int num_fonts = 0;
  if (font_array) {arrayp = font_array; return num_fonts;}

  open_display();
  int xlistsize;
  if (!current_font_)
    setfont(HELVETICA, 9);
  char **xlist = gi_list_fonts( "-*", 10000, &xlistsize);
  if (!xlist) return 0; // ???
  qsort(xlist, xlistsize, sizeof(*xlist), sort_function);

  IFont* family = 0; // current family
  char family_name[128] = ""; // nice name of current family

  int array_size = 0;
  for (int i=0; i<xlistsize;) {
#if 0
    char newname[128];
    int attribute = to_nice(newname, xlist[i]);

    // find all the matching fonts:
    int n = 1;
    for (; i+n < xlistsize; n++) {
      char nextname[128];
      int nextattribute = to_nice(nextname, xlist[i+n]);
      if (nextattribute != attribute || strcmp(nextname, newname)) break;
    }
#endif //fixme

    IFont* newfont;
    // See if it is one of our built-in fonts:
    //const char* skip_foundry = font_word(xlist[i],2);
   // int length = font_word(skip_foundry, 6)-skip_foundry;
   // for (int j = 0; ; j++) {
     // if (j >= 16)
	  // no, create a new font
	newfont = new IFont;
	newfont->f.name_ = newstring(xlist[i]);
	newfont->f.attributes_ = 0;
	newfont->system_name = xlist[i];
	//newfont->bold = newfont;
	//newfont->italic = newfont;
	newfont->first = 0;
	newfont->xlist = xlist+i;
	newfont->n = 0;
	//break;
      
      // see if it is one of our built-in fonts:
      // if so, set the list of x fonts, since we have it anyway
      //IFont* match = (IFont*)(fltk::font(j));
      //if (!strncmp(skip_foundry, match->system_name+2, length)) {
	//newfont = match;
	//if (!newfont->xlist) {
	//  newfont->xlist = xlist+i;
	//  newfont->n = -n;
	//}
	//break;
     // }
   
      family = newfont;
      strcpy(family_name, xlist[i]);
      if (num_fonts >= array_size) {
	array_size = 2*array_size+128;
	font_array = (fltk::Font**)realloc(font_array, array_size*sizeof(fltk::Font*));
      }
      font_array[num_fonts++] = &(newfont->f);

	  free(xlist[i]);
    

    i += 1;
  }

  free(xlist);
  arrayp = font_array;
  return num_fonts;
}

////////////////////////////////////////////////////////////////
int Font::encodings(const char**& arrayp) {
  // CET - FIXME - What about this encoding stuff?
  // WAS: we need some way to find out what charsets are supported
  // and turn these into ISO encoding names, and return this list.
  // This is a poor simulation:
  static const char* simulation[] = {"iso8859-1", 0};
  arrayp = simulation;
  return 1;
}


////////////////////////////////////////////////////////////////
int Font::sizes(int*& sizep) {
  static int simulation[1] = {0};
  sizep = simulation;
  return 1;
}

//
// End of "$Id: list_fonts_xlfd.cxx 4886 2006-03-30 09:55:32Z fabien $"
//
