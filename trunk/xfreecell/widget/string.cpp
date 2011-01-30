#include "widget.h"

//XFontStruct* NSString::fontStruct;
const int NSString::fontGap;
bool NSString::stringInitialized = false;

NSString::NSString(const char* str)
  : _label(str), _resizable(false)
{
  if (!stringInitialized) {
    stringInitialized = true;
    //fontStruct = XQueryFont(NSdpy, font);
  }

  {
    //XCharStruct* charStruct = &fontStruct->max_bounds;
    _fontWindowHeight = gi_ufont_ascent_get(font) + gi_ufont_descent_get(font) + fontGap * 2;
  }

  {
    //int direction, ascent, descent;
    //XCharStruct charStruct;

    //XTextExtents(fontStruct, _label.c_str(), _label.length(), &direction, &ascent, &descent, &charStruct);

	 int w=0,h;

  gi_ufont_param(font, _label.c_str(), _label.length(),&w,&h);
  
    _fontWindowWidth = w+ fontGap * 2; //charStruct.rbearing - charStruct.lbearing ;
    _vGap = h;//_fontWindowHeight - (_fontWindowHeight)) / 2;
    _hGap = 0;//fontGap;
  }
}

void NSString::label(const char* str)
{
  _label = str;

  int w=0,h;

  gi_ufont_param(font, _label.c_str(), _label.length(),&w,&h);
  
  //int direction, ascent, descent;
  //XCharStruct charStruct;
  
  //XTextExtents(fontStruct, _label.c_str(), _label.length(), &direction, &ascent, &descent, &charStruct);

  if (_resizable) {
    _hGap = 0; //(_fontWindowWidth - charStruct.rbearing + charStruct.lbearing) / 2;
    _vGap = h; //(_fontWindowHeight - charStruct.ascent - charStruct.descent) / 2;
  } else {
    _fontWindowWidth = w+ fontGap * 2; //charStruct.rbearing - charStruct.lbearing ;
    _vGap = 0; //(_fontWindowHeight - (charStruct.ascent + charStruct.descent)) / 2;
  }
}

void NSString::fontWindowWidth(unsigned int arg)
{
  //int direction, ascent, descent;
  //XCharStruct charStruct;
  
  //XTextExtents(fontStruct, _label.c_str(), _label.length(), &direction, &ascent, &descent, &charStruct);

  if (_resizable) {
    _fontWindowWidth = arg;
  }
}

void NSString::fontWindowHeight(unsigned int arg)
{
  //int direction, ascent, descent;
  //XCharStruct charStruct;
  
  //XTextExtents(fontStruct, _label.c_str(), _label.length(), &direction, &ascent, &descent, &charStruct);

  if (_resizable) {
    _fontWindowHeight = arg;
  }
}
