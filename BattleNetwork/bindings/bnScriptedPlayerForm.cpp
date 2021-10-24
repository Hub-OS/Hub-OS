#include "bnScriptedPlayerForm.h"
#include "bnScriptedPlayer.h"
#include "bnScriptedCardAction.h"

ScriptedPlayerForm::ScriptedPlayerForm()
{
}

ScriptedPlayerForm::~ScriptedPlayerForm()
{
}

void ScriptedPlayerForm::OnUpdate(double elapsed, Player& player)
{
  if (!on_update) return;

  on_update(this, elapsed, static_cast<ScriptedPlayer&>(player));
}

void ScriptedPlayerForm::OnActivate(Player& player)
{
  if (!on_activate) return;
  on_activate(this, static_cast<ScriptedPlayer&>(player));
}

void ScriptedPlayerForm::OnDeactivate(Player& player)
{
  if (!on_deactivate) return;
  on_deactivate(this, static_cast<ScriptedPlayer&>(player));
}

CardAction* ScriptedPlayerForm::OnChargedBusterAction(Player& player)
{
  CardAction* result{ nullptr };

  if (!on_charge_action)
    return result;

  sol::object obj = on_charge_action(static_cast<ScriptedPlayer&>(player));

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

CardAction*ScriptedPlayerForm::OnSpecialAction(Player& player)
{
  CardAction* result{ nullptr };

  if (!on_special_action)
    return result;

  sol::object obj = on_special_action(static_cast<ScriptedPlayer&>(player));

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
      Logger::Log("Lua function \"create_special_attack\" didn't return a CardAction.");
    }
  }

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::ability);
  }

  return result;
}

frame_time_t ScriptedPlayerForm::CalculateChargeTime(unsigned chargeLevel)
{
  if (!on_calculate_charge_time) {
    return frames(60);
  }

  return on_calculate_charge_time(chargeLevel);
}

ScriptedPlayerFormMeta::ScriptedPlayerFormMeta(size_t index) : PlayerFormMeta(index)
{
  SetFormClass<ScriptedPlayerForm>();
}

PlayerForm* ScriptedPlayerFormMeta::BuildForm()
{
  ScriptedPlayerForm* form = static_cast<ScriptedPlayerForm*>(PlayerFormMeta::BuildForm());

  form->on_activate = this->on_activate;
  form->on_deactivate = this->on_deactivate;
  form->on_charge_action = this->on_charge_action;
  form->on_special_action = this->on_special_action;
  form->on_calculate_charge_time = this->on_calculate_charge_time;
  form->on_update = this->on_update;

  return form;
}
