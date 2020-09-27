#include "../bnBattleSceneState.h"

/* 
    \brief This state will loop and spawn one enemy at a time
*/
struct MobIntroBattleState final : public BattleSceneState {
    const bool IsOver();
};
