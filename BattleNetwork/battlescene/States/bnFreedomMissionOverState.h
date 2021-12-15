#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>

#include "../bnBattleSceneState.h"
#include "../../bnResourceHandle.h"

class Player;

/*
    This state handles the battle end message that appears
*/
struct FreedomMissionOverState final : public BattleSceneState {
  enum class Conditions : char {
    player_won_single_turn = 0,
    player_won_mutliple_turn,
    player_failed,
    player_deleted
  };

  Conditions context{ }; /*!< Display different message depending on the conditions */
  double postBattleLength{ 1000 }; /*!< In milliseconds */
  sf::Sprite battleEnd;   /*!< "Enemy Deleted" graphic */
  swoosh::Timer battleEndTimer; /*!< How long the end graphic should stay on screen */
  sf::Vector2f battleOverPos; /*!< Position of battle pre/post graphic on screen */

  FreedomMissionOverState();
  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsFinished();
};
