/*! \brief Loads card meta and action data from a lua script
 */
#pragma once
#ifdef BN_MOD_SUPPORT
#include "bnScriptedCardAction.h"
#include "../../bnCard.h"

#include <sol/sol.hpp>

class CardImpl;

class ScriptedCard : public CardImpl {
  sol::state& script;

public:
  ScriptedCard(sol::state& script) : script(script) {

  }

  std::shared_ptr<CardAction> BuildCardAction(std::shared_ptr<Character> user, Battle::Card::Properties& props) override {
    std::shared_ptr<CardAction> result{ nullptr };

    sol::object obj = script["card_create_action"](*user, props);

    if (obj.valid()) {
      if (obj.is<std::shared_ptr<ScriptedCardAction>>())
      {
        result = obj.as<std::shared_ptr<ScriptedCardAction>>();
      }
      else {
        Logger::Log("Lua function \"card_create_action\" didn't return a CardAction.");
      }
    }

    if (result) {
      result->SetLockoutGroup(CardAction::LockoutGroup::card);
    }

    return result;
  }
};

#endif