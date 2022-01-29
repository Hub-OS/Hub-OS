#pragma once
#ifdef BN_MOD_SUPPORT

#include <functional>
#include "../bnPlayerForm.h"
#include "dynamic_object.h"

class ScriptedPlayer;

class ScriptedPlayerForm final : public PlayerForm, public dynamic_object {
private:
  std::shared_ptr<CardAction> GenerateCardAction(sol::object& function, const std::string& functionName);

  public:
  ScriptedPlayerForm();
  ~ScriptedPlayerForm();
  void OnUpdate(double elapsed, std::shared_ptr<Player>) override;
  void OnActivate(std::shared_ptr<Player> player) override;
  void OnDeactivate(std::shared_ptr<Player> player) override;
  std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) override;
  std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) override;
  std::optional<frame_time_t> CalculateChargeTime(unsigned chargeLevel) override;

  std::weak_ptr<ScriptedPlayer> playerWeak;
  sol::object calculate_charge_time_func;
  sol::object on_activate_func;
  sol::object on_deactivate_func;
  sol::object update_func;
  sol::object charged_attack_func;
  sol::object special_attack_func;
};

class ScriptedPlayerFormMeta : public PlayerFormMeta {
public:
  ScriptedPlayerFormMeta(size_t index);

  PlayerForm* BuildForm() override;

  std::weak_ptr<ScriptedPlayer> playerWeak;
  sol::object calculate_charge_time_func;
  sol::object on_activate_func;
  sol::object on_deactivate_func;
  sol::object update_func;
  sol::object charged_attack_func;
  sol::object special_attack_func;
};
#endif