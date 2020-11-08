#pragma once

#include "../bnBattleSceneState.h"

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>

class Player;

/*
    This state handles the battle end message that appears
*/
struct BattleOverBattleState final : public BattleSceneState {
  double postBattleLength{ 1000 }; /*!< In milliseconds */
  sf::Sprite battleEnd;   /*!< "Enemy Deleted" graphic */
  swoosh::Timer battleEndTimer; /*!< How long the end graphic should stay on screen */
  sf::Vector2f battleOverPos; /*!< Position of battle pre/post graphic on screen */
  std::vector<Player*>& tracked;

  BattleOverBattleState(std::vector<Player*>& tracked);
  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsFinished();
};
