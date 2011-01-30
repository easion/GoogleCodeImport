#include "widget.h"

NSButton::NSButton
(const char* str, NSButtonListener* bl, void* la, NSButtonCallback* bc, void* ca)
  : NSPlate(up), NSString(str), _listener(bl), _callback(bc),
    buttonPressed(false), cursorOnButton(false), _listenerArg(la), _callbackArg(ca)
{
  gc = gi_create_gc( root(), 0);
  //XSetFont(NSdpy, gc, font);
  gc->values.font = font;
  gi_set_gc_foreground( gc, XC_BLACK);
  selectInput(GI_MASK_EXPOSURE | GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT | 
	      GI_MASK_BUTTON_DOWN | GI_MASK_BUTTON_UP);
  NSWindow::resize(fontWindowWidth(), fontWindowHeight());
}

void NSButton::resize(unsigned int arg1, unsigned int arg2)
{
  resizable(true);
  fontWindowResize(arg1, arg2);
  NSWindow::resize(arg1, arg2);
  clear();
  gi_gc_attch_window(gc,window());
  TEXT_DRAW(gc, hGap(), height() - vGap(), _label.c_str(), _label.length());
  NSPlate::redraw(up);
}

void NSButton::width(unsigned int arg)
{
  resizable(true);
  fontWindowWidth(arg);
  NSWindow::width(arg);
  clear();
  gi_gc_attch_window(gc,window());
  TEXT_DRAW( gc, hGap(), height() - vGap(), _label.c_str(), _label.length());
  NSPlate::redraw(up);
}

void NSButton::height(unsigned int arg)
{
  resizable(true);
  fontWindowHeight(arg);
  NSWindow::height(arg);
  clear();
  gi_gc_attch_window(gc,window());
  TEXT_DRAW( gc, hGap(), height() - vGap(), _label.c_str(), _label.length());
  NSPlate::redraw(up);
}

void NSButton::label(const char* str)
{
  NSString::label(str);
  NSWindow::resize(fontWindowWidth(), fontWindowHeight());
  clear();
  gi_gc_attch_window(gc,window());
  TEXT_DRAW( gc, hGap(), height() - vGap(), _label.c_str(), _label.length());
  NSPlate::redraw(up);
}

void NSButton::redraw()
{
	gi_gc_attch_window(gc,window());
    TEXT_DRAW( gc, hGap(), height() - vGap(), _label.c_str(), _label.length());
    NSPlate::redraw(up);  
}

void NSButton::dispatchEvent(const gi_msg_t& ev)
{
  switch (ev.type) {
  case GI_MSG_EXPOSURE:
    redraw();
    break;
  case GI_MSG_MOUSE_ENTER:
    cursorOnButton = true;
    if (buttonPressed) mode = down;
    else mode = flat;
    NSPlate::redraw();
    break;
  case GI_MSG_MOUSE_EXIT:
    NSPlate::redraw(up);
    cursorOnButton = false;
    break;
  case GI_MSG_BUTTON_DOWN:
    buttonPressed = true;
    NSPlate::redraw(down);
    break;
  case GI_MSG_BUTTON_UP:
    buttonPressed = false;
    if (cursorOnButton && _listener != 0) _listener->buttonAction(ev, _listenerArg);
    if (cursorOnButton && _callback != 0) _callback(ev, _callbackArg);
    if (cursorOnButton) mode = flat;
    else mode = up;
    NSPlate::redraw();
    break;
  }
}

// ##### NSToggleButton #####
bool NSToggleButton::initialized = false;
unsigned int NSToggleButton::squareLength;

NSToggleButton::NSToggleButton(const char* str, bool t)
  : NSButton(str)
{
  if (!initialized) {
    initialized = true;
    squareLength = height() - 2 * vGap();
  }

  _toggled = t;
  width(width() + hGap() + squareLength);
}

void NSToggleButton::redraw()
{
  if (_toggled) 
    rect3D(down, width() - hGap() - squareLength, vGap(), squareLength, squareLength);
  else 
    rect3D(up, width() - hGap() - squareLength, vGap(), squareLength, squareLength);
}

void NSToggleButton::dispatchEvent(const gi_msg_t& ev)
{ 
  switch (ev.type) {
  case GI_MSG_EXPOSURE:
    NSButton::redraw();
    redraw();
    break;
  case GI_MSG_MOUSE_ENTER:
    NSPlate::redraw(flat);
    break;
  case GI_MSG_MOUSE_EXIT:
    NSPlate::redraw(up);
    break;
  case GI_MSG_BUTTON_UP:
    if (_toggled) _toggled = false; else _toggled = true;
    redraw();
    break;
  }
}
