#include "bnUserTypeField.h"

#include "bnWeakWrapper.h"
#include "../bnField.h"
#include "../bnScriptResourceManager.h"
#include "../bnHitboxSpell.h"
#include "bnScriptedCharacter.h"
#include "bnScriptedPlayer.h"
#include "bnScriptedSpell.h"
#include "bnScriptedObstacle.h"
#include "bnScriptedArtifact.h"

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
      [] (WeakWrapper<Field>& field, WeakWrapper<Entity>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },
      [] (WeakWrapper<Field>& field, WeakWrapper<Character>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedCharacter>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedPlayer>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedSpell>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedObstacle>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedArtifact>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },
      [] (WeakWrapper<Field>& field, WeakWrapper<HitboxSpell>& e, int x, int y) { return field.Unwrap()->AddEntity(e.Release(), x, y); },

      [] (WeakWrapper<Field>& field, WeakWrapper<Entity>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); },
      [] (WeakWrapper<Field>& field, WeakWrapper<Character>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedCharacter>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedPlayer>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedSpell>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedObstacle>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); },
      [] (WeakWrapper<Field>& field, WeakWrapper<ScriptedArtifact>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); },
      [] (WeakWrapper<Field>& field, WeakWrapper<HitboxSpell>& e, Battle::Tile& tile) { return field.Unwrap()->AddEntity(e.Release(), tile); }
    ),
    "get_character", [] (
      WeakWrapper<Field>& field,
      Entity::ID_t ID
    ) -> WeakWrapper<Character> {
      return WeakWrapper(field.Unwrap()->GetCharacter(ID));
    },
    "get_entity", [] (
      WeakWrapper<Field>& field,
      Entity::ID_t ID
    ) -> WeakWrapper<Entity> {
      return WeakWrapper(field.Unwrap()->GetEntity(ID));
    },
    "find_characters", [] (
      WeakWrapper<Field>& field,
      std::function<bool(WeakWrapper<Character>)> query
    ) -> std::vector<WeakWrapper<Character>> {
      auto results = field.Unwrap()->FindCharacters([query] (std::shared_ptr<Character> e) {
        return query(e);
      });

      std::vector<WeakWrapper<Character>> wrappedResults;
      wrappedResults.reserve(results.size());

      for (auto& character : results) {
        wrappedResults.push_back(WeakWrapper(character));
      }

      return wrappedResults;
    },
    "find_nearest_characters", [] (
      WeakWrapper<Field>& field,
      const std::shared_ptr<Character> test,
      std::function<bool(WeakWrapper<Character> e)> query
    ) -> std::vector<WeakWrapper<Character>> {
      auto results = field.Unwrap()->FindNearestCharacters(
        test,
        [query] (std::shared_ptr<Character> e) {
          return query(e);
        }
      );

      std::vector<WeakWrapper<Character>> wrappedResults;
      wrappedResults.reserve(results.size());

      for (auto& character : results) {
        wrappedResults.push_back(WeakWrapper(character));
      }

      return wrappedResults;
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
      const std::function<void(WeakWrapper<Entity>, WeakWrapper<Entity>)>& callback
    ) -> Field::NotifyID_t {
      return field.Unwrap()->NotifyOnDelete(
        target,
        observer,
        [callback] (std::shared_ptr<Entity> target, std::shared_ptr<Entity> observer) {
          callback(target, observer);
        }
      );
    },
    "callback_on_delete", [] (
      WeakWrapper<Field>& field,
      Entity::ID_t target,
      const std::function<void(WeakWrapper<Entity>)>& callback
    ) -> Field::NotifyID_t {
      return field.Unwrap()->CallbackOnDelete(
        target,
        [callback] (std::shared_ptr<Entity> e) { callback(e); }
      );
    }
  );
}
