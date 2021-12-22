#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedCharacter.h"
#include "bnWeakWrapper.h"
#include "bnUserTypeEntity.h"
#include "bnScriptedCharacter.h"
#include "bnScriptedPlayer.h"
#include "../bnScriptResourceManager.h"
#include "../bnObstacle.h"

void DefineScriptedCharacterUserType(ScriptResourceManager* scriptManager, sol::state& state, sol::table& battle_namespace) {
  const std::string& namespaceId = scriptManager->GetStateNamespace(state);

  auto from = [state = &state] (std::shared_ptr<Entity> entity) {
    if (auto character = std::dynamic_pointer_cast<ScriptedCharacter>(entity)) {
      return sol::make_object(*state, WeakWrapper(character));
    }
    if (auto character = std::dynamic_pointer_cast<Character>(entity)) {
      if (!dynamic_cast<Obstacle*>(entity.get())) {
        return sol::make_object(*state, WeakWrapper(character));
      }
    }

    return sol::make_object(*state, sol::lua_nil);
  };

  auto table = battle_namespace.new_usertype<WeakWrapper<ScriptedCharacter>>("Character",
    sol::factories([](Team team, Character::Rank rank) -> WeakWrapper<ScriptedCharacter> {
      auto character = std::make_shared<ScriptedCharacter>(rank);
      character->Init();
      character->SetTeam(team);

      auto wrappedCharacter = WeakWrapper(character);
      wrappedCharacter.Own();
      return wrappedCharacter;
    }),
    "from", sol::overload(
      [](WeakWrapper<ScriptedCharacter>& e) -> WeakWrapper<ScriptedCharacter> {
        return e;
      },
      [from](WeakWrapper<Entity>& e) -> sol::object {
        return from(e.Unwrap());
      },
      [from](WeakWrapper<Character>& e) -> sol::object {
        return from(e.Unwrap());
      },
      [from](WeakWrapper<Player>& e) -> sol::object {
        return from(e.Unwrap());
      },
      [from](WeakWrapper<ScriptedPlayer>& e) -> sol::object {
        return from(e.Unwrap());
      },
      [state=&state]() -> sol::object {
        return sol::make_object(*state, sol::lua_nil);
      }
    ),
    "from_package", [scriptManager, &namespaceId](const std::string& fqn, Team team, Character::Rank rank) {
      sol::state* state = scriptManager->FetchCharacter(fqn, namespaceId);

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
        character.Unwrap()->AddAction(CardEvent{ cardAction.UnwrapAndRelease() }, order);
      },
      [](WeakWrapper<ScriptedCharacter>& character, WeakWrapper<CardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->AddAction(CardEvent{ cardAction.UnwrapAndRelease() }, order);
      }
    ),
    "get_rank", [](WeakWrapper<ScriptedCharacter>& character) -> Character::Rank {
      return character.Unwrap()->GetRank();
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
}
#endif
