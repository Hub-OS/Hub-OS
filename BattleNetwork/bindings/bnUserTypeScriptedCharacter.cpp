#include "bnUserTypeScriptedCharacter.h"

#include "bnWeakWrapper.h"
#include "bnScriptedCharacter.h"
#include "../bnTile.h"

void DefineScriptedCharacterUserType(sol::table& battle_namespace) {
  battle_namespace.new_usertype<WeakWrapper<ScriptedCharacter>>("Character",
    sol::meta_function::index, [](WeakWrapper<ScriptedCharacter>& character, std::string key) {
      return character.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedCharacter>& character, std::string key, sol::stack_object value) {
      character.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedCharacter>& character) {
      return character.Unwrap()->entries.size();
    },
    "get_id", [](WeakWrapper<ScriptedCharacter>& character) -> Entity::ID_t {
      return character.Unwrap()->GetID();
    },
    "get_element", [](WeakWrapper<ScriptedCharacter>& character) -> Element {
      return character.Unwrap()->GetElement();
    },
    "set_element", [](WeakWrapper<ScriptedCharacter>& character, Element element) {
      character.Unwrap()->SetElement(element);
    },
    "get_tile", [](WeakWrapper<ScriptedCharacter>& character, Direction dir, unsigned count) -> Battle::Tile* {
      return character.Unwrap()->GetTile(dir, count);
    },
    "get_current_tile", [](WeakWrapper<ScriptedCharacter>& character) -> Battle::Tile* {
      return character.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<ScriptedCharacter>& character) -> WeakWrapper<Field> {
      return WeakWrapper(character.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<ScriptedCharacter>& character) -> Direction {
      return character.Unwrap()->GetFacing();
    },
    "get_target", [](WeakWrapper<ScriptedCharacter>& character) -> std::shared_ptr<Character> {
      return character.Unwrap()->GetTarget();
    },
    "get_alpha", [](WeakWrapper<ScriptedCharacter>& character) -> int {
      return character.Unwrap()->GetAlpha();
    },
    "set_alpha", [](WeakWrapper<ScriptedCharacter>& character, int alpha) {
      character.Unwrap()->SetAlpha(alpha);
    },
    "get_color", [](WeakWrapper<ScriptedCharacter>& character) -> sf::Color {
      return character.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<ScriptedCharacter>& character, sf::Color color) {
      character.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<ScriptedCharacter>& character) -> SpriteProxyNode& {
      return character.Unwrap()->AsSpriteProxyNode();
    },
    "hide", [](WeakWrapper<ScriptedCharacter>& character) {
      character.Unwrap()->Hide();
    },
    "reveal", [](WeakWrapper<ScriptedCharacter>& character) {
      character.Unwrap()->Reveal();
    },
    "teleport", [](
      WeakWrapper<ScriptedCharacter>& character,
      Battle::Tile* dest,
      ActionOrder order,
      std::function<void()> onBegin
    ) -> bool {
      return character.Unwrap()->Teleport(dest, order, onBegin);
    },
    "slide", [](
      WeakWrapper<ScriptedCharacter>& character,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      std::function<void()> onBegin
    ) -> bool {
      return character.Unwrap()->Slide(dest, slideTime, endlag, order, onBegin);
    },
    "jump", [](
      WeakWrapper<ScriptedCharacter>& character,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      std::function<void()> onBegin
    ) -> bool {
      return character.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, onBegin);
    },
    "raw_move_event", [](WeakWrapper<ScriptedCharacter>& character, const MoveEvent& event, ActionOrder order) -> bool {
      return character.Unwrap()->RawMoveEvent(event, order);
    },
    "card_action_event", sol::overload(
      [](WeakWrapper<ScriptedCharacter> character, std::shared_ptr<ScriptedCardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->SimpleCardActionEvent(cardAction, order);
      },
      [](WeakWrapper<ScriptedCharacter> character,std::shared_ptr<CardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->SimpleCardActionEvent(cardAction, order);
      }
    ),
    "is_sliding", [](WeakWrapper<ScriptedCharacter>& character) -> bool {
      return character.Unwrap()->IsJumping();
    },
    "is_jumping", [](WeakWrapper<ScriptedCharacter>& character) -> bool {
      return character.Unwrap()->IsSliding();
    },
    "is_teleporting", [](WeakWrapper<ScriptedCharacter>& character) -> bool {
      return character.Unwrap()->IsTeleporting();
    },
    "is_passthrough", [](WeakWrapper<ScriptedCharacter>& character) -> bool {
      return character.Unwrap()->IsPassthrough();
    },
    "is_moving", [](WeakWrapper<ScriptedCharacter>& character) -> bool {
      return character.Unwrap()->IsMoving();
    },
    "is_deleted", [](WeakWrapper<ScriptedCharacter>& character) -> bool {
      return character.Unwrap()->IsDeleted();
    },
    "will_remove_eof", [](WeakWrapper<ScriptedCharacter>& character) -> bool {
      return character.Unwrap()->WillRemoveLater();
    },
    "get_team", [](WeakWrapper<ScriptedCharacter>& character) -> Team {
      return character.Unwrap()->GetTeam();
    },
    "is_team", [](WeakWrapper<ScriptedCharacter>& character, Team team) {
      character.Unwrap()->Teammate(team);
    },
    "remove", [](WeakWrapper<ScriptedCharacter>& character) {
      character.Unwrap()->Remove();
    },
    "delete", [](WeakWrapper<ScriptedCharacter>& character) {
      character.Unwrap()->Delete();
    },
    "get_texture", [](WeakWrapper<ScriptedCharacter>& character) -> std::shared_ptr<Texture> {
      return character.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<ScriptedCharacter>& character, std::shared_ptr<Texture> texture) {
      character.Unwrap()->setTexture(texture);
    },
    "add_node", [](WeakWrapper<ScriptedCharacter>& character, SceneNode* node) {
      character.Unwrap()->AddNode(node);
    },
    "get_name", [](WeakWrapper<ScriptedCharacter>& character) -> std::string {
      return character.Unwrap()->GetName();
    },
    "set_name", [](WeakWrapper<ScriptedCharacter>& character, std::string name) {
      character.Unwrap()->SetName(name);
    },
    "get_health", [](WeakWrapper<ScriptedCharacter>& character) -> int {
      return character.Unwrap()->GetHealth();
    },
    "get_max_health", [](WeakWrapper<ScriptedCharacter>& character) -> int {
      return character.Unwrap()->GetMaxHealth();
    },
    "set_health", [](WeakWrapper<ScriptedCharacter>& character, int health) {
      character.Unwrap()->SetHealth(health);
    },
    "get_rank", [](WeakWrapper<ScriptedCharacter>& character) -> Character::Rank {
      return character.Unwrap()->GetRank();
    },
    "share_tile", [](WeakWrapper<ScriptedCharacter>& character, bool share) {
      character.Unwrap()->ShareTileSpace(share);
    },
    "register_component", [](WeakWrapper<ScriptedCharacter>& character, std::shared_ptr<Component> component) {
      character.Unwrap()->RegisterComponent(component);
    },
    "add_defense_rule", [](WeakWrapper<ScriptedCharacter>& character, DefenseRule* defenseRule) {
      character.Unwrap()->AddDefenseRule(defenseRule->shared_from_this());
    },
    "remove_defense_rule", [](WeakWrapper<ScriptedCharacter>& character, DefenseRule* defenseRule) {
      character.Unwrap()->RemoveDefenseRule(defenseRule);
    },
    "get_position", [](WeakWrapper<ScriptedCharacter>& character) -> sf::Vector2f {
      return character.Unwrap()->GetDrawOffset();
    },
    "set_position", [](WeakWrapper<ScriptedCharacter>& character, float x, float y) {
      character.Unwrap()->SetDrawOffset(x, y);
    },
    "set_height", [](WeakWrapper<ScriptedCharacter>& character, float height) {
      character.Unwrap()->SetHeight(height);
    },
    "get_animation", [](WeakWrapper<ScriptedCharacter>& character) -> Animation& {
      return character.Unwrap()->GetAnimationObject();
    },
    "shake_camera", [](WeakWrapper<ScriptedCharacter>& character, double power, float duration) {
      character.Unwrap()->ShakeCamera(power, duration);
    },
    "toggle_counter", [](WeakWrapper<ScriptedCharacter>& character, bool on) {
      character.Unwrap()->ToggleCounter(on);
    },
    "register_status_callback", [](WeakWrapper<ScriptedCharacter>& character, const Hit::Flags& flag, const std::function<void()>& callback) {
      character.Unwrap()->RegisterStatusCallback(flag, callback);
    },
    "delete_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) {
        return character.Unwrap()->deleteCallback;
      },
      [](WeakWrapper<ScriptedCharacter>& character, std::function<void(WeakWrapper<ScriptedCharacter>)> callback) {
        character.Unwrap()->deleteCallback = callback;
      }
    ),
    "update_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) {
        return character.Unwrap()->updateCallback;
      },
      [](WeakWrapper<ScriptedCharacter>& character, std::function<void(WeakWrapper<ScriptedCharacter>, double)> callback) {
        character.Unwrap()->updateCallback = callback;
      }
    ),
    "can_move_to_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) {
        return character.Unwrap()->canMoveToCallback;
      },
      [](WeakWrapper<ScriptedCharacter>& character, std::function<bool(Battle::Tile&)> callback) {
        character.Unwrap()->canMoveToCallback = callback;
      }
    ),
    "on_spawn_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) {
        return character.Unwrap()->spawnCallback;
      },
      [](WeakWrapper<ScriptedCharacter>& character, std::function<void(WeakWrapper<ScriptedCharacter>, Battle::Tile&)> callback) {
        character.Unwrap()->spawnCallback = callback;
      }
    ),
    "battle_start_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) {
        return character.Unwrap()->onBattleStartCallback;
      },
      [](WeakWrapper<ScriptedCharacter>& character, std::function<void(WeakWrapper<ScriptedCharacter>)> callback) {
        character.Unwrap()->onBattleStartCallback = callback;
      }
    ),
    "battle_end_func", sol::property(
      [](WeakWrapper<ScriptedCharacter>& character) {
        return character.Unwrap()->onBattleEndCallback;
      },
      [](WeakWrapper<ScriptedCharacter>& character, std::function<void(WeakWrapper<ScriptedCharacter>)> callback) {
        character.Unwrap()->onBattleEndCallback = callback;
      }
    ),
    "set_explosion_behavior", [](WeakWrapper<ScriptedCharacter>& character, int num, double speed, bool isBoss) {
      character.Unwrap()->SetExplosionBehavior(num, speed, isBoss);
    }
  );
}
