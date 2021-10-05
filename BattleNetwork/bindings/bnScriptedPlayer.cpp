#ifdef BN_MOD_SUPPORT
#include "bnScriptedPlayer.h"
#include "bnScriptedCardAction.h"
#include "../bnCardAction.h"
#include "../bnSolHelpers.h"

ScriptedPlayer::ScriptedPlayer(sol::state& script) :
  script(script),
  Player()
{
  chargeEffect.setPosition(0, -30.0f);


  SetLayer(0);
  SetTeam(Team::red);
}

void ScriptedPlayer::Init() {
  Player::Init();

  auto initResult = CallLuaFunction(script, "player_init", WeakWrapper(weak_from_base<ScriptedPlayer>()));

  if (initResult.is_error()) {
    Logger::Log(initResult.error_cstr());
  }

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

std::shared_ptr<CardAction> ScriptedPlayer::GenerateCardAction(const std::string& functionName) {

  auto result = CallLuaFunction(script, functionName, WeakWrapper(weak_from_base<ScriptedPlayer>()));

  if(result.is_error()) {
    Logger::Log(result.error_cstr());
    return nullptr;
  }

  auto obj = result.value();

  if (obj.valid()) {
    if (obj.is<std::shared_ptr<CardAction>>())
    {
      return obj.as<std::shared_ptr<CardAction>>();
    }
    else if (obj.is<std::shared_ptr<ScriptedCardAction>>())
    {
      return obj.as<std::shared_ptr<ScriptedCardAction>>();
    }
    else {
      Logger::Logf("Lua function \"%s\" didn't return a CardAction.", functionName.c_str());
    }
  }

  return nullptr;
}

std::shared_ptr<CardAction> ScriptedPlayer::OnExecuteSpecialAction()
{
  std::shared_ptr<CardAction> result = GenerateCardAction("create_special_attack");

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::ability);
  }

  return result;
}

std::shared_ptr<CardAction> ScriptedPlayer::OnExecuteBusterAction()
{
  std::shared_ptr<CardAction> result = GenerateCardAction("create_normal_attack");

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::weapon);
  }

  return result;
}

std::shared_ptr<CardAction> ScriptedPlayer::OnExecuteChargedBusterAction()
{
  std::shared_ptr<CardAction> result = GenerateCardAction("create_charged_attack");

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::weapon);
  }

  return result;
}

void ScriptedPlayer::OnUpdate(double _elapsed)
{
  Player::OnUpdate(_elapsed);

  if (updateCallback) {
    try {
      auto player = WeakWrapper(weak_from_base<ScriptedPlayer>());
      updateCallback(player, _elapsed);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

#endif