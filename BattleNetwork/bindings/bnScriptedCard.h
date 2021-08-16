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

  CardAction* BuildCardAction(Character* user, Battle::Card::Properties& props) override {
    CardAction* result{ nullptr };

    sol::object obj = script["create_card_action"](*user, props);

    if (obj.valid()) {
      if (obj.is<std::unique_ptr<ScriptedCardAction>>())
      {
        auto& ptr = obj.as<std::unique_ptr<ScriptedCardAction>&>();
        result = ptr.release();
      }
      else {
        Logger::Log("Lua function \"create_card_action\" didn't return a CardAction.");
      }
    }

    if (result) {
      result->SetLockoutGroup(CardAction::LockoutGroup::card);
    }

    return result;
  }
};

#endif