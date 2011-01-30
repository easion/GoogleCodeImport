#ifndef General_H
#define General_H

#include <errno.h>

#include <gi/gi.h>
#include <gi/property.h>
#include <gi/user_font.h>


#define UNDEFINE_CURSOR(w) 
#define DEFINE_CURSOR(w,t) 


#define XC_RED			GI_RGB(255,0,0)
//#define XC_BLACK		BLACK
#define XC_LIGHTSKYBLUE4	GI_RGB(96,123,139)
#define XC_FORESTGREEN		GI_RGB(34,139,34)
#define XC_DARKGREEN		GI_RGB(0,100,0)

#define XC_GREEN		GI_RGB(0,255,0)

#define DefaultBackground 	XC_FORESTGREEN
static const char VersionStr[] = "xfreecell 1.0.5a";

const int mainWindowWidth = 670, mainWindowHeight = 700;

const unsigned int numCards = 52;
const unsigned int numPlayStack = 8;
const unsigned int numSingleStack = 4;
const unsigned int numDoneStack = 4;

const unsigned int cardWidth = 79;
const unsigned int cardHeight = 123;

const unsigned int bmWidth = (cardWidth - 2 + 7) / 8;

enum Suit {
  Heart, Diamond, Club, Spade
};

enum SuitColor {
  RedSuit, BlackSuit, ERROR
};

#endif
