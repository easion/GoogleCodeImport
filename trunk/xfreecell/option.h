#ifndef Option_H
#define Option_H

#include "widget/widget.h"

class Option : public NSFrame, public NSButtonListener {
public:
  Option();
  ~Option() { writePrefs(); }

  static void parse(int, char**);
  void waitForEvent();
  void buttonAction(const gi_msg_t&, void*);

  static int speedup() { return _speedup; }
  static bool queryWindow() { return _queryWindow; }
  static bool roundCard() { return _roundCard; }
  static bool animation() { return _animation; }
  static bool msSeed() { return _msSeed; }

private:
  void readPrefs();
  void writePrefs();

  string saveFile;

  static int _speedup;
  static bool _queryWindow;
  static bool _roundCard;
  static bool _animation;
  static bool _msSeed;

  bool exitPressed;
  bool okPressed;

  NSVContainer mainCon;
  NSHContainer speedCon;
  NSLabel speedLabel;
  NSTextField speedTF;
  NSHContainer togglesCon;
  NSToggleButton anim, query, ms;
  NSHContainer replyCon;
  NSButton okButton, cancelButton;
};

#endif
