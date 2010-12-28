
// Return the current state of a key.  This is the X version.  I identify
// keys (mostly) by the X keysym.  So this turns the keysym into a keycode
// and looks it up in the X key bit vector, which x.C keeps track of.

#include <fltk/events.h>
#include <fltk/x.h>

extern char fl_key_vector[32]; // in x.C

extern unsigned short fltk2gi(unsigned fltk);



bool fltk::event_key_state(unsigned keysym) {
  if (keysym > 0 && keysym <= 8)
    return event_state(BUTTON(keysym)) != 0;
  int keycode=fltk2gi(keysym);

  if (!keycode)
  {
   keycode = keysym & 0xff; // undo the |0x8000 done to unknown keycodes
  } 

  return (fl_key_vector[keycode/8] & (1 << (keycode%8))) != 0;
}

bool fltk::get_key_state(unsigned key) {
  open_display();
  //XQueryKeymap( fl_key_vector);
  return event_key_state(key);
}
