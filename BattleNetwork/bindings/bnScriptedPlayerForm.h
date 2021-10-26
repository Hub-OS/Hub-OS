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
  void OnUpdate(double elapsed, std::shared_ptr<Player>) override;
  void OnActivate(std::shared_ptr<Player> player) override;
  void OnDeactivate(std::shared_ptr<Player> player) override;
  std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) override;
  std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) override;
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