#ifdef BN_MOD_SUPPORT
#include "bnUserTypeBasicCharacter.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeAnimation.h"
#include "bnScriptedComponent.h"
#include "bnScriptedCardAction.h"
#include "../bnCharacter.h"
#include <optional>

void DefineBasicCharacterUserType(sol::table& battle_namespace) {
  battle_namespace.new_usertype<WeakWrapper<Character>>( "BasicCharacter",
    "get_id", [](WeakWrapper<Character>& character) -> Entity::ID_t {
      return character.Unwrap()->GetID();
    },
    "get_element", [](WeakWrapper<Character>& character) -> Element {
      return character.Unwrap()->GetElement();
    },
    "set_element", [](WeakWrapper<Character>& character, Element element) {
      character.Unwrap()->SetElement(element);
    },
    "get_tile", sol::overload(
      [](WeakWrapper<Character>& character, Direction dir, unsigned count) -> Battle::Tile* {
        return character.Unwrap()->GetTile(dir, count);
      },
      [](WeakWrapper<Character>& character) -> Battle::Tile* {
        return character.Unwrap()->GetTile();
      }
    ),
    "input_has", [](WeakWrapper<Character>& character, const InputEvent& event) -> bool {
      return character.Unwrap()->InputState().Has(event);
    },
    "get_current_tile", [](WeakWrapper<Character>& character) -> Battle::Tile* {
      return character.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<Character>& character) -> WeakWrapper<Field> {
      return WeakWrapper(character.Unwrap()->GetField());
    },
    "set_facing", [](WeakWrapper<Character>& character, Direction dir) {
      character.Unwrap()->SetFacing(dir);
    },
    "get_facing", [](WeakWrapper<Character>& character) -> Direction {
      return character.Unwrap()->GetFacing();
    },
    "get_facing_away", [](WeakWrapper<Character>& character) -> Direction {
      return character.Unwrap()->GetFacingAway();
    },
    "get_color", [](WeakWrapper<Character>& character) -> sf::Color {
      return character.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<Character>& character, sf::Color color) {
      character.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<Character>& character) -> WeakWrapper<SpriteProxyNode> {
      return WeakWrapper(std::static_pointer_cast<SpriteProxyNode>(character.Unwrap()));
    },
    "hide", [](WeakWrapper<Character>& character) {
      character.Unwrap()->Hide();
    },
    "reveal", [](WeakWrapper<Character>& character) {
      character.Unwrap()->Reveal();
    },
    "teleport", [](
      WeakWrapper<Character>& character,
      Battle::Tile* dest,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return character.Unwrap()->Teleport(dest, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "slide", [](
      WeakWrapper<Character>& character,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return character.Unwrap()->Slide(dest, slideTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "jump", [](
      WeakWrapper<Character>& character,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return character.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "raw_move_event", [](WeakWrapper<Character>& character, const MoveEvent& event, ActionOrder order) -> bool {
      return character.Unwrap()->RawMoveEvent(event, order);
    },
    "card_action_event", sol::overload(
      [](WeakWrapper<Character>& character, WeakWrapper<ScriptedCardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->AddAction(CardEvent{ cardAction.Release() }, order);
      },
      [](WeakWrapper<Character>& character, WeakWrapper<CardAction>& cardAction, ActionOrder order) {
        character.Unwrap()->AddAction(CardEvent{ cardAction.Release() }, order);
      }
    ),
    "is_sliding", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->IsSliding();
    },
    "is_jumping", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->IsJumping();
    },
    "is_teleporting", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->IsTeleporting();
    },
    "is_passthrough", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->IsPassthrough();
    },
    "is_moving", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->IsMoving();
    },
    "is_deleted", [](WeakWrapper<Character>& character) -> bool {
      auto ptr = character.Lock();
      return !ptr || ptr->IsDeleted();
    },
    "will_erase_eof", [](WeakWrapper<Character>& character) -> bool {
      auto ptr = character.Lock();
      return !ptr || ptr->WillEraseEOF();
    },
    "get_team", [](WeakWrapper<Character>& character) -> Team {
      return character.Unwrap()->GetTeam();
    },
    "is_team", [](WeakWrapper<Character>& character, Team team) -> bool {
      return character.Unwrap()->Teammate(team);
    },
    "get_texture", [](WeakWrapper<Character>& character) -> std::shared_ptr<Texture> {
      return character.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<Character>& character, std::shared_ptr<Texture> texture) {
      character.Unwrap()->setTexture(texture);
    },
    "create_node", [](WeakWrapper<Character>& character) -> WeakWrapper<SpriteProxyNode> {
      auto child = std::make_shared<SpriteProxyNode>();
      child->SetColorMode(ColorMode::additive);
      child->setColor(NoopCompositeColor(ColorMode::additive));
      character.Unwrap()->AddNode(child);

      return WeakWrapper(child);
    },
    "get_name", [](WeakWrapper<Character>& character) -> std::string {
      return character.Unwrap()->GetName();
    },
    "set_name", [](WeakWrapper<Character>& character, std::string name) {
      character.Unwrap()->SetName(name);
    },
    "get_health", [](WeakWrapper<Character>& character) -> int{
      return character.Unwrap()->GetHealth();
    },
    "get_max_health", [](WeakWrapper<Character>& character) -> int {
      return character.Unwrap()->GetMaxHealth();
    },
    "set_health", [](WeakWrapper<Character>& character, int health) {
      character.Unwrap()->SetHealth(health);
    },
    "get_rank", [](WeakWrapper<Character>& character) -> Character::Rank {
      return character.Unwrap()->GetRank();
    },
    "share_tile", [](WeakWrapper<Character>& character, bool share) {
      character.Unwrap()->ShareTileSpace(share);
    },
    "add_defense_rule", [](WeakWrapper<Character>& character, DefenseRule* defenseRule) {
      character.Unwrap()->AddDefenseRule(defenseRule->shared_from_this());
    },
    "register_component", [](WeakWrapper<Character>& character, WeakWrapper<ScriptedComponent>& component) {
      character.Unwrap()->RegisterComponent(component.Release());
    },
    "remove_defense_rule", [](WeakWrapper<Character>& character, DefenseRule* defenseRule) {
      character.Unwrap()->RemoveDefenseRule(defenseRule);
    },
    "get_offset", [](WeakWrapper<Character>& character) -> sf::Vector2f {
      return character.Unwrap()->GetDrawOffset();
    },
    "set_offset", [](WeakWrapper<Character>& character, float x, float y) {
      character.Unwrap()->SetDrawOffset(x, y);
    },
    "set_height", [](WeakWrapper<Character>& character, float height) {
      character.Unwrap()->SetHeight(height);
    },
    "set_shadow", sol::overload(
      [](WeakWrapper<Character>& character, Entity::Shadow type) {
        character.Unwrap()->SetShadowSprite(type);
      },
      [](WeakWrapper<Character>& character, std::shared_ptr<sf::Texture> shadow) {
        character.Unwrap()->SetShadowSprite(shadow);
      }
    ),
    "show_shadow", [](WeakWrapper<Character>& character, bool enabled) {
      character.Unwrap()->ShowShadow(enabled);
    },
    "set_elevation", [](WeakWrapper<Character>& character, float elevation) {
      character.Unwrap()->SetElevation(elevation);
    },
    "get_elevation", [](WeakWrapper<Character>& character) -> float {
      return character.Unwrap()->GetElevation();
    },
    "toggle_counter", [](WeakWrapper<Character>& character, bool on) {
      character.Unwrap()->ToggleCounter(on);
    },
    "get_animation", [](WeakWrapper<Character>& character) -> std::optional<AnimationWrapper> {
      auto characterPtr = character.Unwrap();

      if (auto anim = characterPtr->GetFirstComponent<AnimationComponent>()) {
        return AnimationWrapper(characterPtr, anim->GetAnimationObject());
      }

      return {};
    }
  );
}
#endif