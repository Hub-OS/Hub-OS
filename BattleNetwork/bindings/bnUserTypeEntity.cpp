#ifdef BN_MOD_SUPPORT
#include "bnUserTypeEntity.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeAnimation.h"
#include "../bnEntity.h"
#include "../bnAnimationComponent.h"
#include "../bnSolHelpers.h"
#include "../bnTile.h"

void DefineEntityUserType(sol::table& battle_namespace) {
  battle_namespace.new_usertype<WeakWrapper<Entity>>("Entity",
    "get_id", [](WeakWrapper<Entity>& entity) -> Entity::ID_t {
      return entity.Unwrap()->GetID();
    },
    "get_tile", sol::overload(
      [](WeakWrapper<Entity>& entity, Direction dir, unsigned count) -> Battle::Tile* {
        return entity.Unwrap()->GetTile(dir, count);
      },
      [](WeakWrapper<Entity>& entity) -> Battle::Tile* {
        return entity.Unwrap()->GetTile();
      }
    ),
    "get_current_tile", [](WeakWrapper<Entity>& entity) -> Battle::Tile* {
      return entity.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<Entity>& entity) -> WeakWrapper<Field> {
      return WeakWrapper(entity.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<Entity>& entity) -> Direction {
      return entity.Unwrap()->GetFacing();
    },
    "get_facing_away", [](WeakWrapper<Entity>& entity) -> Direction {
      return entity.Unwrap()->GetFacingAway();
    },
    "set_facing", [](WeakWrapper<Entity>& entity, Direction direction) {
      entity.Unwrap()->SetFacing(direction);
    },
    "get_color", [](WeakWrapper<Entity>& entity) -> sf::Color {
      return entity.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<Entity>& entity, sf::Color color) {
      entity.Unwrap()->setColor(color);
    },
    "set_elevation", [](WeakWrapper<Entity>& entity, float elevation) {
      entity.Unwrap()->SetElevation(elevation);
    },
    "get_elevation", [](WeakWrapper<Entity>& entity) -> float {
      return entity.Unwrap()->GetElevation();
    },
    "sprite", [](WeakWrapper<Entity>& entity) -> WeakWrapper<SpriteProxyNode> {
      return WeakWrapper(std::static_pointer_cast<SpriteProxyNode>(entity.Unwrap()));
    },
    "hide", [](WeakWrapper<Entity>& entity) {
      entity.Unwrap()->Hide();
    },
    "reveal", [](WeakWrapper<Entity>& entity) {
      entity.Unwrap()->Reveal();
    },
    "teleport", [](
      WeakWrapper<Entity>& entity,
      Battle::Tile* dest,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return entity.Unwrap()->Teleport(dest, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "slide", [](
      WeakWrapper<Entity>& entity,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return entity.Unwrap()->Slide(dest, slideTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "jump", [](
      WeakWrapper<Entity>& entity,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return entity.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "raw_move_event", [](WeakWrapper<Entity>& entity, const MoveEvent& event, ActionOrder order) -> bool {
      return entity.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<Entity>& entity) -> bool {
      return entity.Unwrap()->IsSliding();
    },
    "is_jumping", [](WeakWrapper<Entity>& entity) -> bool {
      return entity.Unwrap()->IsJumping();
    },
    "is_teleporting", [](WeakWrapper<Entity>& entity) -> bool {
      return entity.Unwrap()->IsTeleporting();
    },
    "is_moving", [](WeakWrapper<Entity>& entity) -> bool {
      return entity.Unwrap()->IsMoving();
    },
    "is_deleted", [](WeakWrapper<Entity>& entity) -> bool {
      return entity.Unwrap()->IsDeleted();
    },
    "will_remove_eof", [](WeakWrapper<Entity>& entity) -> bool {
      auto ptr = entity.Lock();
      return !ptr || ptr->WillRemoveLater();
    },
    "is_team", [](WeakWrapper<Entity>& entity, Team team) -> bool {
      return entity.Unwrap()->Teammate(team);
    },
    "get_team", [](WeakWrapper<Entity>& entity) -> Team {
      return entity.Unwrap()->GetTeam();
    },
    "remove", [](WeakWrapper<Entity>& entity) {
      entity.Unwrap()->Remove();
    },
    "delete", [](WeakWrapper<Entity>& entity) {
      entity.Unwrap()->Delete();
    },
    "get_texture", [](WeakWrapper<Entity>& entity) -> std::shared_ptr<Texture> {
      return entity.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<Entity>& entity, std::shared_ptr<Texture> texture) {
      entity.Unwrap()->setTexture(texture);
    },
    "set_animation", [](WeakWrapper<Entity>& entity, std::string animation) {
      auto animationComponent = entity.Unwrap()->GetFirstComponent<AnimationComponent>();
      animationComponent->SetPath(animation);
      animationComponent->Reload();
    },
    "get_animation", [](WeakWrapper<Entity>& entity) -> AnimationWrapper {
      auto animationComponent = entity.Unwrap()->GetFirstComponent<AnimationComponent>();
      auto& animation = animationComponent->GetAnimationObject();
      return AnimationWrapper(entity.GetWeak(), animation);
    },
    "create_node", [](WeakWrapper<Entity>& entity) -> WeakWrapper<SpriteProxyNode> {
      auto child = std::make_shared<SpriteProxyNode>();
      entity.Unwrap()->AddNode(child);

      return WeakWrapper(child);
    },
    "highlight_tile", [](WeakWrapper<Entity>& entity, Battle::TileHighlight mode) {
      entity.Unwrap()->HighlightTile(mode);
    },
    "copy_hit_props", [](WeakWrapper<Entity>& entity) -> Hit::Properties {
      return entity.Unwrap()->GetHitboxProperties();
    },
    "set_hit_props", [](WeakWrapper<Entity>& entity, Hit::Properties props) {
      entity.Unwrap()->SetHitboxProperties(props);
    },
    "set_height", [](WeakWrapper<Entity>& entity, float height) {
      entity.Unwrap()->SetHeight(height);
    },
    "show_shadow", [](WeakWrapper<Entity>& entity, bool show) {
      entity.Unwrap()->ShowShadow(show);
    },
    "get_position", [](WeakWrapper<Entity>& entity) -> sf::Vector2f {
      return entity.Unwrap()->GetDrawOffset();
    },
    "set_position", [](WeakWrapper<Entity>& entity, float x, float y) {
      entity.Unwrap()->SetDrawOffset(x, y);
    }
  );
}
#endif
