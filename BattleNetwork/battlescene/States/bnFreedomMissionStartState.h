#pragma once

#include "../bnBattleSceneState.h"
#include "../../frame_time_t.h"

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>


class Player;

struct FreedomMissionStartState final : public BattleSceneState {
  uint8_t maxTurns{};
  frame_time_t preBattleLength{ frames(60) }; 
  frame_time_t startupDelay{ frames(0) }; /*!< Animation delay*/
  sf::Sprite battleStart; /*!< "Battle Start" graphic */
  swoosh::Timer battleStartTimer; /*!< How long the start graphic should stay on screen */
  sf::Vector2f battleStartPos; /*!< Position of battle pre/post graphic on screen */
  std::vector<Player*>& tracked;
  FreedomMissionStartState(std::vector<Player*>& tracked, uint8_t maxTurns);

  void SetStartupDelay(frame_time_t frames);
  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsFinished();
};
