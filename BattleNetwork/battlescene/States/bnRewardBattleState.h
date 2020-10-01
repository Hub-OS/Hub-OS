#pragma once

#include "../bnBattleSceneState.h"

class BattleResults;
class Player;
class Mob;

/*
    \brief This state rewards the player
*/
struct RewardBattleState final : public BattleSceneState {
  BattleResults* battleResults{ nullptr }; /*!< modal that pops up when player wins */
  Player* player{ nullptr };
  Mob* mob{ nullptr };

  RewardBattleState(Player* player, Mob* mob);
  void onStart() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool OKIsPressed();
};