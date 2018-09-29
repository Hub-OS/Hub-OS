#pragma once

#include "bnEaseFunctions.h"

class Mob;
class Player;

class BattleScene {
public:
  // Returns 0 for LOSE, 1 for WIN, 2 for TIE, -1 for EXIT_FAILURE
  static int Run(Player* player, Mob* mob);
};