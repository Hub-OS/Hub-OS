/*! \brief Loads card meta and action data from a lua script
 */
#pragma once
#ifdef ONB_MOD_SUPPORT
#include "bnScriptedCardAction.h"
#include "bnWeakWrapper.h"
#include "../../bnCard.h"
#include "../bnSolHelpers.h"
#include "../bnCardPackageManager.h"

class ScriptedCard : public CardBuilderTrait {
  sol::state& script;

public:
  ScriptedCard(sol::state& script) : script(script) {

  }

  std::shared_ptr<CardAction> BuildCardAction(std::shared_ptr<Character> user, const Battle::Card::Properties& props) override {
    auto functionResult = CallLuaFunctionExpectingValue<WeakWrapper<ScriptedCardAction>>(script, "card_create_action", WeakWrapper(user), props);

    if (functionResult.is_error()) {
      Logger::Log(LogLevel::critical, functionResult.error_cstr());
      return nullptr;
    }

    WeakWrapper<ScriptedCardAction> wrappedCardAction = functionResult.value();
    std::shared_ptr<ScriptedCardAction> cardAction = wrappedCardAction.Release();

    if (cardAction) {
      cardAction->SetLockoutGroup(CardAction::LockoutGroup::card);
    } else {
      Logger::Log(LogLevel::info, "Lua function \"card_create_action\" didn't return a CardAction.");
    }

    return cardAction;
  }

  void OnUpdate(Battle::Card::Properties& props, double elapsed) override {
    // cards can optionally have this function
    sol::object possible_func = script["card_update"];
    if (possible_func.get_type() != sol::type::function) return;

    auto functionResult = CallLuaFunction(script, "card_update", std::ref(props), elapsed);

    if (functionResult.is_error()) {
      Logger::Log(LogLevel::critical, functionResult.error_cstr());
    }
  }
};

#endif