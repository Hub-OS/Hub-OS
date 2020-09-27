#include "bnCombatBattleState.h"

const bool CombatBattleState::HasTimeFreeze() const {
    return true;
}

const bool CombatBattleState::IsCombatOver() const {
    return true;
}

const bool CombatBattleState::IsCardGaugeFull() {
    return true;
}