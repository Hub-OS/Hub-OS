#pragma once

#include "../bnBattleSceneState.h"

// modes for fading out style
enum class FadeOut : int {
    white = 0,
    black
};

class Player;

/* 
    \brief This structure accepts the battle scene as input and asks to quit
    This is a dummy state because the constructor asks the scene in return to use its 
    Quit() routine
*/
class FadeOutBattleState final : public BattleSceneState {
    FadeOut mode;
    double wait{ 2 }; // in seconds
    std::vector<Player*> tracked;
public:
    FadeOutBattleState(const FadeOut& mode, std::vector<Player*> tracked);

    void onStart(const BattleSceneState* last) override;
    void onEnd(const BattleSceneState* next) override;
    void onUpdate(double);
    void onDraw(sf::RenderTexture&);
};