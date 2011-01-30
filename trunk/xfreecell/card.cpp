#include <cstdio>
#include <math.h>
#ifdef SHAPE
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
//#include <X11/extensions/shape.h>
#endif

#include "card.h"
#include "freecell.h"
#include "option.h"
#include "undo.h"
#include "util.h"

#include "cursor.bm"
#include "cursor_back.bm"
#ifdef SHAPE
#include "boundingMask.bm"
#include "clipMask.bm"
#endif

// Typedef
enum MoveMode { SingleMode, MultipleMode };

// Extern declarations

extern gi_window_id_t gameWindow;

// 
Card* hilighted = 0;
bool cursorChanged = false;
gi_image_t* cursor;

// Static declarations
const time_t doubleClickInterval = 250; // milisecond

static bool initialized = false;
#ifdef SHAPE
static gi_window_id_t boundingMask;
static gi_window_id_t clipMask;
#endif

static char bitmap[bmWidth * (cardHeight - 2)];
static MoveMode moveMode;

Card::Card(Suit s, unsigned int v)
  : NSWindow(true, gameWindow, 0, 0, cardWidth, cardHeight, 1, XC_BLACK)
{
  if (!initialized) {
#ifdef SHAPE
    if (Option::roundCard()) {
     	    //Shape
	    boundingMask = XCreateBitmapFromData(dpy, root(),
				boundingMask_bits, boundingMask_width,
				boundingMask_height);
	    clipMask = XCreateBitmapFromData(dpy, root(), clipMask_bits,
				clipMask_width, clipMask_height);
    }
#endif

    //Cursor
    //XColor fore, back, xc;
    //gi_window_id_t p, mask;
    //Colormap cm = DefaultColormap(dpy, 0);
    //p =  XCreateBitmapFromData(dpy, gameWindow, (char*) cursor_bits, cursor_width, cursor_height);
    //mask = XCreateBitmapFromData(dpy, gameWindow, (char*) cursor_back_bits, cursor_back_width, cursor_back_height);
    //XAllocNamedColor(dpy, cm, "white", &fore, &xc);
    //XAllocNamedColor(dpy, cm, "black", &back, &xc);
    //cursor = XCreatePixmapCursor(dpy, p,mask, &fore, &back, 0, 0);

#ifdef GIXOK
    uint16_t *p = gi_create_bitmap_from_data(16, 16, cursor_width, cursor_height, cursor_bits,
    	TRUE,TRUE);
    uint16_t *mask =  gi_create_bitmap_from_data(16, 16, cursor_back_width, cursor_back_height,
    	cursor_back_bits, TRUE,TRUE);
    cursor = GrNewCursor(16, 16, 8, 0, XC_BLACK, XC_WHITE, p, mask);
    free(p);
    free(mask);
#endif

    initialized = true;
  }

  _parent = 0;
  _stack = 0;
  _suit = s;
  _value = v;
  _removed = false;
  makeBitmap(s, v, bitmap);

  // initialization on X things
  unsigned long fore, back, hilight;

  if (suitColor(s) == RedSuit)
    fore = getColor( XC_RED);
  else
    fore = getColor( XC_BLACK);
  back = XC_WHITE;
  hilight = getColor( XC_LIGHTSKYBLUE4);


  _usualPixmap = 
    gi_create_pixmap_from_bitmap( gameWindow, bitmap, cardWidth - 2,
	cardHeight - 2, fore, back, gi_screen_format() );
  _hilightedPixmap = 
    gi_create_pixmap_from_bitmap( gameWindow, bitmap, cardWidth - 2,
	cardHeight - 2, fore, hilight, gi_screen_format() );


  selectInput(GI_MASK_BUTTON_DOWN | GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT);
  backgroundPixmap(_usualPixmap);

#ifdef SHAPE
  if (Option::roundCard()) {
    XShapeCombineMask(dpy, window(), ShapeBounding, 0, 0, boundingMask, ShapeSet);
    XShapeCombineMask(dpy, window(), ShapeClip, 0, 0, clipMask, ShapeSet);
  }
#endif

  NSWindow::move(0, 0);

  map();
}

void Card::move(int dest_x, int dest_y, bool animate)
{
  raise();

  if (animate && 1 /*Option::animation()*/) {
    int oldx = x();
    int oldy = y();
    int newx = dest_x;
    int newy = dest_y;
    int steps = 20; // GRH was max(abs(oldx - newx), abs(oldy - newy)) / Option::speedup();
    float curx = (float) oldx;
    float cury = (float) oldy;

    if (steps == 0) {
      NSWindow::move(newx, newy);
      //XFlush(dpy);
    } else {
      for (int i = 0; i < steps; i++) {
	curx += ((float) (newx - oldx)) / steps;
	cury += ((float) (newy - oldy)) / steps;
	NSWindow::move((int) curx, (int) cury);
	//XFlush(dpy);
      }
    }
  } else {
    NSWindow::move(dest_x, dest_y);
  }

  raise();
}

