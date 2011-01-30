#include <stack>

#include "card.h"
#include "stack.h"

// There is no need to make undo a class. It adds another 
// global variable, and I hate g v.

struct Move {
  Stack* from;
  Stack* to;

  Move(Stack* f, Stack* t) { from = f; to = t; }
  void doUndo();
};

void Move::doUndo()
{
  Card* c = to->topCard();

  if (c->isRemoved()) c->undone();
  c->moveToStack(from, false, false);
}

stack<Move> moves;

void undoClearMoves()
{
  while (moves.size() > 0)
    moves.pop();
}

void undoAddMove(Stack* from, Stack* to)
{
  Move m(from, to);
  moves.push(m);
}

void undoDoUndo()
{
  if (moves.size() == 0) return;

  moves.top().doUndo();
  moves.pop();
}
