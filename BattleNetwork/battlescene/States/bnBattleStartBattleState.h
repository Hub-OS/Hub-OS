#include "../bnBattleSceneState.h"
/*
    \brief This state handles the battle start message that appears

    depending on the type of battle (Round-based PVP? PVE?)
*/
struct BattleStartState final : public BattleSceneState {
    bool IsFinished() {
        return true;
    }
};
