#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>

#include "general.h"
#include "subwindows.h"

extern gi_window_id_t gameWindow;


//QueryWindow
QueryWindow::QueryWindow(const char* queryLabel, const char* leftLabel, const char* rightLabel)
  : label(queryLabel), leftButton(leftLabel, this), rightButton(rightLabel, this),
    con(300, 100), mainCon(300, 150)
{
  con.add(&leftButton); con.add(&rightButton);
  con.reallocate();
  
  mainCon.add(&label); mainCon.add(&con);
  mainCon.reallocate();

  container(&mainCon);

  _left = false;
  _right = false;

  borderWidth(1);
}

void QueryWindow::buttonAction(const gi_msg_t& ev, void*)
{
  //if (ev.xany.window == leftButton.window()) {
  if (ev.ev_window == leftButton.window()) {
    _left = true;
    _right = false;
  //} else if (ev.xany.window == rightButton.window()) {
  } else if (ev.ev_window == rightButton.window()) {
    _left = false;
    _right = true;
  }
}

void QueryWindow::waitForEvent()
{
  _left = false;
  _right = false;

  map();
  gi_raise_window( window());

  gi_msg_t ev;
  NSWindow* win;

  while (!_left && !_right) {
    gi_get_message(&ev, MSG_ALWAYS_WAIT);
    //if ((win = NSWindow::windowToNSWindow(ev.xany.window)) != 0) 
    if ((win = NSWindow::windowToNSWindow(ev.ev_window)) != 0) 
      win->dispatchEvent(ev);
  }
  
  unmap();
}

//Score
extern int errno;

const int winScore = 10;
const int defeatScore = 10;
const int undoScore = 1;
const int continuousScore = 1;

ScoreWindow::ScoreWindow()
  : exitButton("exit", this), 
    winLabel(""),
    defeatLabel(""),
    winpLabel(""),
    defeatpLabel(""),
    cwinLabel(""),
    cdefeatLabel(""),
    undoLabel(""),
    totalLabel(""),
    mainCon(300, 400)
{
  mainCon.add(&winLabel); 
  mainCon.add(&defeatLabel);
  mainCon.add(&winpLabel);
  mainCon.add(&defeatpLabel);
  mainCon.add(&cwinLabel);
  mainCon.add(&cdefeatLabel);
  mainCon.add(&undoLabel);
  mainCon.add(&totalLabel);
  mainCon.add(&exitButton);
  mainCon.reallocate();
    
  container(&mainCon);
    
  _wins = _defeats = 0;
  _continuousWins = 0;
  _continuousDefeats = 0;
  _previousWon = false;
  _undos = 0;
  _totalScore = 0;
  exitPressed = false;

  char* home = getenv("HOME");

  if (home == NULL) {
    fprintf(stderr, "Cannot get $HOME. Assuming I am at home directory now.\n");
    saveFile = ".xfreecell";
  } else {
    saveFile = home;
    saveFile += "/.xfreecell";
  }

  DIR* dir = opendir(saveFile.c_str());

  if (dir == NULL) {
    switch (errno) {
    case ENOENT:
      fprintf(stderr, "Directory %s not found. Creating.\n", saveFile.c_str());
      mkdir(saveFile.c_str(), 0755);
      break;
    case ENOTDIR:
      fprintf(stderr, "%s must be directory.\n", saveFile.c_str());
      exit(1);
      break;
    default:
      perror("ScoreWindow::ScoreWindow()");
      exit(1);
    }
  } else
    closedir(dir);
  saveFile += "/score";

  readScore();
  borderWidth(1);
}

void ScoreWindow::incWins()
{
  if (_previousWon == false) 
    _previousWon = true;

  _wins++;
  _totalScore += winScore + _continuousWins * continuousScore;
  _continuousWins++;
  _continuousDefeats = 0;
  writeScore();
}

void ScoreWindow::incDefeats()
{
  _previousWon = false;
  _defeats++;
  _totalScore -= defeatScore + _continuousDefeats * continuousScore;
  _continuousWins = 0;
  _continuousDefeats++;
  writeScore();
}

void ScoreWindow::incUndos()
{
  _undos++;
  _totalScore -= undoScore;
  writeScore();
}

void ScoreWindow::readScore()
{
  FILE* fp = fopen(saveFile.c_str(), "r");
  const int lineLength = 30;
  char line[lineLength];

  if (fp == NULL) {
    if (errno == ENOENT) 
      fprintf(stderr, "%s does not exist. Creating.\n", saveFile.c_str());
    writeScore();
    return;
  }

  if (fgets(line, lineLength, fp) == NULL) goto ERROR;
  if (sscanf(line, "Wins = %d", &_wins) != 1) goto ERROR;

  if (fgets(line, lineLength, fp) == NULL) goto ERROR;
  if (sscanf(line, "Defeats = %d", &_defeats) != 1) goto ERROR;

  if (fgets(line, lineLength, fp) == NULL) goto ERROR;
  if (sscanf(line, "Continuous Wins = %d", &_continuousWins) != 1) goto ERROR;

  if (fgets(line, lineLength, fp) == NULL) goto ERROR;
  if (sscanf(line, "Continuous Defeats = %d", &_continuousDefeats) != 1) goto ERROR;

  int tmp; 
  if (fgets(line, lineLength, fp) == NULL) goto ERROR;
  if (sscanf(line, "Previous Won = %d", &tmp) != 1) goto ERROR;
  _previousWon = (tmp == 0 ? false : true);

  if (fgets(line, lineLength, fp) == NULL) goto ERROR;
  if (sscanf(line, "Undos = %d", &_undos) != 1) goto ERROR;

  if (fgets(line, lineLength, fp) == NULL) goto ERROR;
  if (sscanf(line, "Total Score = %d", &_totalScore) != 1) goto ERROR;
  
  fclose(fp);
  return;

 ERROR:
  fprintf(stderr, "Error encountered while reading %s\n", saveFile.c_str());
  fclose(fp);
}

