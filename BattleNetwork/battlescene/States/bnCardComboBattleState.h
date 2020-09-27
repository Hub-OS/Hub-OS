#pragma once

#include <Swoosh/Timer.h>

#include "../bnBattleSceneState.h"

/*
    This state spells out the combo steps
 */
struct CardComboBattleState final : public BattleSceneState {
    swoosh::Timer PAStartTimer; /*!< Time to scale the PA graphic */
    double PAStartLength; /*!< Total time to animate PA */

    bool IsDone();
};