#include "bnScriptedPlayerForm.h"
#include "bnScriptedPlayer.h"
#include "bnScriptedCardAction.h"
#include "bnScopedWrapper.h"
#include "../bnSolHelpers.h"

ScriptedPlayerForm::ScriptedPlayerForm()
{
}

ScriptedPlayerForm::~ScriptedPlayerForm()
{
}

void ScriptedPlayerForm::OnUpdate(double elapsed, std::shared_ptr<Player> _)
{
  if (!update_func.valid()) return;

  auto wrappedSelf = ScopedWrapper(*this);
  auto result = CallLuaCallback(update_func, wrappedSelf, elapsed, WeakWrapper(playerWeak));

  if (result.is_error()) {
    Logger::Log(LogLevel::critical, result.error_cstr());
  }
}

void ScriptedPlayerForm::OnActivate(std::shared_ptr<Player> _)
{
  if (!on_activate_func.valid()) return;

  auto wrappedSelf = ScopedWrapper(*this);
  auto result = CallLuaCallback(on_activate_func, wrappedSelf, WeakWrapper(playerWeak));

  if (result.is_error()) {
    Logger::Log(LogLevel::critical, result.error_cstr());
  }
}

void ScriptedPlayerForm::OnDeactivate(std::shared_ptr<Player> _)
{
  if (!on_deactivate_func.valid()) return;

  auto wrappedSelf = ScopedWrapper(*this);
  auto result = CallLuaCallback(on_deactivate_func, wrappedSelf, WeakWrapper(playerWeak));

  if (result.is_error()) {
    Logger::Log(LogLevel::critical, result.error_cstr());
  }
}

std::shared_ptr<CardAction> ScriptedPlayerForm::GenerateCardAction(sol::object& function, const std::string& functionName)
{
  auto result = CallLuaCallback(function, WeakWrapper(playerWeak));

  if(result.is_error()) {
    Logger::Log(LogLevel::critical, result.error_cstr());
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
      Logger::Logf(LogLevel::warning, "Lua function \"%s\" didn't return a CardAction.", functionName.c_str());
    }
  }

  return nullptr;
}

std::shared_ptr<CardAction> ScriptedPlayerForm::OnChargedBusterAction(std::shared_ptr<Player> _)
{
  if (!charged_attack_func.valid()) {
    // prevent error message for nil function, just return nullptr
    return nullptr;
  }

  std::shared_ptr<CardAction> result = GenerateCardAction(charged_attack_func, "charged_attack_func");

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::weapon);
  }

  return result;
}

std::shared_ptr<CardAction> ScriptedPlayerForm::OnSpecialAction(std::shared_ptr<Player> player)
{
  if (!special_attack_func.valid()) {
    // prevent error message for nil function, just return nullptr
    return nullptr;
  }

  std::shared_ptr<CardAction> result = GenerateCardAction(special_attack_func, "special_attack_func");

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::ability);
  }

  return result;
}

std::optional<frame_time_t> ScriptedPlayerForm::CalculateChargeTime(unsigned chargeLevel)
{
  if (!calculate_charge_time_func.valid()) {
    return {};
  }

  auto result = CallLuaCallbackExpectingValue<frame_time_t>(calculate_charge_time_func, chargeLevel);

  if (result.is_error()) {
    Logger::Log(LogLevel::critical, result.error_cstr());
    return {};
  }

  return result.value();
}

ScriptedPlayerFormMeta::ScriptedPlayerFormMeta(size_t index) : PlayerFormMeta(index)
{
  SetFormClass<ScriptedPlayerForm>();
}

PlayerForm* ScriptedPlayerFormMeta::BuildForm()
{
  ScriptedPlayerForm* form = static_cast<ScriptedPlayerForm*>(PlayerFormMeta::BuildForm());

  form->playerWeak = this->playerWeak;
  form->calculate_charge_time_func = this->calculate_charge_time_func;
  form->on_activate_func = this->on_activate_func;
  form->on_deactivate_func = this->on_deactivate_func;
  form->update_func = this->update_func;
  form->charged_attack_func = this->charged_attack_func;
  form->special_attack_func = this->special_attack_func;

  return form;
}
