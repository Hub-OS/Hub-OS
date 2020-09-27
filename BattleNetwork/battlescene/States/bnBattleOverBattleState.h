#include "../bnBattleSceneState.h"

/*
    This state handles the battle end message that appears
*/
struct BattleOverBattleState final : public BattleSceneState {
    bool IsFinished() {
        return true;
    }
};
