#include "bnUserTypeScriptedSpell.h"

#include "bnWeakWrapper.h"
#include "bnScriptedSpell.h"
#include "../bnTile.h"

void DefineScriptedSpellUserType(sol::table& battle_namespace) {
  const auto& scriptedspell_record = battle_namespace.new_usertype<WeakWrapper<ScriptedSpell>>( "Spell",
    sol::factories([](Team team) -> WeakWrapper<ScriptedSpell> {
      auto spell = std::make_shared<ScriptedSpell>(team);
      spell->Init();

      auto wrappedSpell = WeakWrapper<ScriptedSpell>(spell);
      wrappedSpell.Own();
      return wrappedSpell;
    }),
    sol::meta_function::index, [](WeakWrapper<ScriptedSpell>& spell, std::string key) {
      return spell.Unwrap()->dynamic_get(key);
    },
    sol::meta_function::new_index, [](WeakWrapper<ScriptedSpell>& spell, std::string key, sol::stack_object value) {
      spell.Unwrap()->dynamic_set(key, value);
    },
    sol::meta_function::length, [](WeakWrapper<ScriptedSpell>& spell) {
      return spell.Unwrap()->entries.size();
    },
    "get_id", [](WeakWrapper<ScriptedSpell>& spell) -> Entity::ID_t {
      return spell.Unwrap()->GetID();
    },
    "get_tile", [](WeakWrapper<ScriptedSpell>& character, Direction dir, unsigned count) -> Battle::Tile* {
      return character.Unwrap()->GetTile(dir, count);
    },
    "get_current_tile", [](WeakWrapper<ScriptedSpell>& spell) -> Battle::Tile* {
      return spell.Unwrap()->GetCurrentTile();
    },
    "get_field", [](WeakWrapper<ScriptedSpell>& spell) -> WeakWrapper<Field> {
      return WeakWrapper(spell.Unwrap()->GetField());
    },
    "get_facing", [](WeakWrapper<ScriptedSpell>& spell) -> Direction {
      return spell.Unwrap()->GetFacing();
    },
    "set_facing", [](WeakWrapper<ScriptedSpell>& spell, Direction direction) {
      spell.Unwrap()->SetFacing(direction);
    },
    "get_alpha", [](WeakWrapper<ScriptedSpell>& spell) -> int {
      return spell.Unwrap()->GetAlpha();
    },
    "set_alpha", [](WeakWrapper<ScriptedSpell>& spell, int alpha) {
      spell.Unwrap()->SetAlpha(alpha);
    },
    "get_color", [](WeakWrapper<ScriptedSpell>& spell) -> sf::Color {
      return spell.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<ScriptedSpell>& spell, sf::Color color) {
      spell.Unwrap()->setColor(color);
    },
    "sprite", [](WeakWrapper<ScriptedSpell>& spell) -> SpriteProxyNode& {
      return spell.Unwrap()->AsSpriteProxyNode();
    },
    "hide", [](WeakWrapper<ScriptedSpell>& spell) {
      spell.Unwrap()->Hide();
    },
    "reveal", [](WeakWrapper<ScriptedSpell>& spell) {
      spell.Unwrap()->Reveal();
    },
    "teleport", [](
      WeakWrapper<ScriptedSpell>& spell,
      Battle::Tile* dest,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return spell.Unwrap()->Teleport(dest, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "slide", [](
      WeakWrapper<ScriptedSpell>& spell,
      Battle::Tile* dest,
      const frame_time_t& slideTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return spell.Unwrap()->Slide(dest, slideTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "jump", [](
      WeakWrapper<ScriptedSpell>& spell,
      Battle::Tile* dest,
      float destHeight,
      const frame_time_t& jumpTime,
      const frame_time_t& endlag,
      ActionOrder order,
      sol::stack_object onBeginObject
    ) -> bool {
      sol::protected_function onBegin = onBeginObject;

      return spell.Unwrap()->Jump(dest, destHeight, jumpTime, endlag, order, [onBegin] {
        auto result = onBegin();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(error.what());
        }
      });
    },
    "raw_move_event", [](WeakWrapper<ScriptedSpell>& spell, const MoveEvent& event, ActionOrder order) -> bool {
      return spell.Unwrap()->RawMoveEvent(event, order);
    },
    "is_sliding", [](WeakWrapper<ScriptedSpell>& spell) -> bool {
      return spell.Unwrap()->IsJumping();
    },
    "is_jumping", [](WeakWrapper<ScriptedSpell>& spell) -> bool {
      return spell.Unwrap()->IsSliding();
    },
    "is_teleporting", [](WeakWrapper<ScriptedSpell>& spell) -> bool {
      return spell.Unwrap()->IsTeleporting();
    },
    "is_moving", [](WeakWrapper<ScriptedSpell>& spell) -> bool {
      return spell.Unwrap()->IsMoving();
    },
    "is_deleted", [](WeakWrapper<ScriptedSpell>& spell) -> bool {
      return spell.Unwrap()->IsDeleted();
    },
    "will_remove_eof", [](WeakWrapper<ScriptedSpell>& spell) -> bool {
      return spell.Unwrap()->WillRemoveLater();
    },
    "is_team", [](WeakWrapper<ScriptedSpell>& spell, Team team) -> bool {
      return spell.Unwrap()->Teammate(team);
    },
    "get_team", [](WeakWrapper<ScriptedSpell>& spell) -> Team {
      return spell.Unwrap()->GetTeam();
    },
    "remove", [](WeakWrapper<ScriptedSpell>& spell) {
      spell.Unwrap()->Remove();
    },
    "delete", [](WeakWrapper<ScriptedSpell>& spell) {
      spell.Unwrap()->Delete();
    },
    "get_texture", [](WeakWrapper<ScriptedSpell>& spell) -> std::shared_ptr<Texture> {
      return spell.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<ScriptedSpell>& spell, std::shared_ptr<Texture> texture) {
      spell.Unwrap()->setTexture(texture);
    },
    "set_animation", [](WeakWrapper<ScriptedSpell>& spell, std::string animation) {
      spell.Unwrap()->SetAnimation(animation);
    },
    "get_animation", [](WeakWrapper<ScriptedSpell>& spell) -> Animation& {
      return spell.Unwrap()->GetAnimationObject();
    },
    "add_node", [](WeakWrapper<ScriptedSpell>& spell, SceneNode* node) {
      spell.Unwrap()->AddNode(node);
    },
    "highlight_tile", [](WeakWrapper<ScriptedSpell>& spell, Battle::TileHighlight mode) {
      spell.Unwrap()->HighlightTile(mode);
    },
    "copy_hit_props", [](WeakWrapper<ScriptedSpell>& spell) -> Hit::Properties {
      return spell.Unwrap()->GetHitboxProperties();
    },
    "set_hit_props", [](WeakWrapper<ScriptedSpell>& spell, Hit::Properties props) {
      spell.Unwrap()->SetHitboxProperties(props);
    },
    "set_height", [](WeakWrapper<ScriptedSpell>& spell, float height) {
      spell.Unwrap()->SetHeight(height);
    },
    "show_shadow", [](WeakWrapper<ScriptedSpell>& spell, bool show) {
      spell.Unwrap()->ShowShadow(show);
    },
    "shake_camera", [](WeakWrapper<ScriptedSpell>& spell, double power, float duration) {
      spell.Unwrap()->ShakeCamera(power, duration);
    },
    "get_position", [](WeakWrapper<ScriptedSpell>& spell) -> sf::Vector2f {
      return spell.Unwrap()->GetDrawOffset();
    },
    "set_position", [](WeakWrapper<ScriptedSpell>& spell, float x, float y) {
      spell.Unwrap()->SetDrawOffset(x, y);
    },
    "never_flip", [](WeakWrapper<ScriptedSpell>& spell, bool enabled) {
      spell.Unwrap()->NeverFlip(enabled);
    }
  );
}
