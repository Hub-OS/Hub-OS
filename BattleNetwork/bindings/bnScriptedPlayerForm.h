#pragma once
#ifdef BN_MOD_SUPPORT

#include <functional>
#include "../bnPlayerForm.h"
#include "dynamic_object.h"

class ScriptedPlayer;

class ScriptedPlayerForm final : public PlayerForm, public dynamic_object {
  public:
  ScriptedPlayerForm();
  ~ScriptedPlayerForm();
  void OnUpdate(double elapsed, Player&) override;
  void OnActivate(Player& player) override;
  void OnDeactivate(Player& player) override;
  CardAction* OnChargedBusterAction(Player&) override;
  CardAction* OnSpecialAction(Player&) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;

  std::function<void(ScriptedPlayerForm*, double, ScriptedPlayer&)> on_update;
  std::function<void(ScriptedPlayerForm*, ScriptedPlayer&)> on_activate, on_deactivate;
  std::function<sol::object(ScriptedPlayer&)> on_charge_action, on_special_action;
  std::function<frame_time_t(unsigned)> on_calculate_charge_time;
};

class ScriptedPlayerFormMeta : public PlayerFormMeta {
public:
  ScriptedPlayerFormMeta(size_t index);

  PlayerForm* BuildForm() override;

  std::function<void(ScriptedPlayerForm*, double, ScriptedPlayer&)> on_update;
  std::function<void(ScriptedPlayerForm*, ScriptedPlayer&)> on_activate, on_deactivate;
  std::function<sol::object(ScriptedPlayer&)> on_charge_action, on_special_action;
  std::function<frame_time_t(unsigned)> on_calculate_charge_time;
};
#endif