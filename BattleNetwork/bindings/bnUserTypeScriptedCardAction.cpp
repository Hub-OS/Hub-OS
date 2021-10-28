#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedCardAction.h"

#include "bnWeakWrapper.h"
#include "bnWeakWrapperChild.h"
#include "bnUserTypeAnimation.h"
#include "bnScriptedCardAction.h"
#include "bnScriptedCharacter.h"
#include "bnScriptedPlayer.h"
#include "../bnSolHelpers.h"

using CardActionAttachmentWrapper = WeakWrapperChild<CardAction, CardAction::Attachment>;

static WeakWrapper<ScriptedCardAction> construct(std::shared_ptr<Character> character, const std::string& state) {
  auto cardAction = std::make_shared<ScriptedCardAction>(character, state);
  cardAction->Init();

  auto wrappedCardAction = WeakWrapper(cardAction);
  wrappedCardAction.Own();
  return wrappedCardAction;
}

void DefineScriptedCardActionUserType(sol::table& battle_namespace) {
  // make sure to copy method changes to bnUserTypeBaseCardAction
  battle_namespace.new_usertype<WeakWrapper<ScriptedCardAction>>("CardAction",
    sol::factories(
      [](WeakWrapper<Character>& character, const std::string& state)-> WeakWrapper<ScriptedCardAction> {
        return construct(character.Unwrap(), state);
      },
      [](WeakWrapper<ScriptedCharacter>& character, const std::string& state)-> WeakWrapper<ScriptedCardAction> {
        return construct(character.Unwrap(), state);
      },
      [](WeakWrapper<ScriptedPlayer>& character, const std::string& state)-> WeakWrapper<ScriptedCardAction> {
        return construct(character.Unwrap(), state);
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
    "add_anim_action", [](WeakWrapper<ScriptedCardAction>& cardAction, int frame, sol::stack_object actionObject) {
      sol::protected_function action = actionObject;
      cardAction.Unwrap()->AddAnimAction(frame, [action]{ action(); });
    },
    "add_step", [](WeakWrapper<ScriptedCardAction>& cardAction, const CardAction::Step& step) {
      cardAction.Unwrap()->AddStep(step);
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
    )
  );
}
#endif
