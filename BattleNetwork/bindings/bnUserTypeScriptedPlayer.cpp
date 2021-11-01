#ifdef BN_MOD_SUPPORT
#include "bnUserTypeScriptedPlayer.h"

#include "bnWeakWrapper.h"
#include "bnScopedWrapper.h"
#include "bnUserTypeAnimation.h"
#include "bnScriptedPlayer.h"
#include "bnScriptedComponent.h"
#include "bnScriptedPlayerForm.h"
#include "../bnSolHelpers.h"

void DefineScriptedPlayerUserType(sol::table& battle_namespace) {
  battle_namespace.new_usertype<WeakWrapper<ScriptedPlayer>>("Player",
    sol::meta_function::index, [](WeakWrapper<ScriptedPlayer>& player, const std::string& key) {
      return player.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedPlayer>& player, const std::string& key, sol::stack_object value) {
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
    "get_tile", sol::overload(
      [](WeakWrapper<ScriptedPlayer>& player, Direction dir, unsigned count) -> Battle::Tile* {
        return player.Unwrap()->GetTile(dir, count);
      },
      [](WeakWrapper<ScriptedPlayer>& player) -> Battle::Tile* {
        return player.Unwrap()->GetTile();
      }
    ),
    "get_current_tile", [](WeakWrapper<ScriptedPlayer>& player) -> Battle::Tile* {
      return player.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<ScriptedPlayer>& player) -> WeakWrapper<Field> {
      return WeakWrapper(player.Unwrap()->GetField());
    },
    "set_facing", [](WeakWrapper<ScriptedPlayer>& player, Direction dir) {
      player.Unwrap()->SetFacing(dir);
    },
    "get_facing", [](WeakWrapper<ScriptedPlayer>& player) -> Direction {
      return player.Unwrap()->GetFacing();
    },
    "get_facing_away", [](WeakWrapper<ScriptedPlayer>& player) -> Direction {
      return player.Unwrap()->GetFacingAway();
    },
    "set_elevation", [](WeakWrapper<ScriptedPlayer>& player, float elevation) {
      player.Unwrap()->SetElevation(elevation);
    },
    "get_elevation", [](WeakWrapper<ScriptedPlayer>& player) -> float {
      return player.Unwrap()->GetElevation();
    },
    "get_color", [](WeakWrapper<ScriptedPlayer>& player) -> sf::Color {
      return player.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<ScriptedPlayer>& player, sf::Color color) {
      player.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<ScriptedPlayer>& player) -> WeakWrapper<SpriteProxyNode> {
      return WeakWrapper(std::static_pointer_cast<SpriteProxyNode>(player.Unwrap()));
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
    "set_shadow", sol::overload(
      [](WeakWrapper<ScriptedPlayer>& player, Entity::Shadow type) {
        player.Unwrap()->SetShadowSprite(type);
      },
      [](WeakWrapper<ScriptedPlayer>& player, WeakWrapper<SpriteProxyNode> shadow) {
        player.Unwrap()->SetShadowSprite(shadow.Release());
      }
    ),
    "show_shadow", [](WeakWrapper<ScriptedPlayer>& player, bool enabled) {
      player.Unwrap()->ShowShadow(enabled);
    },
    "raw_move_event", [](WeakWrapper<ScriptedPlayer>& player, const MoveEvent& event, ActionOrder order) -> bool {
      return player.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsSliding();
    },
    "is_jumping", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      return player.Unwrap()->IsJumping();
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
    "will_erase_eof", [](WeakWrapper<ScriptedPlayer>& player) -> bool {
      auto ptr = player.Lock();
      return !ptr || ptr->WillRemoveLater();
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
    "set_health", [](WeakWrapper<ScriptedPlayer>& player, int health) {
      player.Unwrap()->SetHealth(health);
    },
    "get_attack_level", [](WeakWrapper<ScriptedPlayer>& player) -> unsigned int {
      return player.Unwrap()->GetAttackLevel();
    },
    "get_charge_level", [](WeakWrapper<ScriptedPlayer>& player) -> unsigned int {
      return player.Unwrap()->GetChargeLevel();
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
    "set_charge_position", [](WeakWrapper<ScriptedPlayer>& player, float x, float y) {
      player.Unwrap()->SetChargePosition(x, y);
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
    "register_component", [](WeakWrapper<ScriptedPlayer>& player, WeakWrapper<ScriptedComponent>& component) {
      player.Unwrap()->RegisterComponent(component.Release());
    },
    "add_defense_rule", [](WeakWrapper<ScriptedPlayer>& player, DefenseRule* defenseRule) {
      player.Unwrap()->AddDefenseRule(defenseRule->shared_from_this());
    },
    "remove_defense_rule", [](WeakWrapper<ScriptedPlayer>& player, DefenseRule* defenseRule) {
      player.Unwrap()->RemoveDefenseRule(defenseRule);
    },
    "get_offset", [](WeakWrapper<ScriptedPlayer>& player) -> sf::Vector2f {
      return player.Unwrap()->GetDrawOffset();
    },
    "set_offset", [](WeakWrapper<ScriptedPlayer>& player, float x, float y) {
      player.Unwrap()->SetDrawOffset(x, y);
    },
    "get_current_palette",  [](WeakWrapper<ScriptedPlayer>& player) -> std::shared_ptr<Texture> {
      return player.Unwrap()->GetPalette();
    },
    "set_palette",  [](WeakWrapper<ScriptedPlayer>& player, std::shared_ptr<Texture>& texture) {
      player.Unwrap()->SetPalette(texture);
    },
    "get_base_palette",  [](WeakWrapper<ScriptedPlayer>& player) -> std::shared_ptr<Texture> {
      return player.Unwrap()->GetBasePalette();
    },
    "store_base_palette",  [](WeakWrapper<ScriptedPlayer>& player, std::shared_ptr<Texture>& texture) {
      player.Unwrap()->StoreBasePalette(texture);
    },
    "create_form", [](WeakWrapper<ScriptedPlayer>& player) -> WeakWrapperChild<Player, ScriptedPlayerFormMeta> {
      auto parentPtr = player.Unwrap();
      auto formMeta = parentPtr->CreateForm();

      if (!formMeta) {
        throw std::runtime_error("Failed to create form");
      }

      return WeakWrapperChild<Player, ScriptedPlayerFormMeta>(parentPtr, *formMeta);
    },
    "create_node", [](WeakWrapper<ScriptedPlayer>& player) -> WeakWrapper<SpriteProxyNode> {
      auto child = std::make_shared<SpriteProxyNode>();
      player.Unwrap()->AddNode(child);

      return WeakWrapper(child);
    },
    "create_sync_node", [] (WeakWrapper<ScriptedPlayer>& player, const std::string& point) -> std::shared_ptr<SyncNode> {
      return player.Unwrap()->AddSyncNode(point);
    },
    "remove_sync_node", [] (WeakWrapper<ScriptedPlayer>& player, const std::shared_ptr<SyncNode>& node) {
      player.Unwrap()->RemoveSyncNode(node);
    },
    "update_func", sol::property(
      [](WeakWrapper<ScriptedPlayer>& player) { return player.Unwrap()->update_func; },
      [](WeakWrapper<ScriptedPlayer>& player, sol::stack_object value) {
        player.Unwrap()->update_func = VerifyLuaCallback(value);
      }
    ),
    "normal_attack_func", sol::property(
      [](WeakWrapper<ScriptedPlayer>& player) { return player.Unwrap()->normal_attack_func; },
      [](WeakWrapper<ScriptedPlayer>& player, sol::stack_object value) {
        player.Unwrap()->normal_attack_func = VerifyLuaCallback(value);
      }
    ),
    "charged_attack_func", sol::property(
      [](WeakWrapper<ScriptedPlayer>& player) { return player.Unwrap()->charged_attack_func; },
      [](WeakWrapper<ScriptedPlayer>& player, sol::stack_object value) {
        player.Unwrap()->charged_attack_func = VerifyLuaCallback(value);
      }
    ),
    "special_attack_func", sol::property(
      [](WeakWrapper<ScriptedPlayer>& player) { return player.Unwrap()->special_attack_func; },
      [](WeakWrapper<ScriptedPlayer>& player, sol::stack_object value) {
        player.Unwrap()->special_attack_func = VerifyLuaCallback(value);
      }
    )
  );

  battle_namespace.new_usertype<WeakWrapperChild<Player, ScriptedPlayerFormMeta>>("PlayerFormMeta",
    "set_mugshot_texture_path", [] (WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form, const std::string& path) {
      form.Unwrap().SetUIPath(path);
    },
    "calculate_charge_time_func", sol::property(
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form) { return form.Unwrap().calculate_charge_time_func; },
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form, sol::stack_object value) {
        form.Unwrap().calculate_charge_time_func = VerifyLuaCallback(value);
      }
    ),
    "on_activate_func", sol::property(
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form) { return form.Unwrap().on_activate_func; },
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form, sol::stack_object value) {
        form.Unwrap().on_activate_func = VerifyLuaCallback(value);
      }
    ),
    "on_deactivate_func", sol::property(
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form) { return form.Unwrap().on_deactivate_func; },
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form, sol::stack_object value) {
        form.Unwrap().on_deactivate_func = VerifyLuaCallback(value);
      }
    ),
    "update_func", sol::property(
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form) { return form.Unwrap().update_func; },
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form, sol::stack_object value) {
        form.Unwrap().update_func = VerifyLuaCallback(value);
      }
    ),
    "charged_attack_func", sol::property(
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form) { return form.Unwrap().charged_attack_func; },
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form, sol::stack_object value) {
        form.Unwrap().charged_attack_func = VerifyLuaCallback(value);
      }
    ),
    "special_attack_func", sol::property(
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form) { return form.Unwrap().special_attack_func; },
      [](WeakWrapperChild<Player, ScriptedPlayerFormMeta>& form, sol::stack_object value) {
        form.Unwrap().special_attack_func = VerifyLuaCallback(value);
      }
    )
  );

  battle_namespace.new_usertype<ScopedWrapper<ScriptedPlayerForm>>("PlayerForm",
    sol::meta_function::index, [](ScopedWrapper<ScriptedPlayerForm>& form, const std::string& key) {
      return form.Unwrap().dynamic_get(key);
    },
    sol::meta_function::new_index, [](ScopedWrapper<ScriptedPlayerForm>& form, const std::string& key, sol::stack_object value) {
      form.Unwrap().dynamic_set(key, value);
    },
    sol::meta_function::length, [](ScopedWrapper<ScriptedPlayerForm>& form) {
      return form.Unwrap().entries.size();
    }
  );
}
#endif
