#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnAnimation.h"

#include <SFML/Graphics/Sprite.hpp>

/*
    \brief This state handles transformations
*/
struct CharacterTransformaBattleState final : public BattleSceneState {
    Animation shineAnimation;
    sf::Sprite shine;
    int lastSelectedForm{ -1 };

    bool IsFinished() {
        return true;
    }
};