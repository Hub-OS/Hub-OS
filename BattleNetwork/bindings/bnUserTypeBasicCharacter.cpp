#include "bnUserTypeBasicCharacter.h"

#include "bnWeakWrapper.h"
#include "../bnCharacter.h"

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
    "get_tile", [](WeakWrapper<Character>& character, Direction dir, unsigned count) -> Battle::Tile* {
      return character.Unwrap()->GetTile(dir, count);
    },
    "get_current_tile", [](WeakWrapper<Character>& character) -> Battle::Tile* {
      return character.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<Character>& character) -> WeakWrapper<Field> {
      return WeakWrapper(character.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<Character>& character) -> Direction {
      return character.Unwrap()->GetFacing();
    },
    "get_alpha", [](WeakWrapper<Character>& character) -> int {
      return character.Unwrap()->GetAlpha();
    },
    "set_alpha", [](WeakWrapper<Character>& character, int alpha) {
      character.Unwrap()->SetAlpha(alpha);
    },
    "get_color", [](WeakWrapper<Character>& character) -> sf::Color {
      return character.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<Character>& character, sf::Color color) {
      character.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<Character>& character) -> SpriteProxyNode& {
      return character.Unwrap()->AsSpriteProxyNode();
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
      std::function<void()> onBegin
    ) -> bool {
      return character.Unwrap()->Teleport(dest, order, onBegin);
    },
    "slide", [](
      WeakWrapper<Character>& character,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      std::function<void()> onBegin
    ) -> bool {
      return character.Unwrap()->Slide(dest, slideTime, endlag, order, onBegin);
    },
    "jump", [](
      WeakWrapper<Character>& character,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      std::function<void()> onBegin
    ) -> bool {
      return character.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, onBegin);
    },
    "raw_move_event", [](WeakWrapper<Character>& character, const MoveEvent& event, ActionOrder order) -> bool {
      return character.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->IsJumping();
    },
    "is_jumping", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->IsSliding();
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
      return character.Unwrap()->IsDeleted();
    },
    "will_remove_eof", [](WeakWrapper<Character>& character) -> bool {
      return character.Unwrap()->WillRemoveLater();
    },
    "get_team", [](WeakWrapper<Character>& character) -> Team {
      return character.Unwrap()->GetTeam();
    },
    "is_team", [](WeakWrapper<Character>& character, Team team) {
      character.Unwrap()->Teammate(team);
    },
    "get_texture", [](WeakWrapper<Character>& character) -> std::shared_ptr<Texture> {
      return character.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<Character>& character, std::shared_ptr<Texture> texture) {
      character.Unwrap()->setTexture(texture);
    },
    "add_node", [](WeakWrapper<Character>& character, SceneNode* node) {
      character.Unwrap()->AddNode(node);
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
    "register_component", [](WeakWrapper<Character>& character, std::shared_ptr<Component> component) {
      character.Unwrap()->RegisterComponent(component);
    },
    "remove_defense_rule", [](WeakWrapper<Character>& character, DefenseRule* defenseRule) {
      character.Unwrap()->RemoveDefenseRule(defenseRule);
    },
    "get_position", [](WeakWrapper<Character>& character) -> sf::Vector2f {
      return character.Unwrap()->GetDrawOffset();
    },
    "set_position", [](WeakWrapper<Character>& character, float x, float y) {
      character.Unwrap()->SetDrawOffset(x, y);
    },
    "set_height", [](WeakWrapper<Character>& character, float height) {
      character.Unwrap()->SetHeight(height);
    },
    "toggle_counter", [](WeakWrapper<Character>& character, bool on) {
      character.Unwrap()->ToggleCounter(on);
    },
    "get_animation", [](WeakWrapper<Character>& character) -> Animation* {
      return character.Unwrap()->GetAnimationFromComponent();
    }
  );
}