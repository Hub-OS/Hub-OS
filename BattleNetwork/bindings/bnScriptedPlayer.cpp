#include "bnScriptedPlayer.h"
#include "../bnCardAction.h"

ScriptedPlayer::ScriptedPlayer(sol::state& script) : 
  script(script),
  Player()
{
  chargeEffect.setPosition(0, -30.0f);


  SetLayer(0);
  SetTeam(Team::red);

  script["battle_init"](this);

  animationComponent->Reload();
}

void ScriptedPlayer::SetFullyChargeColor(const sf::Color& color)
{
  chargeEffect.SetFullyChargedColor(color);
}

void ScriptedPlayer::SetHeight(const float& height)
{
  this->height = height;
}

const float ScriptedPlayer::GetHeight() const
{
  return height;
}

AnimationComponent& ScriptedPlayer::GetAnimationComponent()
{
  return *animationComponent;
}

CardAction * ScriptedPlayer::OnExecuteSpecialAction()
{
  sol::object obj = script["execute_special_attack"](this);

  if (obj.valid()) {
    std::shared_ptr<CardAction> ptr = obj.as<std::shared_ptr<CardAction>>();
    return ptr.get();
  }

  return nullptr;
}

CardAction* ScriptedPlayer::OnExecuteBusterAction()
{
  sol::object obj = script["execute_buster_attack"](this);

  if (obj.valid()) {
    std::shared_ptr<CardAction> ptr = obj.as<std::shared_ptr<CardAction>>();
    return ptr.get();
  }

  return nullptr;
}

CardAction* ScriptedPlayer::OnExecuteChargedBusterAction()
{
  sol::object obj = script["execute_charged_attack"](this);

  if (obj.valid()) {
    std::shared_ptr<CardAction> ptr = obj.as<std::shared_ptr<CardAction>>();
    return ptr.get();
  }

  return nullptr;
}