#ifdef BN_MOD_SUPPORT
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

void ScriptedPlayer::SetChargePosition(const float x, const float y)
{
  chargeEffect.setPosition({ x,y });
}

void ScriptedPlayer::SetFullyChargeColor(const sf::Color& color)
{
  chargeEffect.SetFullyChargedColor(color);
}

void ScriptedPlayer::SetHeight(const float height)
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
  Character& character = *this;
  sol::object obj = script["execute_special_attack"](character);

  if (obj.valid()) {
    auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
    return ptr.release();
  }

  return nullptr;
}

CardAction* ScriptedPlayer::OnExecuteBusterAction()
{
  Character& character = *this;
  sol::object obj = script["execute_buster_attack"](character);

  if (obj.valid()) {
    auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
    return ptr.release();
  }

  return nullptr;
}

CardAction* ScriptedPlayer::OnExecuteChargedBusterAction()
{
  Character& character = *this;
  sol::object obj = script["execute_charged_attack"](character);

  if (obj.valid()) {
    auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
    return ptr.release();
  }

  return nullptr;
}

#endif