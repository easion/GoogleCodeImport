#include "widget.h"
#include <cstdio>

class Yes : public NSButtonListener {
    void action(const gi_msg_t&, void *);
};

void Yes::action(const gi_msg_t&, void*) { printf("Yes\n"); }

class No : public NSButtonListener {
    void action(const gi_msg_t&, void *);
};

void No::action(const gi_msg_t&, void*) { printf("No\n"); }

int main()
{
  NSInitialize();

  NSFrame frame;
  NSButton yes("Yes", new Yes());
  NSButton no("No", new No());
  NSLabel label("Yes or No");
  NSVContainer con1(100, 200);
  NSHContainer con2(100, 100);

  frame.resize(100, 200);
  con2.add(&yes);
  con2.add(&no);
  con2.reallocate();
  con1.add(&label); 
  con1.add(&con2);
  con1.reallocate();
  frame.container(&con1);
  
  frame.map();
  
  NSMainLoop();

  return 0;
}
