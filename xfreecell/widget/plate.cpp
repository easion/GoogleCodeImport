#include "widget.h"

bool NSPlate::plateInitialized = false;
gi_gc_ptr_t NSPlate::bgGC, NSPlate::bRimGC, NSPlate::dRimGC;
unsigned long NSPlate::_bgPixel, NSPlate::_bRimPixel, NSPlate::_dRimPixel;
const int NSPlate::_thickness;

NSPlate::NSPlate(Mode m)
  : NSWindow(true, GI_DESKTOP_WINDOW_ID), mode(m)
{
  if (!plateInitialized) {
    plateInitialized = true;

    _bgPixel = allocColor( 0x6180,0xa290,0xc300); 
    _bRimPixel = allocColor( 0x8a20,0xe380,0xffff); 
    _dRimPixel = allocColor( 0x30c0,0x5144,0x6180);

    bgGC = gi_create_gc( root(),  0);
    gi_set_gc_foreground( bgGC, _bgPixel);
    bRimGC = gi_create_gc( root(), 0);
    gi_set_gc_foreground( bRimGC, _bRimPixel);
    dRimGC = gi_create_gc( root(), 0);
    gi_set_gc_foreground( dRimGC, _dRimPixel);
  }

  selectInput(GI_MASK_EXPOSURE);
  background(_bgPixel);
}

void NSPlate::rect3D(Mode m, int x, int y, unsigned int w, unsigned int h)
{
  gi_gc_ptr_t upperLeft, bottomRight;
  switch (m) {
  case up: upperLeft = bRimGC; bottomRight = dRimGC; break;
  case down: upperLeft = dRimGC; bottomRight = bRimGC; break;
  case flat: default: upperLeft = bottomRight = bgGC; break;
  }
    
  for (int i = 0; i < _thickness; i++) {
    short xa = x + i, ya = y + i, xe = x + w - i, ye = y + h - i;
    gi_point_t xp[5] = { {xa,ye}, {xa,ya}, {xe,ya}, {xe,ye}, {xa,ye}}; 
	gi_gc_attch_window(upperLeft,window());
	gi_gc_attch_window(bottomRight,window());
    gi_draw_lines(  upperLeft, xp, 3);
    gi_draw_lines( bottomRight, xp+2, 3);
  }
}

void NSPlate::dispatchEvent(const gi_msg_t& ev)
{
  if (ev.type == GI_MSG_EXPOSURE) redraw();
}
