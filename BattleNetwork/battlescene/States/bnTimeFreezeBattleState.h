#include "../bnBattleSceneState.h"

/* 
    \brief This state governs transitions and combat rules while in TF
*/
struct TimeFreezeBattleState final : public BattleSceneState {
    bool IsOver();
};
