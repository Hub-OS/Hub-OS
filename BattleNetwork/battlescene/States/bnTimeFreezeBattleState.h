#pragma once

#include <Swoosh/Timer.h>

#include "../bnBattleSceneState.h"

/* 
    \brief This state governs transitions and combat rules while in TF
*/
struct TimeFreezeBattleState final : public BattleSceneState {
    swoosh::Timer summonTimer; /*!< Timer for TFC label to appear at top */
    bool showSummonText; /*!< Whether or not TFC label should appear */
    double summonTextLength; /*!< How long TFC label should stay on screen */
    bool showSummonBackdrop; /*!< Dim screen and show new backdrop if applicable */
    double showSummonBackdropLength; /*!< How long the dim should last */
    double showSummonBackdropTimer; /*!< If > 0, the state is in effect else change state */

    bool IsOver();
};
