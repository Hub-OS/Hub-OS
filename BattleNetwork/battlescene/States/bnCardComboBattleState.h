#pragma once

#include <Swoosh/Timer.h>

#include "../bnBattleSceneState.h"

/*
    This state spells out the combo steps
 */
struct CardComboBattleState final : public BattleSceneState {
  swoosh::Timer PAStartTimer; /*!< Time to scale the PA graphic */
  sf::Sprite programAdvanceSprite;
  double PAStartLength{ 0 }; /*!< Total time to animate PA */
  bool isPAComplete{ false };
  int hasPA{ -1 };
  int paStepIndex{ 0 };
  double listStepCooldown{ 0 };
  double listStepCounter{ 0 };

  CardComboBattleState();
  bool IsDone();
};