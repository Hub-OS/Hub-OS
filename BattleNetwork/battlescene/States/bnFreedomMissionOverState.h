#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>

#include "bnBattleTextIntroState.h"
#include "../../bnResourceHandle.h"

class Player;

/*
    This state handles the battle end message that appears
*/
struct FreedomMissionOverState final : public BattleTextIntroState {
  enum class Conditions : char {
    player_won_single_turn = 0,
    player_won_mutliple_turn,
    player_failed,
    player_deleted
  };

  Conditions context{ }; /*!< Display different message depending on the conditions */

  FreedomMissionOverState();
  void onStart(const BattleSceneState* last) override;
  void onUpdate(double elapsed) override;
};
