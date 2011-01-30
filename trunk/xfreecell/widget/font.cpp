#include "widget.h"

NSFont::NSFont()
{
  _font = gi_create_ufont( defaultFont);
  //gi_ufont_set_text_attr(gc,_font,FALSE,GI_RGB(0,0,0),0);
  //_fontStruct = XQueryFont(NSdpy, _font);
}

NSFont::NSFont(gi_ufont_t* f)
{
  _font = f;
  //_fontStruct = XQueryFont(NSdpy, _font);
}

void NSFont::textExtents(const char* str, int* dir, int* asc, int* desc)
{
  //int direction, ascent, descent;

  //XTextExtents(_fontStruct, str, strlen(str), &direction, &ascent, &descent, cs);
  
  //if (dir != 0) *dir = direction;
  //if (asc != 0) *asc = ascent;
  //if (desc != 0) *desc = descent;
*dir = 0;
*asc = 13;
*desc = 0;
}

unsigned int NSFont::textWidth(const char* str)
{
  //XCharStruct cs;
  int w=0,h;

  gi_ufont_param(_font, str, strlen(str),&w,&h);

  //textExtents(str, &cs);
  //return cs.rbearing - cs.lbearing;
  return w;	// FIXME
}

unsigned int NSFont::textHeight(const char* str)
{
  //XCharStruct cs;

  //textExtents(str, &cs);
  //return cs.ascent + cs.descent;
  return gi_ufont_ascent_get(_font) + gi_ufont_descent_get(_font);	// FIXME
}
