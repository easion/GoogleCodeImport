#include "widget.h"

bool NSWindow::windowInitialized = false;
gi_window_id_t NSWindow::_root;
vector<NSWindow::Elt> NSWindow::eltVector;

NSWindow::NSWindow(bool create, gi_window_id_t w, int x, int y, unsigned int width, unsigned int height, 
		   unsigned int borderWidth, unsigned long border, unsigned long bg)
{
  if (!windowInitialized) {
    windowInitialized = true;
    
    _root = GI_DESKTOP_WINDOW_ID;
  }
  if (create) {
    _window = gi_create_window( w, x, y, width, height,  bg, GI_FLAGS_NON_FRAME); //fixme
  //printf("NSWindow::NSWindow = %x parent=%x\n",_window,w);
    _x = x; _y = y;
    _width = width; _height = height;
    registerWindow(this, _window);
  } /* else {
    _window = w;
    gi_window_id_t rootRet;
    int xRet, yRet;
    unsigned int widthRet, heightRet, borderRet, depthRet;
    XGetGeometry(NSdpy, w, &rootRet, &xRet, &yRet, &widthRet, &heightRet, &borderRet, &depthRet);
    _x = xRet; _y = yRet;
    _width = widthRet; _height = heightRet;
  }
    */
}

NSWindow::~NSWindow()
{
  eraseWindow(_window);
  gi_destroy_window( _window);
}

void NSWindow::move(int arg1, int arg2) 
{ 
  if (arg1 != _x || arg2 != _y) {
    _x = arg1; _y = arg2; 
    gi_move_window( _window, _x, _y);
  }
}

void NSWindow::resize(unsigned int arg1, unsigned int arg2) 
{ 
  if (arg1 != _width || arg2 != _height) {
    _width = arg1; _height = arg2;
if (_width && _height)
    gi_resize_window( _window, _width, _height); 
//else XUnmapWindow(NSdpy, _window);
  }
}

void NSWindow::selectInput(long mask)
{
  //XWindowAttributes attr;
  //XGetWindowAttributes(NSdpy, _window, &attr);
  //gi_set_events_mask( _window, attr.your_event_mask | mask);
  gi_set_events_mask( _window, mask);	// FIXME
}

void NSWindow::setMaxMinSize(unsigned int maxW, unsigned int maxH, unsigned int minW, unsigned int minH)
{
  //XSizeHints hint;

  //hint.max_width = maxW; hint.max_height = maxH;
  //hint.min_width = minW; hint.min_height = minH;
  //if (maxW != 0 && maxH != 0) hint.flags |= PMaxSize;
  //if (minW != 0 && minH != 0) hint.flags |= PMinSize;
  //XSetWMNormalHints(NSdpy, _window, &hint);
}

NSWindow* NSWindow::windowToNSWindow(gi_window_id_t w)
{
  int l = 0, h = eltVector.size();

  while (l < h) {
    int i = (l + h ) / 2;
    if (w == eltVector[i].window) return eltVector[i].nswindow;
    if (w > eltVector[i].window) l = i + 1;
    else h = i;
  }
  if (l >= h) return 0;
  if (w == eltVector[l].window) return eltVector[l].nswindow;
  return 0;
}

void NSWindow::registerWindow(NSWindow* nsw, gi_window_id_t w)
{
  vector<Elt>::iterator begin = eltVector.begin();
  vector<Elt>::iterator end = eltVector.end();
  Elt elt(nsw, w);

  if (eltVector.size() == 0 || w > eltVector.back().window) {
    eltVector.push_back(elt);
    return;
  }

  for (vector<Elt>::iterator iter = begin; iter != end; iter++)
    if ((*iter).window > w)
      eltVector.insert(iter, elt);
  
  fprintf(stderr, "registerWindow bug\n");
}

void NSWindow::eraseWindow(gi_window_id_t w)
{
  vector<Elt>::iterator begin = eltVector.begin();
  vector<Elt>::iterator end = eltVector.end();
  vector<Elt>::iterator iter;

  for (iter = begin; iter != end; iter++) {
    if ((*iter).window == w) eltVector.erase(iter);
    return;
  }
  
  fprintf(stderr, "eraseWindow bug\n");
}
