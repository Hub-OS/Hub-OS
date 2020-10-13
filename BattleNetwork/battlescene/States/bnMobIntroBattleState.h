#pragma once

#include "../bnBattleSceneState.h"
#include <vector>

class Player;
class Mob;

/* 
    \brief This state will loop and spawn one enemy at a time
*/
struct MobIntroBattleState final : public BattleSceneState {
  std::vector<Player*> tracked;
  Mob* mob{ nullptr };

  void onUpdate(double elapsed) override;
  const bool IsOver();
  MobIntroBattleState(Mob* mob, std::vector<Player*> tracked);
};
