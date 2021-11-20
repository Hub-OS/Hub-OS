#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedArtifact.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeEntity.h"
#include "bnScriptedArtifact.h"

void DefineScriptedArtifactUserType(sol::table& battle_namespace) {
  auto table = battle_namespace.new_usertype<WeakWrapper<ScriptedArtifact>>("Artifact",
    sol::factories([]() -> WeakWrapper<ScriptedArtifact> {
      auto artifact = std::make_shared<ScriptedArtifact>();
      artifact->Init();

      auto wrappedArtifact = WeakWrapper<ScriptedArtifact>(artifact);
      wrappedArtifact.Own();
      return wrappedArtifact;
    }),
    sol::meta_function::index, [](WeakWrapper<ScriptedArtifact>& artifact, const std::string& key) {
      return artifact.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedArtifact>& artifact, const std::string& key, sol::stack_object value) {
      artifact.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedArtifact>& artifact) {
      return artifact.Unwrap()->entries.size();
    },
    "update_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) { return artifact.Unwrap()->update_func; },
      [](WeakWrapper<ScriptedArtifact>& artifact, sol::stack_object value) {
        artifact.Unwrap()->update_func = VerifyLuaCallback(value);
      }
    ),
    "on_spawn_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) { return artifact.Unwrap()->on_spawn_func; },
      [](WeakWrapper<ScriptedArtifact>& artifact, sol::stack_object value) {
        artifact.Unwrap()->on_spawn_func = VerifyLuaCallback(value);
      }
    ),
    "delete_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) { return artifact.Unwrap()->delete_func; },
      [](WeakWrapper<ScriptedArtifact>& artifact, sol::stack_object value) {
        artifact.Unwrap()->delete_func = VerifyLuaCallback(value);
      }
    ),
    "can_move_to_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) { return artifact.Unwrap()->can_move_to_func; },
      [](WeakWrapper<ScriptedArtifact>& artifact, sol::stack_object value) {
        artifact.Unwrap()->can_move_to_func = VerifyLuaCallback(value);
      }
    )
  );

  DefineEntityFunctionsOn(table);
  table["set_animation"] = [](WeakWrapper<ScriptedArtifact>& artifact, std::string animation) {
    artifact.Unwrap()->SetAnimation(animation);
  };
  table["get_animation"] = [](WeakWrapper<ScriptedArtifact>& artifact) -> AnimationWrapper {
    auto& animation = artifact.Unwrap()->GetAnimationObject();
    return AnimationWrapper(artifact.GetWeak(), animation);
  };
}
#endif
