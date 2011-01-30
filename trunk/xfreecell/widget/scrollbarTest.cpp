#include "widget.h"

class Listener : public NSScrollbarListener {
public:
  void vScrollAction(const gi_msg_t&, void*, int);
  void hScrollAction(const gi_msg_t&, void*, int);
};

void Listener::vScrollAction(const gi_msg_t&, void*, int pos) 
{
  printf("V: delta = %d\n", pos);
}

void Listener::hScrollAction(const gi_msg_t&, void*, int pos)
{
  printf("H: delta =  %d\n", pos);
}

int main()
{
  Listener l;

  NSInitialize();

  NSFrame frame;
  NSVContainer con(500, 500);
  NSVScrollbar vsb(10, 200, &l);
  NSHScrollbar hsb(10, 400, &l);

  vsb.movement(5);
  hsb.movement(4);

  con.add(&vsb);
  con.add(&hsb);
  con.reallocate();
  frame.container(&con);

  frame.map();
  
  //XFlush(NSdisplay());

  NSMainLoop();

  return 0;
}
