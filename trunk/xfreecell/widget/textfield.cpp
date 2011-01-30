#include "widget.h"

gi_gc_ptr_t NSTextField::gc;
const unsigned int NSTextField::gap;
bool NSTextField::initialized;
//XFontStruct* NSTextField::fontStruct;
unsigned int NSTextField::charWidth, NSTextField::charHeight;

NSTextField::NSTextField(unsigned int arg)
  : NSPlate(down)
{
  if (!initialized) {
    initialized = true;
    //fontStruct = XQueryFont(NSdpy, font);
    gc = gi_create_gc( window(),  0);
    gi_set_gc_foreground( gc, bgPixel() ^ XC_BLACK);
    gi_set_gc_function( gc, GI_MODE_XOR);
  }
  cursorPos = 0;
  strStart = 0;
  mode = down;
  cursorOnTF = false;
  width(arg);
  //height(fontStruct->max_bounds.ascent + fontStruct->max_bounds.descent + gap * 2);
  height(13);
  selectInput(GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT | GI_MASK_KEY_DOWN | GI_MASK_EXPOSURE);
  //orig  charWidth = fontStruct->max_bounds.rbearing - fontStruct->min_bounds.lbearing;
  charWidth = 11; //fontStruct->max_bounds.width - 1; // Can't figure out why, but this works.
  charHeight = 13; //fontStruct->max_bounds.ascent + fontStruct->max_bounds.descent;
  maxCharNum = width() / charWidth;
}

void NSTextField::draw()
{
  gi_clear_window( window(),NEEDREDRAW);
  NSPlate::redraw();

  gi_gc_attch_window(gc,window());

  TEXT_DRAW( gc, gap, height() - gap, _str.c_str() + strStart, _str.size() - strStart);
  if (cursorOnTF){
	  
    gi_fill_rect( gc, gap + (cursorPos - strStart) * charWidth, gap, charWidth, charHeight);
  }
}

void NSTextField::dispatchEvent(const gi_msg_t& ev)
{
  switch (ev.type) {
#if 0
  case KeyPress:
    {
      const int keyStringLength = 10;
      char keyString[keyStringLength];
      KeySym keysym;
      XKeyEvent xkey = ev.xkey;
      XLookupString(&xkey, keyString, keyStringLength, &keysym, NULL);

      if ((ev.xkey.state & ControlMask) == 0) {
	switch (keysym) {
	case XK_Left:
	  if (cursorPos == strStart && strStart > 0) strStart--;
	  if (cursorPos > 0) cursorPos--; 
	  draw();
	  break;
	case XK_Delete:
	  if (cursorPos < _str.length()) _str.erase(cursorPos, 1);
	  draw();
	  break;
	case XK_Right:
	  if (_str.length() > maxCharNum - 1 && cursorPos == maxCharNum - 1 + strStart) strStart++;
	  if (cursorPos < _str.length()) cursorPos++;
	  draw();
	  break;
	case XK_BackSpace:
	  if (cursorPos == strStart && strStart > 0) strStart--;
	  if (cursorPos > 0) {
	    _str.erase(cursorPos - 1, 1);
	    cursorPos--;
	  }
	  if (strStart > 0) strStart--;
	  draw();
	  break;
	}
	if (!isprint(keyString[0])) return;
	cursorPos++;
	if (cursorPos - strStart == maxCharNum) strStart++;
	if (cursorPos == _str.length()) 
	  _str += keyString;
	else 
	  _str.insert(cursorPos - 1, keyString);
	draw();
      } else {
	switch (keysym) {
	case XK_a: case XK_A:
	  cursorPos = 0; strStart = 0; break;
	case XK_b: case XK_B:
	  if (cursorPos == strStart && strStart > 0) strStart--;
	  if (cursorPos > 0) cursorPos--; 
	  break;
	case XK_d: case XK_D:
	  if (cursorPos < _str.length()) _str.erase(cursorPos, 1);
	  break;
	case XK_e: case XK_E:
	  if (_str.length() > maxCharNum - 1) strStart = _str.length() - maxCharNum + 1;
	  cursorPos = _str.length();
	  break;
	case XK_f: case XK_F:
	  if (_str.length() > maxCharNum - 1 && cursorPos == maxCharNum - 1 + strStart) strStart++;
	  if (cursorPos < _str.length()) cursorPos++;
	  break;
	case XK_h: case XK_H:
	  if (cursorPos == strStart && strStart > 0) strStart--;
	  if (cursorPos > 0) {
	    _str.erase(cursorPos - 1, 1);
	    cursorPos--;
	  }
	  if (strStart > 0) strStart--;
	  break;
	case XK_k: case XK_K:
	  _str.erase(cursorPos, _str.length() - cursorPos); break;
	}
	draw();
      }
      break;
    }
#endif
  case GI_MSG_EXPOSURE:
    NSPlate::redraw();
    draw();
    break;
  case GI_MSG_MOUSE_ENTER:
	  gi_gc_attch_window(gc,window());
    gi_fill_rect(  gc, gap + (cursorPos - strStart) * charWidth, gap, charWidth, charHeight);
    cursorOnTF = true;
    break;
  case GI_MSG_MOUSE_EXIT:
	  gi_gc_attch_window(gc,window());
    gi_fill_rect(  gc, gap + (cursorPos - strStart) * charWidth, gap, charWidth, charHeight);
    cursorOnTF = false;
    break;
  }
}
