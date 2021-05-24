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
  long long preBattleLength{ 1000000 }; /*!< In microseconds */
  long long startupDelay{ 0 }; /*!< Animation delay measured in microseconds*/
  sf::Sprite battleStart; /*!< "Battle Start" graphic */
  swoosh::Timer battleStartTimer; /*!< How long the start graphic should stay on screen */
  sf::Vector2f battleStartPos; /*!< Position of battle pre/post graphic on screen */
  std::vector<Player*>& tracked;
  BattleStartBattleState(std::vector<Player*>& tracked);

  void SetStartupDelay(long long microseconds);
  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsFinished();
};
