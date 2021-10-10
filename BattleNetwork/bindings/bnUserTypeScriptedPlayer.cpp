#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedPlayer.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeAnimation.h"
#include "bnScriptedPlayer.h"

void DefineScriptedPlayerUserType(sol::table& battle_namespace) {
  battle_namespace.new_usertype<WeakWrapper<ScriptedPlayer>>("Player",
    sol::meta_function::index, [](WeakWrapper<ScriptedPlayer>& player, std::string key) {
      return player.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedPlayer>& player, std::string key, sol::stack_object value) {
      player.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedPlayer>& player) {
      return player.Unwrap()->entries.size();
    },
    "get_id", [](WeakWrapper<ScriptedPlayer>& player) -> Entity::ID_t {
      return player.Unwrap()->GetID();
    },
    "get_element", [](WeakWrapper<ScriptedPlayer>& player) -> Element {
      return player.Unwrap()->GetElement();
    },
    "set_element", [](WeakWrapper<ScriptedPlayer>& player, Element element) {
      player.Unwrap()->SetElement(element);
    },
    "get_tile", [](WeakWrapper<ScriptedPlayer>& player, Direction dir, unsigned count) -> Battle::Tile* {
      return player.Unwrap()->GetTile(dir, count);
    },
    "get_current_tile", [](WeakWrapper<ScriptedPlayer>& player) -> Battle::Tile* {
      return player.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<ScriptedPlayer>& player) -> WeakWrapper<Field> {
      return WeakWrapper(player.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<ScriptedPlayer>& player) -> Direction {
      return player.Unwrap()->GetFacing();
    },
    "get_alpha", [](WeakWrapper<ScriptedPlayer>& player) -> int {
      return player.Unwrap()->GetAlpha();
    },
    "set_alpha", [](WeakWrapper<ScriptedPlayer>& player, int alpha) {
      player.Unwrap()->SetAlpha(alpha);
    },
    "get_color", [](WeakWrapper<ScriptedPlayer>& player) -> sf::Color {
      return player.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<ScriptedPlayer>& player, sf::Color color) {
      player.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<ScriptedPlayer>& player) -> std::shared_ptr<SpriteProxyNode> {
      return player.Unwrap();
    },
    "hide", [](WeakWrapper<ScriptedPlayer>& player) {
      player.Unwrap()->Hide();
    },
    "reveal", [](WeakWrapper<ScriptedPlayer>& player) {
      player.Unwrap()->Reveal();
    },
    "teleport", [](
      WeakWrapper<ScriptedPlayer>& player,
      Battle::Tile* dest,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return player.Unwrap()->Teleport(dest, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "slide", [](
      WeakWrapper<ScriptedPlayer>& player,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return player.Unwrap()->Slide(dest, slideTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "jump", [](
      WeakWrapper<ScriptedPlayer>& player,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return player.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "raw_move_event", [](WeakWrapper<ScriptedPlayer>& player, const MoveEvent& event, ActionOrder order) -> bool {
      return player.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsJumping();
    },
    "is_jumping", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsSliding();
    },
    "is_teleporting", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsTeleporting();
    },
    "is_moving", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsMoving();
    },
    "is_passthrough", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsPassthrough();
    },
    "is_deleted", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsDeleted();
    },
    "will_remove_eof", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->WillRemoveLater();
    },
    "get_team", [](WeakWrapper<ScriptedPlayer>& player) -> Team {
      return player.Unwrap()->GetTeam();
    },
    "get_name", [](WeakWrapper<ScriptedPlayer>& player) -> std::string {
      return player.Unwrap()->GetName();
    },
    "set_name", [](WeakWrapper<ScriptedPlayer>& player, std::string name) {
      player.Unwrap()->SetName(name);
    },
    "get_health", [](WeakWrapper<ScriptedPlayer>& player) -> int {
      return player.Unwrap()->GetHealth();
    },
    "get_max_health", [](WeakWrapper<ScriptedPlayer>& player) -> int {
      return player.Unwrap()->GetMaxHealth();
    },
    "set_health", [](WeakWrapper<ScriptedPlayer>& player, int health) {
      player.Unwrap()->SetHealth(health);
    },
    "get_texture", [](WeakWrapper<ScriptedPlayer>& player) -> std::shared_ptr<Texture> {
      return player.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<ScriptedPlayer>& player, std::shared_ptr<Texture> texture) {
      player.Unwrap()->setTexture(texture);
    },
    "set_height", [](WeakWrapper<ScriptedPlayer>& player, float height) {
      player.Unwrap()->SetHeight(height);
    },
    "set_fully_charged_color", [](WeakWrapper<ScriptedPlayer>& player, sf::Color color) {
      player.Unwrap()->SetFullyChargeColor(color);
    },
    "set_charge_position", [](WeakWrapper<ScriptedPlayer>& player, sf::Color color) {
      player.Unwrap()->SetFullyChargeColor(color);
    },
    "set_animation", [](WeakWrapper<ScriptedPlayer>& player, std::string animation) {
      player.Unwrap()->SetAnimation(animation);
    },
    "get_animation", [](WeakWrapper<ScriptedPlayer>& player) -> AnimationWrapper {
      auto& animation = player.Unwrap()->GetAnimationObject();
      return AnimationWrapper(player.GetWeak(), animation);
    },
    "set_float_shoe", [](WeakWrapper<ScriptedPlayer>& player, bool enable) {
      player.Unwrap()->SetFloatShoe(enable);
    },
    "set_air_shoe", [](WeakWrapper<ScriptedPlayer>& player, bool enable) {
      player.Unwrap()->SetAirShoe(enable);
    },
    "slide_when_moving", [](WeakWrapper<ScriptedPlayer>& player, bool enable, const frame_time_t& frames) {
      player.Unwrap()->SlideWhenMoving(enable, frames);
    },
    "delete", [](WeakWrapper<ScriptedPlayer>& player) {
      player.Unwrap()->Delete();
    },
    "register_component", [](WeakWrapper<ScriptedPlayer>& player, std::shared_ptr<Component> component) {
      player.Unwrap()->RegisterComponent(component);
    }
  );
}
#endif
