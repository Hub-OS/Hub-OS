#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedObstacle.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeEntity.h"
#include "bnScriptedObstacle.h"

void DefineScriptedObstacleUserType(sol::table& battle_namespace) {
  auto scriptedobstacle_record = battle_namespace.new_usertype<WeakWrapper<ScriptedObstacle>>("Obstacle",
    sol::factories([](Team team) -> WeakWrapper<ScriptedObstacle> {
      auto obstacle = std::make_shared<ScriptedObstacle>(team);
      obstacle->Init();

      auto wrappedObstacle = WeakWrapper<ScriptedObstacle>(obstacle);
      wrappedObstacle.Own();
      return wrappedObstacle;
    }),
    sol::meta_function::index, [](WeakWrapper<ScriptedObstacle>& obstacle, const std::string& key) {
      return obstacle.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedObstacle>& obstacle, const std::string& key, sol::stack_object value) {
      obstacle.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedObstacle>& obstacle) {
      return obstacle.Unwrap()->entries.size();
    },
    "set_name", [](WeakWrapper<ScriptedObstacle>& obstacle, std::string name) {
      obstacle.Unwrap()->SetName(name);
    },
    "share_tile", [](WeakWrapper<ScriptedObstacle>& obstacle, bool share) {
      obstacle.Unwrap()->ShareTileSpace(share);
    },
    "add_defense_rule", [](WeakWrapper<ScriptedObstacle>& obstacle, DefenseRule* defenseRule) {
      obstacle.Unwrap()->AddDefenseRule(defenseRule->shared_from_this());
    },
    "remove_defense_rule", [](WeakWrapper<ScriptedObstacle>& obstacle, DefenseRule* defenseRule) {
      obstacle.Unwrap()->RemoveDefenseRule(defenseRule);
    },
    "ignore_common_aggressor", [](WeakWrapper<ScriptedObstacle>& obstacle, bool enable) {
      obstacle.Unwrap()->IgnoreCommonAggressor(enable);
    },
    "shake_camera", [](WeakWrapper<ScriptedObstacle>& obstacle, double power, float duration) {
      obstacle.Unwrap()->ShakeCamera(power, duration);
    },
    "can_move_to_func", sol::property(
      [](WeakWrapper<ScriptedObstacle>& obstacle) { return obstacle.Unwrap()->can_move_to_func; },
      [](WeakWrapper<ScriptedObstacle>& obstacle, sol::stack_object value) {
        obstacle.Unwrap()->can_move_to_func = VerifyLuaCallback(value);
      }
    ),
    "collision_func", sol::property(
      [](WeakWrapper<ScriptedObstacle>& obstacle) { return obstacle.Unwrap()->collision_func; },
      [](WeakWrapper<ScriptedObstacle>& obstacle, sol::stack_object value) {
        obstacle.Unwrap()->collision_func = VerifyLuaCallback(value);
      }
    ),
    "update_func", sol::property(
      [](WeakWrapper<ScriptedObstacle>& obstacle) { return obstacle.Unwrap()->update_func; },
      [](WeakWrapper<ScriptedObstacle>& obstacle, sol::stack_object value) {
        obstacle.Unwrap()->update_func = VerifyLuaCallback(value);
      }
    ),
    "delete_func", sol::property(
      [](WeakWrapper<ScriptedObstacle>& obstacle) { return obstacle.Unwrap()->delete_func; },
      [](WeakWrapper<ScriptedObstacle>& obstacle, sol::stack_object value) {
        obstacle.Unwrap()->delete_func = VerifyLuaCallback(value);
      }
    ),
    "attack_func", sol::property(
      [](WeakWrapper<ScriptedObstacle>& obstacle) { return obstacle.Unwrap()->attack_func; },
      [](WeakWrapper<ScriptedObstacle>& obstacle, sol::stack_object value) {
        obstacle.Unwrap()->attack_func = VerifyLuaCallback(value);
      }
    ),
    "on_spawn_func", sol::property(
      [](WeakWrapper<ScriptedObstacle>& obstacle) { return obstacle.Unwrap()->on_spawn_func; },
      [](WeakWrapper<ScriptedObstacle>& obstacle, sol::stack_object value) {
        obstacle.Unwrap()->on_spawn_func = VerifyLuaCallback(value);
      }
    )
  );

  DefineEntityFunctionsOn(scriptedobstacle_record);

  scriptedobstacle_record["set_animation"] = [](WeakWrapper<ScriptedObstacle>& obstacle, std::string animation) {
    obstacle.Unwrap()->SetAnimation(animation);
  };
  scriptedobstacle_record["get_animation"] = [](WeakWrapper<ScriptedObstacle>& obstacle) -> AnimationWrapper {
    auto& animation = obstacle.Unwrap()->GetAnimationObject();
    return AnimationWrapper(obstacle.GetWeak(), animation);
  };
}
#endif
