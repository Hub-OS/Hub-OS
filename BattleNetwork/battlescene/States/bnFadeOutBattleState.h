#pragma once

#include "../bnBattleSceneState.h"

// modes for fading out style
enum class FadeOut : int {
    white = 0,
    black
} fadeOutMode;

class IBattleScene; // forward declare

/* 
    \brief This structure accepts the battle scene as input and asks to quit
*/
class FadeOutBattleState final : public BattleSceneState {
    IBattleScene* bsPtr{nullptr};
    FadeOut mode;

public:
    FadeOutBattleState(IBattleScene* bsPtr, const FadeOut& mode);

    void onStart() override;
};