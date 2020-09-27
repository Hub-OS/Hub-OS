#include "../bnBattleSceneState.h"

/*
    This state spells out the combo steps
 */
struct CardComboBattleState final : public BattleSceneState {
    bool IsDone();
};