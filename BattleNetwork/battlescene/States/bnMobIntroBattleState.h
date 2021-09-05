#pragma once

#include "../bnBattleSceneState.h"
#include <vector>

class Player;
class Mob;

/* 
    \brief This state will loop and spawn one enemy at a time
*/
struct MobIntroBattleState final : public BattleSceneState {
  std::vector<std::shared_ptr<Player>> tracked;
  Mob* mob{ nullptr };

  void onUpdate(double elapsed) override;
  void onEnd(const BattleSceneState* last) override;
  void onStart(const BattleSceneState* next) override;
  void onDraw(sf::RenderTexture&);

  const bool IsOver();
  MobIntroBattleState(Mob* mob, std::vector<std::shared_ptr<Player>> tracked);
};
