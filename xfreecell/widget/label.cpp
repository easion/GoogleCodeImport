#include "widget.h"

gi_gc_ptr_t NSLabel::gc;
bool NSLabel::initialized = false;

NSLabel::NSLabel(const char* str)
  : NSWindow(true), NSString(str)
{
  if (!initialized) {
    gc = gi_create_gc( root(), 0);
    //XSetFont(NSdpy, gc, font);
	gc->values.font = font;
    gi_set_gc_foreground( gc, XC_BLACK);
    initialized = true;
  }
  selectInput(GI_MASK_EXPOSURE);
  NSWindow::resize(fontWindowWidth(), fontWindowHeight());
  NSWindow::background(XC_WHITE);
}

void NSLabel::label(const char* str)
{ 
    NSString::label(str);
    resize(fontWindowWidth(), fontWindowHeight()); 
    gi_clear_window( window(),NEEDREDRAW);
	gi_gc_attch_window(gc,window());
    TEXT_DRAW( gc, hGap(), height() - vGap(), _label.c_str(), _label.length());
}

void NSLabel::dispatchEvent(const gi_msg_t& ev)
{
  if (ev.type == GI_MSG_EXPOSURE /*&& ev.xexpose.count == 0*/){
	  gi_gc_attch_window(gc,window());
    TEXT_DRAW( gc, hGap(), height() - vGap(), _label.c_str(), _label.length());
  }
}
