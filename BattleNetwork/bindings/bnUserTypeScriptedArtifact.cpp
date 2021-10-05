#include "bnUserTypeScriptedArtifact.h"

#include "bnWeakWrapper.h"
#include "bnScriptedArtifact.h"
#include "../bnTile.h"

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
    sol::meta_function::index, [](WeakWrapper<ScriptedArtifact>& artifact, std::string key) {
      return artifact.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedArtifact>& artifact, std::string key, sol::stack_object value) {
      artifact.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedArtifact>& artifact) {
      return artifact.Unwrap()->entries.size();
    },
    "get_id", [](WeakWrapper<ScriptedArtifact>& artifact) -> Entity::ID_t {
      return artifact.Unwrap()->GetID();
    },
    "get_tile", [](WeakWrapper<ScriptedArtifact>& artifact, Direction dir, unsigned count) -> Battle::Tile* {
      return artifact.Unwrap()->GetTile(dir, count);
    },
    "get_current_tile", [](WeakWrapper<ScriptedArtifact>& artifact) -> Battle::Tile* {
      return artifact.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<ScriptedArtifact>& artifact) -> WeakWrapper<Field> {
      return WeakWrapper(artifact.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<ScriptedArtifact>& artifact) -> Direction {
      return artifact.Unwrap()->GetFacing();
    },
    "set_facing", [](WeakWrapper<ScriptedArtifact>& artifact, Direction direction) {
      artifact.Unwrap()->SetFacing(direction);
    },
    "get_alpha", [](WeakWrapper<ScriptedArtifact>& artifact) -> int {
      return artifact.Unwrap()->GetAlpha();
    },
    "set_alpha", [](WeakWrapper<ScriptedArtifact>& artifact, int alpha) {
      artifact.Unwrap()->SetAlpha(alpha);
    },
    "get_color", [](WeakWrapper<ScriptedArtifact>& artifact) -> sf::Color {
      return artifact.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<ScriptedArtifact>& artifact, sf::Color color) {
      artifact.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<ScriptedArtifact>& artifact) -> SpriteProxyNode& {
      return artifact.Unwrap()->AsSpriteProxyNode();
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
      std::function<void()> onBegin
    ) -> bool {
      return artifact.Unwrap()->Teleport(dest, order, onBegin);
    },
    "slide", [](
      WeakWrapper<ScriptedArtifact>& artifact,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      std::function<void()> onBegin
    ) -> bool {
      return artifact.Unwrap()->Slide(dest, slideTime, endlag, order, onBegin);
    },
    "jump", [](
      WeakWrapper<ScriptedArtifact>& artifact,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      std::function<void()> onBegin
    ) -> bool {
      return artifact.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, onBegin);
    },
    "raw_move_event", [](WeakWrapper<ScriptedArtifact>& artifact, const MoveEvent& event, ActionOrder order) -> bool {
      return artifact.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      return artifact.Unwrap()->IsJumping();
    },
    "is_jumping", [](WeakWrapper<ScriptedArtifact>& artifact) -> bool {
      return artifact.Unwrap()->IsSliding();
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
      return artifact.Unwrap()->WillRemoveLater();
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
    "get_animation", [](WeakWrapper<ScriptedArtifact>& artifact) -> Animation& {
      return artifact.Unwrap()->GetAnimationObject();
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
    "delete_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) {
        return artifact.Unwrap()->deleteCallback;
      },
      [](WeakWrapper<ScriptedArtifact>& artifact, std::function<void(WeakWrapper<ScriptedArtifact>)> callback) {
        artifact.Unwrap()->deleteCallback = callback;
      }
    ),
    "update_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) {
        return artifact.Unwrap()->updateCallback;
      },
      [](WeakWrapper<ScriptedArtifact>& artifact, std::function<void(WeakWrapper<ScriptedArtifact>, double)> callback) {
        artifact.Unwrap()->updateCallback = callback;
      }
    ),
    "can_move_to_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) {
        return artifact.Unwrap()->canMoveToCallback;
      },
      [](WeakWrapper<ScriptedArtifact>& artifact, std::function<bool(Battle::Tile&)> callback) {
        artifact.Unwrap()->canMoveToCallback = callback;
      }
    ),
    "on_spawn_func", sol::property(
      [](WeakWrapper<ScriptedArtifact>& artifact) {
        return artifact.Unwrap()->spawnCallback;
      },
      [](WeakWrapper<ScriptedArtifact>& artifact, std::function<void(WeakWrapper<ScriptedArtifact>, Battle::Tile&)> callback) {
        artifact.Unwrap()->spawnCallback = callback;
      }
    )
  );
}