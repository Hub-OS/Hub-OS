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
  /*
  Program Advance + labels
  */
  PA& programAdvance; /*!< PA object loads PA database and returns matching PA card from input */
  PA::Steps paSteps; /*!< Matching steps in a PA */
  bool isPAComplete{ true }; /*!< Flag if PA state is complete */
  int hasPA{ -1 }; /*!< If -1, no PA found, otherwise is the start of the PA in the current card list */
  int paStepIndex{ -1 }; /*!< Index in the PA list */

  float listStepCooldown; /*!< Remaining time inbetween PA list items */
  float listStepCounter; /*!< Max time inbetween PA list items */
  sf::Sprite programAdvanceSprite; /*!< Sprite for "ProgramAdvanced" graphic */
  
  // Selection input delays
  double maxCardSelectInputCooldown; /*!< When interacting with Card Cust GUI API, delay input */
  double heldCardSelectInputCooldown; /*!< When holding the directional inputs, when does the sticky key effect trigger*/
  double cardSelectInputCooldown; /*!< Time remaining with delayed input */
public:
    bool OKIsPressed();
};