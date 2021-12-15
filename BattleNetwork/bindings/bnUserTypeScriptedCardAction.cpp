#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedCardAction.h"

#include "bnWeakWrapper.h"
#include "bnWeakWrapperChild.h"
#include "bnUserTypeAnimation.h"
#include "bnScriptedCardAction.h"
#include "bnScriptedCharacter.h"
#include "bnScriptedPlayer.h"
#include "../bnCardPackageManager.h"
#include "../bnSolHelpers.h"

using CardActionAttachmentWrapper = WeakWrapperChild<CardAction, CardAction::Attachment>;

static WeakWrapper<ScriptedCardAction> construct(std::shared_ptr<Character> character, const std::string& state) {
  auto cardAction = std::make_shared<ScriptedCardAction>(character, state);
  cardAction->Init();

  auto wrappedCardAction = WeakWrapper(cardAction);
  wrappedCardAction.Own();
  return wrappedCardAction;
}

void DefineScriptedCardActionUserType(ScriptResourceManager* scriptManager, sol::table& battle_namespace) {
  auto action_from_card = [scriptManager](const std::string& fqn, std::shared_ptr<Character> character, const Battle::Card::Properties& props) -> WeakWrapper<ScriptedCardAction> {
    auto cardPackages = &scriptManager->GetCardPackageManager();

    if (!cardPackages) {
      Logger::Log(LogLevel::critical, "Battle.CardAction.from_card() was called but CardPackageManager was nullptr!");
      return WeakWrapper<ScriptedCardAction>();
    }

    if (!cardPackages->HasPackage(fqn)) {
      Logger::Log(LogLevel::critical, "Battle.CardAction.from_card() was called with unknown package " + fqn);
      return WeakWrapper<ScriptedCardAction>();
    }

    auto wrappedCharacter = WeakWrapper(character);
    auto functionResult = CallLuaFunctionExpectingValue<WeakWrapper<ScriptedCardAction>>(*scriptManager->FetchCard(fqn), "card_create_action", wrappedCharacter, props);

    if (functionResult.is_error()) {
      Logger::Log(LogLevel::critical, functionResult.error_cstr());
      return WeakWrapper<ScriptedCardAction>();
    }

    return functionResult.value();
  };

  // make sure to copy method changes to bnUserTypeBaseCardAction
  battle_namespace.new_usertype<WeakWrapper<ScriptedCardAction>>("CardAction",
    sol::factories(
      [](WeakWrapper<Character>& character, const std::string& state)-> WeakWrapper<ScriptedCardAction> {
        return construct(character.Unwrap(), state);
      },
      [](WeakWrapper<Player>& character, const std::string& state)-> WeakWrapper<ScriptedCardAction> {
        return construct(character.Unwrap(), state);
      },
      [](WeakWrapper<ScriptedCharacter>& character, const std::string& state)-> WeakWrapper<ScriptedCardAction> {
        return construct(character.Unwrap(), state);
      },
      [](WeakWrapper<ScriptedPlayer>& character, const std::string& state)-> WeakWrapper<ScriptedCardAction> {
        return construct(character.Unwrap(), state);
      }
    ),
    "from_card", sol::overload(
      // todo: cut these in half by making use of std::optional?
      [action_from_card](const std::string& fqn, WeakWrapper<Character> character, const Battle::Card::Properties& props) -> WeakWrapper<ScriptedCardAction>
      {
        return action_from_card(fqn, character.Unwrap(), props);
      },
      [action_from_card](const std::string& fqn, WeakWrapper<Player> character, const Battle::Card::Properties& props) -> WeakWrapper<ScriptedCardAction>
      {
        return action_from_card(fqn, character.Unwrap(), props);
      },
      [action_from_card](const std::string& fqn, WeakWrapper<ScriptedCharacter> character, const Battle::Card::Properties& props) -> WeakWrapper<ScriptedCardAction>
      {
        return action_from_card(fqn, character.Unwrap(), props);
      },
      [action_from_card](const std::string& fqn, WeakWrapper<ScriptedPlayer> character, const Battle::Card::Properties& props) -> WeakWrapper<ScriptedCardAction>
      {
        return action_from_card(fqn, character.Unwrap(), props);
      },
      [scriptManager, action_from_card](const std::string& fqn, WeakWrapper<Character> character) -> WeakWrapper<ScriptedCardAction>
      {
        Battle::Card::Properties props = scriptManager->GetCardPackageManager().FindPackageByID(fqn).GetCardProperties();
        return action_from_card(fqn, character.Unwrap(), props);
      },
      [scriptManager, action_from_card](const std::string& fqn, WeakWrapper<Player> character) -> WeakWrapper<ScriptedCardAction>
      {
        Battle::Card::Properties props = scriptManager->GetCardPackageManager().FindPackageByID(fqn).GetCardProperties();
        return action_from_card(fqn, character.Unwrap(), props);
      },
      [scriptManager, action_from_card](const std::string& fqn, WeakWrapper<ScriptedCharacter> character) -> WeakWrapper<ScriptedCardAction>
      {
        Battle::Card::Properties props = scriptManager->GetCardPackageManager().FindPackageByID(fqn).GetCardProperties();
        return action_from_card(fqn, character.Unwrap(), props);
      },
      [scriptManager, action_from_card](const std::string& fqn, WeakWrapper<ScriptedPlayer> character) -> WeakWrapper<ScriptedCardAction>
      {
        Battle::Card::Properties props = scriptManager->GetCardPackageManager().FindPackageByID(fqn).GetCardProperties();
        return action_from_card(fqn, character.Unwrap(), props);
      }
    ),
    sol::meta_function::index, [](WeakWrapper<ScriptedCardAction>& cardAction, const std::string& key) {
      return cardAction.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedCardAction>& cardAction, const std::string& key, sol::stack_object value) {
      cardAction.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedCardAction>& cardAction) {
      return cardAction.Unwrap()->entries.size();
    },
    "set_lockout", [](WeakWrapper<ScriptedCardAction>& cardAction, const CardAction::LockoutProperties& props) {
      cardAction.Unwrap()->SetLockout(props);
    },
    "set_lockout_group", [](WeakWrapper<ScriptedCardAction>& cardAction, const CardAction::LockoutGroup& group) {
      cardAction.Unwrap()->SetLockoutGroup(group);
    },
    "override_animation_frames", [](WeakWrapper<ScriptedCardAction>& cardAction, std::list<OverrideFrame> frameData) {
      cardAction.Unwrap()->OverrideAnimationFrames(frameData);
    },
    "add_attachment", [](WeakWrapper<ScriptedCardAction>& cardAction, const std::string& point) -> CardActionAttachmentWrapper {
      auto cardActionPtr = cardAction.Unwrap();
      auto& attachment = cardActionPtr->AddAttachment(point);
      return CardActionAttachmentWrapper(cardAction.GetWeak(), attachment);
    },
    "add_anim_action", [](WeakWrapper<ScriptedCardAction>& cardAction, int frame, sol::object callbackObject) {
      ExpectLuaFunction(callbackObject);

      cardAction.Unwrap()->AddAnimAction(frame, [callbackObject] {
        sol::protected_function callback = callbackObject;
        auto result = callback();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(LogLevel::critical, error.what());
        }
      });
    },
    "add_step", [](WeakWrapper<ScriptedCardAction>& cardAction, WeakWrapper<CardAction::Step> step) {
      cardAction.Unwrap()->AddStep(step.UnwrapAndRelease());
    },
    "end_action", [](WeakWrapper<ScriptedCardAction>& cardAction) {
      cardAction.Unwrap()->EndAction();
    },
    "get_actor", [](WeakWrapper<ScriptedCardAction>& cardAction) -> WeakWrapper<Character> {
      return WeakWrapper(cardAction.Unwrap()->GetActor());
    },
    "set_metadata", [](WeakWrapper<ScriptedCardAction>& cardAction, const Battle::Card::Properties& props) {
      cardAction.Unwrap()->SetMetaData(props);
    },
    "copy_metadata", [](WeakWrapper<ScriptedCardAction>& cardAction) -> Battle::Card::Properties {
      return cardAction.Unwrap()->GetMetaData();
    },
    "update_func", sol::property(
      [](WeakWrapper<ScriptedCardAction>& cardAction) { return cardAction.Unwrap()->update_func; },
      [](WeakWrapper<ScriptedCardAction>& cardAction, sol::stack_object value) {
        cardAction.Unwrap()->update_func = VerifyLuaCallback(value);
      }
    ),
    "animation_end_func", sol::property(
      [](WeakWrapper<ScriptedCardAction>& cardAction) { return cardAction.Unwrap()->animation_end_func; },
      [](WeakWrapper<ScriptedCardAction>& cardAction, sol::stack_object value) {
        cardAction.Unwrap()->animation_end_func = VerifyLuaCallback(value);
      }
    ),
    "action_end_func", sol::property(
      [](WeakWrapper<ScriptedCardAction>& cardAction) { return cardAction.Unwrap()->action_end_func; },
      [](WeakWrapper<ScriptedCardAction>& cardAction, sol::stack_object value) {
        cardAction.Unwrap()->action_end_func = VerifyLuaCallback(value);
      }
    ),
    "execute_func", sol::property(
      [](WeakWrapper<ScriptedCardAction>& cardAction) { return cardAction.Unwrap()->execute_func; },
      [](WeakWrapper<ScriptedCardAction>& cardAction, sol::stack_object value) {
        cardAction.Unwrap()->execute_func = VerifyLuaCallback(value);
      }
    ),
    "can_move_to_func", sol::property(
      [](WeakWrapper<ScriptedCardAction>& cardAction) { return cardAction.Unwrap()->can_move_to_func; },
      [](WeakWrapper<ScriptedCardAction>& cardAction, sol::stack_object value) {
        cardAction.Unwrap()->can_move_to_func = VerifyLuaCallback(value);
      }
    )
  );
}
#endif