void Card::moveToStack(Stack* s, bool autoMoving, bool pushUndo)
{
  if (s == 0 || s == _stack) return;

  if (pushUndo)
    undoAddMove(_stack, s);

  _stack->popCard(); 
  _stack = s;

  Card* top = _stack->topCard();
  if (top != 0 && top->canBeParent(this))
    parent(_stack->topCard());
  else
    parent(0);

  _stack->pushCard(this);
  move(_stack->next_x(), _stack->next_y(), true);

  if (autoMoving) autoMove();
}

void Card::moveToStackInitial(Stack* s)
{
  _stack = s;

  Card* top = _stack->topCard();
  if (top != 0 && top->canBeParent(this))
    parent(_stack->topCard());
  else
    parent(0);

  _stack->pushCard(this);
   move(_stack->next_x(), _stack->next_y(), false);
   //move(_stack->next_x(), _stack->next_y(), true);	//GRH animate in deck initialize
}

bool Card::canBeParent(Card* c) const
{
  if (c == 0) return false;

  return ( suitColor(c->suit()) != suitColor(_suit) ) && (_value == c->value() + 1);
}

void Card::dispatchEvent(const gi_msg_t& ev)
{
  switch (ev.type) {
  case GI_MSG_BUTTON_DOWN:
    dispatchButtonPress(ev);
    break;
  case GI_MSG_MOUSE_ENTER:
    dispatchEnterNotify(ev);
    break;
  case GI_MSG_MOUSE_EXIT:
    dispatchLeaveNotify(ev);
    break;
  }
}

void Card::dispatchButtonPress(const gi_msg_t& ev)
{
  static time_t lastPressTime = 0;

  if (hilighted == _stack->topCard()) {
    _stack->topCard()->unhilighten();
    hilighted = 0;
    //if (ev.xbutton.time - lastPressTime < doubleClickInterval) {
    if (ev.time - lastPressTime < doubleClickInterval) {
      SingleStack* stack = emptySingleStack();
      if (stack != 0)
	_stack->topCard()->moveToStack(stack);
    }
  } else if (hilighted == 0 && !_removed) { 
    switch (ev.params[2]) {
    case GI_BUTTON_L:
      _stack->topCard()->hilighten();
      hilighted = _stack->topCard();
      //lastPressTime = ev.xbutton.time;
      lastPressTime = ev.time;
      break;
    case GI_BUTTON_M:
      { 
	SingleStack* stack = emptySingleStack();
	if (stack != 0)
	  _stack->topCard()->moveToStack(stack);
      }
      break;
    case GI_BUTTON_R:
      moveToAppropriateDoneStack(_stack->topCard());
      break;
    }
  } else if (cursorChanged) {
    // cursorChanged == true means moving is possible. 
    if (moveMode == SingleMode) {
      hilighted->unhilighten();
      hilighted->moveToStack(_stack);
      hilighted = 0;
      UNDEFINE_CURSOR( window());
      cursorChanged = false;
    } else if (moveMode == MultipleMode) {
      hilighted->unhilighten();
      moveMultipleCards(hilighted, _stack->topCard());
      hilighted = 0;
      UNDEFINE_CURSOR( window());
      cursorChanged = false;
    } else {
      fprintf(stderr, "Bug in Card::dispatchButtonPress cursorChanged\n");
      exit(1);
    }
  }
}

void Card::dispatchEnterNotify(const gi_msg_t&)
{
  if (hilighted == 0) return;

  if (_stack->acceptable(hilighted)) {
    DEFINE_CURSOR( window(), cursor);
    cursorChanged = true;
    moveMode = SingleMode;
  } else if (multipleMovable(hilighted, _stack->topCard())) {
    DEFINE_CURSOR( window(), cursor);
    cursorChanged = true;
    moveMode = MultipleMode;
  }
}

void Card::dispatchLeaveNotify(const gi_msg_t&)
{
  if (cursorChanged) {
    UNDEFINE_CURSOR( window());
    cursorChanged = false;
  }
}

// Interface to external
void initializeHilighted()
{
  hilighted = 0;
}

void initializeCursor()
{
  cursorChanged = false;
}
