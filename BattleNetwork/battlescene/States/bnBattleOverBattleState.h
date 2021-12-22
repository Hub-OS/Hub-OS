#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>

#include "bnBattleTextIntroState.h"
#include "../../bnText.h"
#include "../../bnResourceHandle.h"

class Player;

/*
    This state handles the battle end message that appears
*/
struct BattleOverBattleState final : public BattleTextIntroState {
  BattleOverBattleState();
  void onUpdate(double dt) override;
  void onStart(const BattleSceneState*);
};
