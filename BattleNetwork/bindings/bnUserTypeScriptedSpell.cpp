#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedSpell.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeEntity.h"
#include "bnScriptedSpell.h"

void DefineScriptedSpellUserType(sol::table& battle_namespace) {
  auto table = battle_namespace.new_usertype<WeakWrapper<ScriptedSpell>>("Spell",
    sol::factories([](Team team) -> WeakWrapper<ScriptedSpell> {
      auto spell = std::make_shared<ScriptedSpell>(team);
      spell->Init();

      auto wrappedSpell = WeakWrapper<ScriptedSpell>(spell);
      wrappedSpell.Own();
      return wrappedSpell;
    }),
    sol::meta_function::index, [](WeakWrapper<ScriptedSpell>& spell, const std::string& key) {
      return spell.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedSpell>& spell, const std::string& key, sol::stack_object value) {
      spell.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedSpell>& spell) {
      return spell.Unwrap()->entries.size();
    },
    "can_move_to_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->can_move_to_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->can_move_to_func = VerifyLuaCallback(value);
      }
    ),
    "update_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->update_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->update_func = VerifyLuaCallback(value);
      }
    ),
    "delete_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->delete_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->delete_func = VerifyLuaCallback(value);
      }
    ),
    "collision_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->collision_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->collision_func = VerifyLuaCallback(value);
      }
    ),
    "attack_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->attack_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->attack_func = VerifyLuaCallback(value);
      }
    ),
    "on_spawn_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_spawn_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_spawn_func = VerifyLuaCallback(value);
      }
    ),
    "battle_start_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->battle_start_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->battle_start_func = VerifyLuaCallback(value);
      }
    ),
    "battle_end_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->battle_end_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->battle_end_func = VerifyLuaCallback(value);
      }
    )
  );

  DefineEntityFunctionsOn(table);
  table["set_animation"] = [](WeakWrapper<ScriptedSpell>& spell, std::string animation) {
    spell.Unwrap()->SetAnimation(animation);
  };
  table["get_animation"] = [](WeakWrapper<ScriptedSpell>& spell) -> AnimationWrapper {
    auto& animation = spell.Unwrap()->GetAnimationObject();
    return AnimationWrapper(spell.GetWeak(), animation);
  };
}
#endif
