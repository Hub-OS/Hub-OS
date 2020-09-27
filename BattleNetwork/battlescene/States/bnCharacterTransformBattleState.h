#include "../bnBattleSceneState.h

/*
    \brief This state handles transformations
*/
struct CharacterTransformaBattleState final : public BattleSceneState {
    bool IsFinished() {
        return true;
    }
};