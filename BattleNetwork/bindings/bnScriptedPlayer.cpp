#ifdef BN_MOD_SUPPORT
#include "bnScriptedPlayer.h"
#include "bnScriptedCardAction.h"
#include "../bnCardAction.h"
#include "../bnSolHelpers.h"

ScriptedPlayer::ScriptedPlayer(sol::state& script) :
  script(script),
  Player()
{
  chargeEffect->setPosition(0, -30.0f);


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

  weakWrap = WeakWrapper(weak_from_base<ScriptedPlayer>());
}

void ScriptedPlayer::SetChargePosition(const float x, const float y)
{
  chargeEffect->setPosition({ x,y });
}

void ScriptedPlayer::SetFullyChargeColor(const sf::Color& color)
{
  chargeEffect->SetFullyChargedColor(color);
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
    if (obj.is<WeakWrapper<CardAction>>())
    {
      return obj.as<WeakWrapper<CardAction>>().Release();
    }
    else if (obj.is<WeakWrapper<ScriptedCardAction>>())
    {
      return obj.as<WeakWrapper<ScriptedCardAction>>().Release();
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

void ScriptedPlayer::OnUpdate(double elapsed)
{
  Player::OnUpdate(elapsed);

  if (update_func.valid())
  {
    auto result = CallLuaCallback(update_func, weakWrap, elapsed);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

#endif