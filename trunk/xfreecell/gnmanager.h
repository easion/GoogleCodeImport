#ifndef GNManager_H
#define GNManager_H

#include <string>
#if 0
#include <hash_set>
#else
#include <hash_set.h>
#endif

using namespace std;

class GameNumberManager {
public:
  GameNumberManager();

  bool alreadyPlayed(int num) { return alreadyWon(num) || alreadyLost(num); }

  void addWonGame(int);
  void addLostGame(int);
  void writeFiles();
  int randomLostGame();

private:
  bool alreadyWon(int);
  bool alreadyLost(int);

  void readFiles();
  void readFile(const string&, hash_set<int>*);
  void writeFile(const string&, hash_set<int>*);

  string lostGameFile, wonGameFile;
  
  hash_set<int> wonGames;
  hash_set<int> lostGames;

  string msLostGameFile, msWonGameFile;

  hash_set<int> msWonGames;
  hash_set<int> msLostGames;
};

#endif
