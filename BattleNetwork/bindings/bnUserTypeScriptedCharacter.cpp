#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedCharacter.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeEntity.h"
#include "bnScriptedCharacter.h"

void DefineScriptedCharacterUserType(ScriptResourceManager* scriptManager, sol::table& battle_namespace) {
  auto table = battle_namespace.new_usertype<WeakWrapper<ScriptedCharacter>>("Character",
    sol::factories([](Team team, Character::Rank rank) -> WeakWrapper<ScriptedCharacter> {
      auto character = std::make_shared<ScriptedCharacter>(rank);
      character->Init();
      character->SetTeam(team);

      auto wrappedCharacter = WeakWrapper(character);
      wrappedCharacter.Own();
      return wrappedCharacter;
    }),
    "from_package", [scriptManager](const std::string& fqn, Team team, Character::Rank rank) {
      sol::state* state = scriptManager->FetchCharacter(fqn);

      if (!state) {
        throw std::runtime_error("Character does not exist");
      }

      auto character = std::make_shared<ScriptedCharacter>(rank);
      character->SetTeam(team);
      character->InitFromScript(*state);

      auto wrappedCharacter = WeakWrapper(character);
      wrappedCharacter.Own();
      return wrappedCharacter;
    },
    sol::meta_function::index, [](WeakWrapper<ScriptedCharacter>& character, const std::string& key) {
      return character.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedCharacter>& character, const std::string& key, sol::stack_object value) {
      character.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedCharacter>& character) {
      return character.Unwrap()->entries.size();
    },
    "input_has", [](WeakWrapper<ScriptedCharacter>& character, const InputEvent& event) -> bool {
      return character.Unwrap()->InputState().Has(event);
    },
    "get_target", [](WeakWrapper<ScriptedCharacter>& character) -> WeakWrapper<Character> {
      return WeakWrapper(character.Unwrap()->GetTarget());
    },
    "card_action_event", sol::overload(
      [](WeakWrapper<ScriptedCharacter>& character, WeakWrapper<ScriptedCardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->AddAction(CardEvent{ cardAction.Release() }, order);
      },
      [](WeakWrapper<ScriptedCharacter>& character, WeakWrapper<CardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->AddAction(CardEvent{ cardAction.Release() }, order);
      }
    ),
    "set_shadow", sol::overload(
      [](WeakWrapper<ScriptedCharacter>& character, Entity::Shadow type) {
        character.Unwrap()->SetShadowSprite(type);
      },
      [](WeakWrapper<ScriptedCharacter>& character, std::shared_ptr<sf::Texture> shadow) {
        character.Unwrap()->SetShadowSprite(shadow);
      }
    ),
    "set_name", [](WeakWrapper<ScriptedCharacter>& character, std::string name) {
      character.Unwrap()->SetName(name);
    },
    "get_rank", [](WeakWrapper<ScriptedCharacter>& character) -> Character::Rank {
      return character.Unwrap()->GetRank();
    },
    "toggle_hitbox", [](WeakWrapper<ScriptedCharacter>& character, bool enabled) {
      return character.Unwrap()->EnableHitbox(enabled);
    },
    "share_tile", [](WeakWrapper<ScriptedCharacter>& character, bool share) {
      character.Unwrap()->ShareTileSpace(share);
    },
    "add_defense_rule", [](WeakWrapper<ScriptedCharacter>& character, DefenseRule* defenseRule) {
      character.Unwrap()->AddDefenseRule(defenseRule->shared_from_this());
    },
    "remove_defense_rule", [](WeakWrapper<ScriptedCharacter>& character, DefenseRule* defenseRule) {
      character.Unwrap()->RemoveDefenseRule(defenseRule);
    },
    "toggle_counter", [](WeakWrapper<ScriptedCharacter>& character, bool on) {
      character.Unwrap()->ToggleCounter(on);
    },
    "register_status_callback", [](WeakWrapper<ScriptedCharacter>& character, const Hit::Flags& flag, sol::object callbackObject) {
      character.Unwrap()->RegisterStatusCallback(flag, [callbackObject] {
        sol::protected_function callback = callbackObject;
        auto result = callback();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(LogLevel::critical, error.what());
        }
      });
    },
    "set_explosion_behavior", [](WeakWrapper<ScriptedCharacter>& character, int num, double speed, bool isBoss) {
      character.Unwrap()->SetExplosionBehavior(num, speed, isBoss);
    },
    "update_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) { return character.Unwrap()->update_func; },
      [](WeakWrapper<ScriptedCharacter>& character, sol::stack_object value) {
        character.Unwrap()->update_func = VerifyLuaCallback(value);
      }
    ),
    "delete_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) { return character.Unwrap()->delete_func; },
      [](WeakWrapper<ScriptedCharacter>& character, sol::stack_object value) {
        character.Unwrap()->delete_func = VerifyLuaCallback(value);
      }
    ),
    "on_spawn_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) { return character.Unwrap()->on_spawn_func; },
      [](WeakWrapper<ScriptedCharacter>& character, sol::stack_object value) {
        character.Unwrap()->on_spawn_func = VerifyLuaCallback(value);
      }
    ),
    "battle_start_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) { return character.Unwrap()->battle_start_func; },
      [](WeakWrapper<ScriptedCharacter>& character, sol::stack_object value) {
        character.Unwrap()->battle_start_func = VerifyLuaCallback(value);
      }
    ),
    "battle_end_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) { return character.Unwrap()->battle_end_func; },
      [](WeakWrapper<ScriptedCharacter>& character, sol::stack_object value) {
        character.Unwrap()->battle_end_func = VerifyLuaCallback(value);
      }
    ),
    "can_move_to_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) { return character.Unwrap()->can_move_to_func; },
      [](WeakWrapper<ScriptedCharacter>& character, sol::stack_object value) {
        character.Unwrap()->can_move_to_func = VerifyLuaCallback(value);
      }
    ),
    "on_countered", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) { return character.Unwrap()->on_countered_func; },
      [](WeakWrapper<ScriptedCharacter>& character, sol::stack_object value) {
        character.Unwrap()->on_countered_func = VerifyLuaCallback(value);
      }
    )
  );

  DefineEntityFunctionsOn(table);
  table["get_animation"] = [](WeakWrapper<ScriptedCharacter>& character) -> AnimationWrapper {
    auto& animation = character.Unwrap()->GetAnimationObject();
    return AnimationWrapper(character.GetWeak(), animation);
  };
  table["shake_camera"] = [](WeakWrapper<ScriptedCharacter>& character, double power, float duration) {
    character.Unwrap()->ShakeCamera(power, duration);
  };
}
#endif
