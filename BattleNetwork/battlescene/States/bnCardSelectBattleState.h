#include "../bnBattleSceneState.h"

/*
    \brief This state will move the cust GUI and allow the player to select new cards
*/
struct CardSelectBattleState final : public BattleSceneState {
    bool OKIsPressed();
};