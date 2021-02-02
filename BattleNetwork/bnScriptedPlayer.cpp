#include "bnScriptedPlayer.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnLogger.h"
#include "bnBusterCardAction.h"

ScriptedPlayer::ScriptedPlayer(sol::state& script) : 
  script(script),
  Player()
{
  chargeEffect.setPosition(0, -30.0f);
  chargeEffect.SetFullyChargedColor(sf::Color::Cyan);

  SetLayer(0);
  SetTeam(Team::red);

  script["load_player"](this);

  animationComponent->Reload();
}

const float ScriptedPlayer::GetHeight() const
{
  return 140.0f;
}

AnimationComponent& ScriptedPlayer::GetAnimationComponent()
{
  return *animationComponent;
}

CardAction * ScriptedPlayer::OnExecuteSpecialAction()
{
  return Player::OnExecuteSpecialAction();
}

CardAction* ScriptedPlayer::OnExecuteBusterAction()
{
  return new BusterCardAction(*this, false, 1*GetAttackLevel());
}

CardAction* ScriptedPlayer::OnExecuteChargedBusterAction()
{
  return new BusterCardAction(*this, true, 10*GetAttackLevel());
}