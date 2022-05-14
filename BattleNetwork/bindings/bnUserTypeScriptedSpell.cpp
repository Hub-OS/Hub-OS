#ifdef ONB_MOD_SUPPORT
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
    "on_update_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_update_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_update_func = VerifyLuaCallback(value);
      }
    ),
    "on_delete_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_delete_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_delete_func = VerifyLuaCallback(value);
      }
    ),
    "on_collision_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_collision_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_collision_func = VerifyLuaCallback(value);
      }
    ),
    "on_attack_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_attack_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_attack_func = VerifyLuaCallback(value);
      }
    ),
    "on_spawn_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_spawn_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_spawn_func = VerifyLuaCallback(value);
      }
    ),
    "on_battle_start_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_battle_start_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_battle_start_func = VerifyLuaCallback(value);
      }
    ),
    "on_battle_end_func", sol::property(
      [](WeakWrapper<ScriptedSpell>& spell) { return spell.Unwrap()->on_battle_end_func; },
      [](WeakWrapper<ScriptedSpell>& spell, sol::stack_object value) {
        spell.Unwrap()->on_battle_end_func = VerifyLuaCallback(value);
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
