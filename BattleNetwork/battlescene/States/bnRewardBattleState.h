#include "../bnBattleSceneState.h"

/*
    \brief This state rewards the player
*/
struct RewardBattleState final : public BattleSceneState {
    bool OKIsPressed();
};