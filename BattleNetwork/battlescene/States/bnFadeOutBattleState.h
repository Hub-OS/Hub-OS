#pragma once

#include "../bnBattleSceneState.h"

// modes for fading out style
enum class FadeOut : int {
    white = 0,
    black
};

/* 
    \brief This structure accepts the battle scene as input and asks to quit
    This is a dummy state because the constructor asks the scene in return to use its 
    Quit() routine
*/
class FadeOutBattleState final : public BattleSceneState {
    FadeOut mode;

public:
    FadeOutBattleState(const FadeOut& mode);

    void onStart() override;
    void onEnd() override;
    void onUpdate(double);
    void onDraw(sf::RenderTexture&);
};