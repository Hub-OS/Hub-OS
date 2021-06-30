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

  script["battle_init"](*this);
    
  animationComponent->Reload();
  CreateMoveAnimHash();
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

CardAction * ScriptedPlayer::OnExecuteSpecialAction()
{
  sol::object obj = script["execute_special_attack"](*this);

  if (obj.valid()) {
          // Removing the cast to unique_ptr as they are being passed up as raw pointers.
          // They were initially passed up as unique_ptr types but that was causing issues with
          //  retrieving them when they were a derived type (i.e. ScriptedCardAction),
          //  but then they had the unique_ptr stripped immediately after being retrieved from Lua anyway.
      // auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      // return ptr.release();

      if (obj.is<CardAction>())
      {
          auto& ptr = obj.as<CardAction&>();
          return &ptr;
      }

      Logger::Log("Lua function \"execute_special_attack\" didn't return a CardAction.");
  }

  return nullptr;
}

CardAction* ScriptedPlayer::OnExecuteBusterAction()
{
  sol::object obj = script["execute_buster_attack"](*this);

  if (obj.valid()) {
          // See above.
      // auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      // return ptr.release();
      if (obj.is<CardAction>())
      {
          auto& ptr = obj.as<CardAction&>();
          return &ptr;
      }

      Logger::Log("Lua function \"execute_buster_attack\" didn't return a CardAction.");
  }

  return nullptr;
}

CardAction* ScriptedPlayer::OnExecuteChargedBusterAction()
{
    // Pass along the ScriptedPlayer* reference so that 
  sol::object obj = script["execute_charged_attack"](*this);

  if (obj.valid()) {
          // See above.
      // auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      // return ptr.release();

      if (obj.is<CardAction>())
      {
          auto& ptr = obj.as<CardAction&>();
          return &ptr;
      }

      Logger::Log("Lua function \"execute_charged_attack\" didn't return a CardAction.");
  }

  return nullptr;
}

void ScriptedPlayer::OnUpdate(double _elapsed)
{
    Player::OnUpdate( _elapsed );

    script["on_update"](*this, _elapsed);
}

#endif