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

  std::shared_ptr<CardAction> BuildCardAction(std::shared_ptr<Character> user, const Battle::Card::Properties& props) override {
    auto functionResult = CallLuaFunctionExpectingValue<WeakWrapper<ScriptedCardAction>>(script, "card_create_action", WeakWrapper(user), props);

    if (functionResult.is_error()) {
      Logger::Log(functionResult.error_cstr());
      return nullptr;
    }

    auto wrappedCardAction = functionResult.value();
    auto cardAction = wrappedCardAction.Release();

    if (cardAction) {
      cardAction->SetLockoutGroup(CardAction::LockoutGroup::card);
    } else {
      Logger::Log("Lua function \"card_create_action\" didn't return a CardAction.");
    }

    return cardAction;
  }
};

#endif