/*! \brief Roll playable net navi
 * 
 * Sets health to 900, name to "Roll", and enables Float Shoe
 */

#pragma once

#include "bnPlayer.h"
#include "bnPlayerState.h"
#include "bnTextureType.h"
#include "bnPlayerHealthUI.h"
#include "bnChargeEffectSceneNode.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
#include "bnPlayerHitState.h"
#include "bnNaviRegistration.h"

using sf::IntRect;

class Roll : public Player {
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  Roll();

  virtual const float GetHitHeight() const;
  ChipAction* ExecuteBusterAction() final;
  ChipAction* ExecuteChargedBusterAction() final;
};