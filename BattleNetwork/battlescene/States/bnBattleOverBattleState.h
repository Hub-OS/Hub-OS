#pragma once

#include "../bnBattleSceneState.h"

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>

/*
    This state handles the battle end message that appears
*/
struct BattleOverBattleState final : public BattleSceneState {
  double postBattleLength{ 500 }; /*!< In milliseconds */
  sf::Sprite battleEnd;   /*!< "Enemy Deleted" graphic */
  swoosh::Timer battleEndTimer; /*!< How long the end graphic should stay on screen */
  sf::Vector2f battleOverPos; /*!< Position of battle pre/post graphic on screen */

  BattleOverBattleState();
  void onStart() override;
  void onEnd() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsFinished();
};
