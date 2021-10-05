/*! \brief Loads card meta and action data from a lua script
 */
#pragma once
#ifdef BN_MOD_SUPPORT
#include "bnScriptedCardAction.h"
#include "../../bnCard.h"
#include "../bnSolHelpers.h"
#include "bnWeakWrapper.h"

class CardImpl;

class ScriptedCard : public CardImpl {
  sol::state& script;

public:
  ScriptedCard(sol::state& script) : script(script) {

  }

  std::shared_ptr<CardAction> BuildCardAction(std::shared_ptr<Character> user, Battle::Card::Properties& props) override {
    auto functionResult = CallLuaFunction(script, "card_create_action", WeakWrapper(user), props);

    if (functionResult.is_error()) {
      Logger::Log(functionResult.error_cstr());
      return nullptr;
    }

    std::shared_ptr<CardAction> result{ nullptr };

    auto obj = functionResult.value();

    if (obj.valid() && obj.is<std::shared_ptr<ScriptedCardAction>>())
    {
      result = obj.as<std::shared_ptr<ScriptedCardAction>>();
    }

    if (result) {
      result->SetLockoutGroup(CardAction::LockoutGroup::card);
    } else {
      Logger::Log("Lua function \"card_create_action\" didn't return a CardAction.");
    }

    return result;
  }
};

#endif