#pragma once

#include "bnBattleTextIntroState.h"

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>


class Player;

struct FreedomMissionStartState final : public BattleTextIntroState {
  uint8_t maxTurns{};
  FreedomMissionStartState(uint8_t maxTurns);

  void onStart(const BattleSceneState* last) override;
};
