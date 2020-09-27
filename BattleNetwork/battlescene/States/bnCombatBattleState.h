#include "../bnBattleSceneState.h"

/*
    \brief This state will govern combat rules
*/
struct CombatBattleState final : public BattleSceneState {
    const bool HasTimeFreeze() const;
    const bool IsCombatOver() const;
    const bool IsCardGaugeFull();
};