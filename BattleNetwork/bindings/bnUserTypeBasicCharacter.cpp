#ifdef BN_MOD_SUPPORT
#include "bnUserTypeBasicCharacter.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeEntity.h"
#include "bnScriptedCardAction.h"
#include "../bnCharacter.h"

void DefineBasicCharacterUserType(sol::table& battle_namespace) {
  auto table = battle_namespace.new_usertype<WeakWrapper<Character>>("BasicCharacter",
    "card_action_event", sol::overload(
      [](WeakWrapper<Character>& character, WeakWrapper<ScriptedCardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->AddAction(CardEvent{ cardAction.UnwrapAndRelease() }, order);
      },
      [](WeakWrapper<Character>& character, WeakWrapper<CardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->AddAction(CardEvent{ cardAction.UnwrapAndRelease() }, order);
      }
    ),
    "set_name", [](WeakWrapper<Character>& character, std::string name) {
      character.Unwrap()->SetName(name);
    },
    "get_rank", [](WeakWrapper<Character>& character) -> Character::Rank {
      return character.Unwrap()->GetRank();
    },
    "toggle_hitbox", [](WeakWrapper<Character>& character, bool enabled) {
      return character.Unwrap()->EnableHitbox(enabled);
    },
    "add_defense_rule", [](WeakWrapper<Character>& character, DefenseRule* defenseRule) {
      character.Unwrap()->AddDefenseRule(defenseRule->shared_from_this());
    },
    "remove_defense_rule", [](WeakWrapper<Character>& character, DefenseRule* defenseRule) {
      character.Unwrap()->RemoveDefenseRule(defenseRule);
    },
    "set_shadow", sol::overload(
      [](WeakWrapper<Character>& character, Entity::Shadow type) {
        character.Unwrap()->SetShadowSprite(type);
      },
      [](WeakWrapper<Character>& character, std::shared_ptr<sf::Texture> shadow) {
        character.Unwrap()->SetShadowSprite(shadow);
      }
    ),
    "toggle_counter", [](WeakWrapper<Character>& character, bool on) {
      character.Unwrap()->ToggleCounter(on);
    }
  );

  DefineEntityFunctionsOn(table);
}
#endif