void ScoreWindow::writeScore() const
{
  FILE* fp = fopen(saveFile.c_str(), "w+");

  if (fp == NULL) {
    fprintf(stderr, "Cannot open %s\n", saveFile.c_str());
    perror("ScoreWindow::writeScore()");
    return;
  }

  fprintf(fp, "Wins = %d\n", _wins);
  fprintf(fp, "Defeats = %d\n", _defeats);
  fprintf(fp, "Continuous Wins = %d\n", _continuousWins);
  fprintf(fp, "Continuous Defeats = %d\n", _continuousDefeats);
  fprintf(fp, "Previous Won = %d\n", _previousWon);
  fprintf(fp, "Undos = %d\n", _undos);
  fprintf(fp, "Total Score = %d\n", _totalScore);

  fclose(fp);
  return;
}    

void ScoreWindow::waitForEvent()
{
  exitPressed = false;
  //XFlush(dpy);
  setLabels();

  map();
  gi_raise_window( window());

  gi_msg_t ev;
  NSWindow* win;
  while (!exitPressed) {
    gi_get_message(&ev, MSG_ALWAYS_WAIT);
    //win = NSWindow::windowToNSWindow(ev.xany.window);
    win = NSWindow::windowToNSWindow(ev.ev_window);
    if (win != 0) win->dispatchEvent(ev);
  }
  unmap();
}

void ScoreWindow::buttonAction(const gi_msg_t&, void*)
{
    exitPressed = true;
}

void ScoreWindow::setLabels()
{
  static char line[100];

  sprintf(line, "Wins: %d", _wins);
  winLabel.label(line);

  sprintf(line, "Defeats: %d", _defeats);
  defeatLabel.label(line);

  sprintf(line, "Win Percentage: %2.0f%%",
	  _wins + _defeats == 0 ? 0 : ((float) _wins) / (_wins + _defeats) * 100);
  winpLabel.label(line);

  sprintf(line, "Defeat Percentage: %2.0f%%",
  	  _wins + _defeats == 0 ? 0 : ((float) _defeats) / (_wins + _defeats) * 100);
  defeatpLabel.label(line);

  sprintf(line, "Continuous Wins: %d", _continuousWins);
  cwinLabel.label(line);

  sprintf(line, "Continuous Defeats: %d", _continuousDefeats);
  cdefeatLabel.label(line);

  sprintf(line, "Undos: %d", _undos);
  undoLabel.label(line);

  sprintf(line, "Total Score: %d", _totalScore);
  totalLabel.label(line);
  
  mainCon.reallocate();
}    

// About
AboutWindow::AboutWindow()
  : version(VersionStr),
    copyright("Copyright 1999 by Nakayama Shintaro"),
    homepage("http://www2.giganet.net/~nakayama"),
    okButton(" Ok ", this),
    labelCon(350, 100),
    mainCon(350, 150)
{
  okPressed = false;

  labelCon.add(&version);
  labelCon.add(&copyright);
  labelCon.add(&homepage);
  labelCon.reallocate();

  mainCon.add(&labelCon);
  mainCon.add(&okButton);
  mainCon.reallocate();

  okButton.y(110);

  container(&mainCon);
}

void AboutWindow::buttonAction(const gi_msg_t&, void*)
{
  okPressed = true;
}

void AboutWindow::waitForEvent()
{
  okPressed = false;

  map();
  raise();

  gi_msg_t ev;

  while (!okPressed) {
    gi_get_message(&ev, MSG_ALWAYS_WAIT);
    NSDispatchEvent(ev);
  };

  unmap();
}

// ##### SeedWindow #####
SeedWindow::SeedWindow()
  : buttonCon(200, 50),
    mainCon(200, 120),
    label("Type game number"),
    okButton(" Ok ", this),
    cancelButton("Cancel", this),
    textField(150)
{
  buttonCon.add(&okButton);
  buttonCon.add(&cancelButton);
  buttonCon.reallocate();

  mainCon.add(&label);
  mainCon.add(&textField);
  mainCon.add(&buttonCon);
  mainCon.reallocate();

  container(&mainCon);

  _okPressed = false;
  _cancelPressed = false;
}

void SeedWindow::buttonAction(const gi_msg_t& ev, void*)
{
  //if (ev.xany.window == okButton.window())
  if (ev.ev_window == okButton.window())
    _okPressed = true;
  //else if (ev.xany.window == cancelButton.window())
  else if (ev.ev_window == cancelButton.window())
    _cancelPressed = true;
}

void SeedWindow::waitForEvent()
{
  _okPressed = false;
  _cancelPressed = false;

  map();
  raise();

  gi_msg_t ev;

  while (!_okPressed && !_cancelPressed) {
    NSNextEvent( &ev);
    NSDispatchEvent(ev);
  }

  unmap();
}
