#pragma once

#include "../bnBattleSceneState.h"

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>
/*
    \brief This state handles the battle start message that appears

    depending on the type of battle (Round-based PVP? PVE?)
*/

class Player;

struct BattleStartBattleState final : public BattleSceneState {
    sf::Sprite battleStart; /*!< "Battle Start" graphic */
    swoosh::Timer battleStartTimer; /*!< How long the start graphic should stay on screen */
    sf::Vector2f battleStartPos; /*!< Position of battle pre/post graphic on screen */
    std::vector<Player*> tracked;
    BattleStartBattleState(std::vector<Player*> tracked);

    void onStart() override;
    void onUpdate(double elapsed) override;

    bool IsFinished();
};
