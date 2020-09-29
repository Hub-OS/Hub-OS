#pragma once

#include "../bnBattleSceneState.h"

// modes for fading out style
enum class FadeOut : int {
    white = 0,
    black
} fadeOutMode;

/* 
    \brief This structure accepts the battle scene as input and asks to quit
*/
class FadeOutBattleState final : public BattleSceneState {
    FadeOut mode;

public:
    FadeOutBattleState(const FadeOut& mode);

    void onStart() override;
};