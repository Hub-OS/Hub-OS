#pragma once

#include "../bnBattleSceneState.h"

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>
/*
    \brief This state handles the battle start message that appears

    depending on the type of battle (Round-based PVP? PVE?)
*/

class Player;

struct BattleStartBattleState final : public BattleSceneState {
  double preBattleLength{ 1000.0 }; /*!< In milliseconds */
  sf::Sprite battleStart; /*!< "Battle Start" graphic */
  swoosh::Timer battleStartTimer; /*!< How long the start graphic should stay on screen */
  sf::Vector2f battleStartPos; /*!< Position of battle pre/post graphic on screen */
  std::vector<Player*> tracked;
  BattleStartBattleState(std::vector<Player*> tracked);

  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsFinished();
};
