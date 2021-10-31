#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedArtifact.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeAnimation.h"
#include "bnScriptedArtifact.h"
#include "../bnTile.h"
#include "../bnSolHelpers.h"

void DefineScriptedArtifactUserType(sol::table& battle_namespace) {
  battle_namespace.new_usertype<WeakWrapper<ScriptedArtifact>>("Artifact",
    sol::factories([](Team team) -> WeakWrapper<ScriptedArtifact> {
      auto artifact = std::make_shared<ScriptedArtifact>();
      artifact->Init();
      artifact->SetTeam(team);

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
    "get_id", [](WeakWrapper<ScriptedArtifact>& artifact) -> Entity::ID_t {
      return artifact.Unwrap()->GetID();
    },
    "get_tile", sol::overload(
      [](WeakWrapper<ScriptedArtifact>& artifact, Direction dir, unsigned count) -> Battle::Tile* {
        return artifact.Unwrap()->GetTile(dir, count);
      },
      [](WeakWrapper<ScriptedArtifact>& artifact) -> Battle::Tile* {
        return artifact.Unwrap()->GetTile();
      }
    ),
    "get_current_tile", [](WeakWrapper<ScriptedArtifact>& artifact) -> Battle::Tile* {
      return artifact.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<ScriptedArtifact>& artifact) -> WeakWrapper<Field> {
      return WeakWrapper(artifact.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<ScriptedArtifact>& artifact) -> Direction {
      return artifact.Unwrap()->GetFacing();
    },
    "get_facing_away", [](WeakWrapper<ScriptedArtifact>& artifact) -> Direction {
      return artifact.Unwrap()->GetFacingAway();
    },
    "set_facing", [](WeakWrapper<ScriptedArtifact>& artifact, Direction direction) {
      artifact.Unwrap()->SetFacing(direction);
    },
    "get_color", [](WeakWrapper<ScriptedArtifact>& artifact) -> sf::Color {
      return artifact.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<ScriptedArtifact>& artifact, sf::Color color) {
      artifact.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<ScriptedArtifact>& artifact) -> WeakWrapper<SpriteProxyNode> {
      return WeakWrapper(std::static_pointer_cast<SpriteProxyNode>(artifact.Unwrap()));
    },
    "hide", [](WeakWrapper<ScriptedArtifact>& artifact) {
      artifact.Unwrap()->Hide();
    },
    "reveal", [](WeakWrapper<ScriptedArtifact>& artifact) {
      artifact.Unwrap()->Reveal();
    },
    "teleport", [](
      WeakWrapper<ScriptedArtifact>& artifact,
      Battle::Tile* dest,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return artifact.Unwrap()->Teleport(dest, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "slide", [](
      WeakWrapper<ScriptedArtifact>& artifact,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return artifact.Unwrap()->Slide(dest, slideTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "jump", [](
      WeakWrapper<ScriptedArtifact>& artifact,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return artifact.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "raw_move_event", [](WeakWrapper<ScriptedArtifact>& artifact, const MoveEvent& event, ActionOrder order) -> bool {
      return artifact.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      return artifact.Unwrap()->IsSliding();
    },
    "is_jumping", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      return artifact.Unwrap()->IsJumping();
    },
    "is_teleporting", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      return artifact.Unwrap()->IsTeleporting();
    },
    "is_moving", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      return artifact.Unwrap()->IsMoving();
    },
    "is_deleted", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      return artifact.Unwrap()->IsDeleted();
    },
    "will_remove_eof", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      auto ptr = artifact.Lock();
      return !ptr || ptr->WillRemoveLater();
    },
    "get_team", [](WeakWrapper<ScriptedArtifact>& artifact) -> Team {
      return artifact.Unwrap()->GetTeam();
    },
    "remove", [](WeakWrapper<ScriptedArtifact>& artifact) {
      artifact.Unwrap()->Remove();
    },
    "delete", [](WeakWrapper<ScriptedArtifact>& artifact) {
      artifact.Unwrap()->Delete();
    },
    "get_texture", [](WeakWrapper<ScriptedArtifact>& artifact) -> std::shared_ptr<Texture> {
      return artifact.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<ScriptedArtifact>& artifact, std::shared_ptr<Texture> texture) {
      artifact.Unwrap()->setTexture(texture);
    },
    "set_animation", [](WeakWrapper<ScriptedArtifact>& artifact, std::string animation) {
      artifact.Unwrap()->SetAnimation(animation);
    },
    "get_animation", [](WeakWrapper<ScriptedArtifact>& artifact) -> AnimationWrapper {
      auto& animation = artifact.Unwrap()->GetAnimationObject();
      return AnimationWrapper(artifact.GetWeak(), animation);
    },
    "set_height", [](WeakWrapper<ScriptedArtifact>& artifact, float height) {
      artifact.Unwrap()->SetHeight(height);
    },
    "set_position", [](WeakWrapper<ScriptedArtifact>& artifact, float x, float y) {
      artifact.Unwrap()->SetDrawOffset(x, y);
    },
    "never_flip", [](WeakWrapper<ScriptedArtifact>& artifact, bool enabled) {
      artifact.Unwrap()->NeverFlip(enabled);
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
}
#endif
