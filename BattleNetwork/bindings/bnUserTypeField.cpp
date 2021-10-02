#include "bnUserTypeField.h"

#include "bnWeakWrapper.h"
#include "../bnField.h"
#include "../bnScriptResourceManager.h"

void DefineFieldUserType(sol::table& battle_namespace) {
  // Exposed "GetCharacter" so that there's a way to maintain a reference to other actors without hanging onto pointers.
  // If you hold onto their ID, and use that through Field::GetCharacter,
  // instead you will get passed a nullptr/nil in Lua after they've been effectively removed,
  // rather than potentially holding onto a hanging pointer to deleted data.
  const auto& field_record = battle_namespace.new_usertype<WeakWrapper<Field>>("Field",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Field", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Field", key );
    },
    "tile_at", [] (
      WeakWrapper<Field>& field,
      int x,
      int y
    ) -> Battle::Tile* {
      return field.Unwrap()->GetAt(x, y);
    },
    "width", [] (WeakWrapper<Field>& field) -> int {
      return field.Unwrap()->GetWidth();
    },
    "height", [] (WeakWrapper<Field>& field) -> int {
      return field.Unwrap()->GetHeight();
    },
    "spawn", sol::overload(
      [](WeakWrapper<Field>& field, Entity* e, int x, int y) { return field.Unwrap()->AddEntity(e->shared_from_this(), x, y); },
      [](WeakWrapper<Field>& field, Entity* e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e->shared_from_this(), tile); }
    ),
    "get_character", [] (
      WeakWrapper<Field>& field,
      Entity::ID_t ID
    ) -> std::shared_ptr<Character> {
      return field.Unwrap()->GetCharacter(ID);
    },
    "get_entity", [] (
      WeakWrapper<Field>& field,
      Entity::ID_t ID
    ) -> std::shared_ptr<Entity> {
      return field.Unwrap()->GetEntity(ID);
    },
    "find_characters", [] (
      WeakWrapper<Field>& field,
      std::function<bool(std::shared_ptr<Character> e)> query
    ) -> std::vector<std::shared_ptr<Character>> {
      return field.Unwrap()->FindCharacters(query);
    },
    "find_nearest_characters", [] (
      WeakWrapper<Field>& field,
      const std::shared_ptr<Character> test,
      std::function<bool(std::shared_ptr<Character> e)> query
    ) -> std::vector<std::shared_ptr<Character>> {
      return field.Unwrap()->FindNearestCharacters(test, query);
    },
    "find_tiles", [] (
      WeakWrapper<Field>& field,
      std::function<bool(Battle::Tile* t)> query
    ) -> std::vector<Battle::Tile*> {
      return field.Unwrap()->FindTiles(query);
    },
    "notify_on_delete", [] (
      WeakWrapper<Field>& field,
      Entity::ID_t target,
      Entity::ID_t observer,
      const std::function<void(std::shared_ptr<Entity>, std::shared_ptr<Entity>)>& callback
    ) -> Field::NotifyID_t {
      return field.Unwrap()->NotifyOnDelete(target, observer, callback);
    },
    "callback_on_delete", [] (
      WeakWrapper<Field>& field,
      Entity::ID_t target,
      const std::function<void(std::shared_ptr<Entity>)>& callback
    ) -> Field::NotifyID_t {
      return field.Unwrap()->CallbackOnDelete(target, callback);
    }
  );
}
