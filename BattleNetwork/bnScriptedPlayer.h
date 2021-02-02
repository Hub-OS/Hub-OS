/*! \brief scriptable navi
 * 
 * Uses callback functions defined in an external file to configure
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
#include "bnScriptResourceManager.h"

using sf::IntRect;

class ScriptedPlayer : public Player {
  sol::state& script;

public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  ScriptedPlayer(sol::state& script);

  const float GetHeight() const;
  AnimationComponent& GetAnimationComponent();

  CardAction* OnExecuteSpecialAction() override final;
  CardAction* OnExecuteBusterAction() override final;
  CardAction* OnExecuteChargedBusterAction() override final;
};