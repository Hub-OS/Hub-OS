#ifdef BN_MOD_SUPPORT
#include "bnScriptedPlayer.h"
#include "../bnCardAction.h"
#include "bnScriptedCardAction.h"

ScriptedPlayer::ScriptedPlayer(sol::state& script) :
  script(script),
  Player()
{
  chargeEffect.setPosition(0, -30.0f);


  SetLayer(0);
  SetTeam(Team::red);

  script["player_init"](*this);

  animationComponent->Reload();
  FinishConstructor();
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

void ScriptedPlayer::SetAnimation(const std::string& path)
{
  animationComponent->SetPath(path);
}

const float ScriptedPlayer::GetHeight() const
{
  return height;
}

Animation& ScriptedPlayer::GetAnimationObject()
{
  return animationComponent->GetAnimationObject();
}

Battle::Tile* ScriptedPlayer::GetCurrentTile() const
{
  return GetTile();
}

CardAction* ScriptedPlayer::OnExecuteSpecialAction()
{
  CardAction* result{ nullptr };

  sol::object obj = script["create_special_attack"](*this);

  if (obj.valid()) {
    if (obj.is<std::unique_ptr<CardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      result = ptr.release();
    }else if (obj.is<std::unique_ptr<ScriptedCardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<ScriptedCardAction>&>();
      result = ptr.release();
    }
    else {
      Logger::Log("Lua function \"create_special_attack\" didn't return a CardAction.");
    }
  }

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::ability);
  }

  return result;
}

CardAction* ScriptedPlayer::OnExecuteBusterAction()
{
  CardAction* result{ nullptr };

  sol::object obj = script["create_normal_attack"](*this);

  if (obj.valid()) {
    if (obj.is<std::unique_ptr<CardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      result = ptr.release();
    }
    else if (obj.is<std::unique_ptr<ScriptedCardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<ScriptedCardAction>&>();
      result = ptr.release();
    }
    else {
      Logger::Log("Lua function \"create_normal_attack\" didn't return a CardAction.");
    }
  }

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::weapon);
  }

  return result;
}

CardAction* ScriptedPlayer::OnExecuteChargedBusterAction()
{
  CardAction* result{ nullptr };

  sol::object obj = script["create_charged_attack"](*this);

  if (obj.valid()) {
    if (obj.is<std::unique_ptr<CardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      result = ptr.release();
    }
    else if (obj.is<std::unique_ptr<ScriptedCardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<ScriptedCardAction>&>();
      result = ptr.release();
    }
    else {
      Logger::Log("Lua function \"create_charged_attack\" didn't return a CardAction.");
    }
  }

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::weapon);
  }

  return result;
}

void ScriptedPlayer::OnUpdate(double _elapsed)
{
  Player::OnUpdate(_elapsed);

  if (on_update_func) {
    on_update_func(*this, _elapsed);
  }
}

#endif