#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedObstacle.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeAnimation.h"
#include "bnScriptedObstacle.h"
#include "bnScriptedComponent.h"
#include "../bnTile.h"

void DefineScriptedObstacleUserType(sol::table& battle_namespace) {
  const auto& scriptedobstacle_record = battle_namespace.new_usertype<WeakWrapper<ScriptedObstacle>>("Obstacle",
    sol::factories([](Team team) -> WeakWrapper<ScriptedObstacle> {
      auto obstacle = std::make_shared<ScriptedObstacle>(team);
      obstacle->Init();

      auto wrappedObstacle = WeakWrapper<ScriptedObstacle>(obstacle);
      wrappedObstacle.Own();
      return wrappedObstacle;
    }),
    sol::meta_function::index, [](WeakWrapper<ScriptedObstacle>& obstacle, std::string key) {
      return obstacle.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedObstacle>& obstacle, std::string key, sol::stack_object value) {
      obstacle.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedObstacle>& obstacle) {
      return obstacle.Unwrap()->entries.size();
    },
    "get_id", [](WeakWrapper<ScriptedObstacle>& obstacle) -> Entity::ID_t {
      return obstacle.Unwrap()->GetID();
    },
    "get_element", [](WeakWrapper<ScriptedObstacle>& obstacle) -> Element {
      return obstacle.Unwrap()->GetElement();
    },
    "set_element", [](WeakWrapper<ScriptedObstacle>& obstacle, Element element) {
      obstacle.Unwrap()->SetElement(element);
    },
    "get_tile", [](WeakWrapper<ScriptedObstacle>& obstacle, Direction dir, unsigned count) -> Battle::Tile* {
      return obstacle.Unwrap()->GetTile(dir, count);
    },
    "get_current_tile", [](WeakWrapper<ScriptedObstacle>& obstacle) -> Battle::Tile* {
      return obstacle.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<ScriptedObstacle>& obstacle) -> WeakWrapper<Field> {
      return WeakWrapper(obstacle.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<ScriptedObstacle>& obstacle) -> Direction {
      return obstacle.Unwrap()->GetFacing();
    },
    "set_facing", [](WeakWrapper<ScriptedObstacle>& obstacle, Direction direction) {
      obstacle.Unwrap()->SetFacing(direction);
    },
    "get_alpha", [](WeakWrapper<ScriptedObstacle>& obstacle) -> int {
      return obstacle.Unwrap()->GetAlpha();
    },
    "set_alpha", [](WeakWrapper<ScriptedObstacle>& obstacle, int alpha) {
      obstacle.Unwrap()->SetAlpha(alpha);
    },
    "get_color", [](WeakWrapper<ScriptedObstacle>& obstacle) -> sf::Color {
      return obstacle.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<ScriptedObstacle>& obstacle, sf::Color color) {
      obstacle.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<ScriptedObstacle>& obstacle) -> WeakWrapper<SpriteProxyNode> {
      return WeakWrapper(std::static_pointer_cast<SpriteProxyNode>(obstacle.Unwrap()));
    },
    "hide", [](WeakWrapper<ScriptedObstacle>& obstacle) {
      obstacle.Unwrap()->Hide();
    },
    "reveal", [](WeakWrapper<ScriptedObstacle>& obstacle) {
      obstacle.Unwrap()->Reveal();
    },
    "teleport", [](
      WeakWrapper<ScriptedObstacle>& obstacle,
      Battle::Tile* dest,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return obstacle.Unwrap()->Teleport(dest, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "slide", [](
      WeakWrapper<ScriptedObstacle>& obstacle,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return obstacle.Unwrap()->Slide(dest, slideTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "jump", [](
      WeakWrapper<ScriptedObstacle>& obstacle,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return obstacle.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "raw_move_event", [](WeakWrapper<ScriptedObstacle>& obstacle, const MoveEvent& event, ActionOrder order) -> bool {
      return obstacle.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<ScriptedObstacle>& obstacle) -> bool {
      return obstacle.Unwrap()->IsJumping();
    },
    "is_jumping", [](WeakWrapper<ScriptedObstacle>& obstacle) -> bool {
      return obstacle.Unwrap()->IsSliding();
    },
    "is_teleporting", [](WeakWrapper<ScriptedObstacle>& obstacle) -> bool {
      return obstacle.Unwrap()->IsTeleporting();
    },
    "is_moving", [](WeakWrapper<ScriptedObstacle>& obstacle) -> bool {
      return obstacle.Unwrap()->IsMoving();
    },
    "is_passthrough", [](WeakWrapper<ScriptedObstacle>& obstacle) -> bool {
      return obstacle.Unwrap()->IsPassthrough();
    },
    "is_deleted", [](WeakWrapper<ScriptedObstacle>& obstacle) -> bool {
      return obstacle.Unwrap()->IsDeleted();
    },
    "will_remove_eof", [](WeakWrapper<ScriptedObstacle>& obstacle) -> bool {
      return obstacle.Unwrap()->WillRemoveLater();
    },
    "is_team", [](WeakWrapper<ScriptedObstacle>& obstacle, Team team) -> bool {
      return obstacle.Unwrap()->Teammate(team);
    },
    "get_team", [](WeakWrapper<ScriptedObstacle>& obstacle) -> Team {
      return obstacle.Unwrap()->GetTeam();
    },
    "remove", [](WeakWrapper<ScriptedObstacle>& obstacle) {
      obstacle.Unwrap()->Remove();
    },
    "delete", [](WeakWrapper<ScriptedObstacle>& obstacle) {
      obstacle.Unwrap()->Delete();
    },
    "get_name", [](WeakWrapper<ScriptedObstacle>& obstacle) -> std::string {
      return obstacle.Unwrap()->GetName();
    },
    "set_name", [](WeakWrapper<ScriptedObstacle>& obstacle, std::string name) {
      obstacle.Unwrap()->SetName(name);
    },
    "get_health", [](WeakWrapper<ScriptedObstacle>& obstacle) -> int {
      return obstacle.Unwrap()->GetHealth();
    },
    "get_max_health", [](WeakWrapper<ScriptedObstacle>& obstacle) -> int {
      return obstacle.Unwrap()->GetMaxHealth();
    },
    "set_health", [](WeakWrapper<ScriptedObstacle>& obstacle, int health) {
      obstacle.Unwrap()->SetHealth(health);
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
    "get_texture", [](WeakWrapper<ScriptedObstacle>& obstacle) -> std::shared_ptr<Texture> {
      return obstacle.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<ScriptedObstacle>& obstacle, std::shared_ptr<Texture> texture) {
      obstacle.Unwrap()->setTexture(texture);
    },
    "set_animation", [](WeakWrapper<ScriptedObstacle>& obstacle, std::string animation) {
      obstacle.Unwrap()->SetAnimation(animation);
    },
    "get_animation", [](WeakWrapper<ScriptedObstacle>& obstacle) -> AnimationWrapper {
      auto& animation = obstacle.Unwrap()->GetAnimationObject();
      return AnimationWrapper(obstacle.GetWeak(), animation);
    },
    "add_node", [](WeakWrapper<ScriptedObstacle>& obstacle) -> WeakWrapper<SpriteProxyNode> {
      auto child = std::make_shared<SpriteProxyNode>();
      obstacle.Unwrap()->AddNode(child);

      return WeakWrapper(child);
    },
    "highlight_tile", [](WeakWrapper<ScriptedObstacle>& obstacle, Battle::TileHighlight mode) {
      obstacle.Unwrap()->HighlightTile(mode);
    },
    "copy_hit_props", [](WeakWrapper<ScriptedObstacle>& obstacle) -> Hit::Properties {
      return obstacle.Unwrap()->GetHitboxProperties();
    },
    "set_hit_props", [](WeakWrapper<ScriptedObstacle>& obstacle, Hit::Properties props) {
      obstacle.Unwrap()->SetHitboxProperties(props);
    },
    "ignore_common_aggressor", [](WeakWrapper<ScriptedObstacle>& obstacle, bool enable) {
      obstacle.Unwrap()->IgnoreCommonAggressor(enable);
    },
    "set_height", [](WeakWrapper<ScriptedObstacle>& obstacle, float height) {
      obstacle.Unwrap()->SetHeight(height);
    },
    "show_shadow", [](WeakWrapper<ScriptedObstacle>& obstacle, bool show) {
      obstacle.Unwrap()->ShowShadow(show);
    },
    "shake_camera", [](WeakWrapper<ScriptedObstacle>& obstacle, double power, float duration) {
      obstacle.Unwrap()->ShakeCamera(power, duration);
    },
    "register_component", [](WeakWrapper<ScriptedObstacle>& obstacle, WeakWrapper<ScriptedComponent>& component) {
      obstacle.Unwrap()->RegisterComponent(component.Release());
    },
    "get_position", [](WeakWrapper<ScriptedObstacle>& obstacle) -> sf::Vector2f {
      return obstacle.Unwrap()->GetDrawOffset();
    },
    "set_position", [](WeakWrapper<ScriptedObstacle>& obstacle, float x, float y) {
      obstacle.Unwrap()->SetDrawOffset(x, y);
    },
    "never_flip", [](WeakWrapper<ScriptedObstacle>& obstacle, bool enabled) {
      obstacle.Unwrap()->NeverFlip(enabled);
    }
  );
}
#endif
