#ifndef Util_H
#define Util_H

#include "general.h"
//#include <X11/Xlib.h>

unsigned long getColor( gi_color_t name);
SuitColor suitColor(Suit);

void makeBitmap(Suit, int, char*);
void makeOneSymbolBitmap(Suit, char*);
#endif
