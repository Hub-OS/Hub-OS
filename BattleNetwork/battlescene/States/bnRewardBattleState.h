#pragma once

#include "../bnBattleSceneState.h"

class BattleResults;

/*
    \brief This state rewards the player
*/
struct RewardBattleState final : public BattleSceneState {
    BattleResults* battleResults; /*!< BR modal that pops up when player wins */

    bool OKIsPressed();
};