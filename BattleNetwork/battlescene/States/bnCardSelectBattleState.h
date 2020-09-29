#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnPA.h"

#include <SFML/Graphics/Sprite.hpp>

// modals like card cust and battle reward slide in 12px per frame for 10 frames. 60 frames = 1 sec
// modal slide moves 120px in 1/6th of a second
// Per 1 second that is 6*120px in 6*1/6 of a sec = 720px in 1 sec
#define MODAL_SLIDE_PX_PER_SEC 720.0f

/*
    \brief This state will move the cust GUI and allow the player to select new cards
*/
class CardSelectBattleState final : public BattleSceneState {
  // Selection input delays
  double maxCardSelectInputCooldown; /*!< When interacting with Card Cust GUI API, delay input */
  double heldCardSelectInputCooldown; /*!< When holding the directional inputs, when does the sticky key effect trigger*/
  double cardSelectInputCooldown; /*!< Time remaining with delayed input */
public:
    bool OKIsPressed();
    CardSelectBattleState();
};