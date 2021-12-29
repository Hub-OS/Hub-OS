#ifdef BN_MOD_SUPPORT
#include "bnUserTypeTile.h"

#include "bnUserTypeField.h"
#include "bnWeakWrapper.h"
#include "../bnTile.h"
#include "../bnHitboxSpell.h"
#include "bnScriptedCharacter.h"
#include "bnScriptedPlayer.h"
#include "bnScriptedSpell.h"
#include "bnScriptedObstacle.h"
#include "bnScriptedArtifact.h"
#include "../bnScriptResourceManager.h"

void DefineTileUserType(sol::state& state) {
  state.new_usertype<Battle::Tile>("Tile",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Tile", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Tile", key );
    },
    "x", &Battle::Tile::GetX,
    "y", &Battle::Tile::GetY,
    "width", &Battle::Tile::GetWidth,
    "height", &Battle::Tile::GetHeight,
    "get_state", &Battle::Tile::GetState,
    "set_state", &Battle::Tile::SetState,
    "is_edge", &Battle::Tile::IsEdgeTile,
    "is_cracked", &Battle::Tile::IsCracked,
    "is_hole", &Battle::Tile::IsHole,
    "is_walkable", &Battle::Tile::IsWalkable,
    "is_hidden", &Battle::Tile::IsHidden,
    "is_reserved", &Battle::Tile::IsReservedByCharacter,
    "get_team", &Battle::Tile::GetTeam,
    "set_team", &Battle::Tile::SetTeam,
    "attack_entities", sol::overload(
      [] (Battle::Tile& tile, WeakWrapper<Entity>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<Character>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedCharacter>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<Player>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedPlayer>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedSpell>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<Obstacle>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedObstacle>& e) { return tile.AffectEntities(*e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<HitboxSpell>& e) { return tile.AffectEntities(*e.Unwrap()); }
    ),
    "get_distance_to_tile", &Battle::Tile::Distance,
    "find_characters", [] (
      Battle::Tile& tile,
      sol::stack_object queryObject
    ) {
      // store entities in a temp to avoid issues if the scripter mutates entities in this loop
      std::vector<WeakWrapper<Character>> characters;

      tile.FindCharacters([&characters](std::shared_ptr<Character>& character) -> bool {
        characters.push_back(WeakWrapper(character));
        return false;
      });

      return FilterEntities(characters, queryObject);
    },
    "find_entities", [](
      Battle::Tile& tile,
      sol::stack_object queryObject
    ) {
      // store entities in a temp to avoid issues if the scripter mutates entities in this loop
      std::vector<WeakWrapper<Entity>> entities;

      tile.FindEntities([&entities](std::shared_ptr<Entity>& entity) -> bool {
        entities.push_back(WeakWrapper(entity));
        return false;
      });

      return FilterEntities(entities, queryObject);
    },
    "find_obstacles", [](
      Battle::Tile& tile,
      sol::stack_object queryObject
    ) {
      // store entities in a temp to avoid issues if the scripter mutates entities in this loop
      std::vector<WeakWrapper<Obstacle>> obstacles;

      tile.FindObstacles([&obstacles](std::shared_ptr<Obstacle>& obstacle) -> bool {
        obstacles.push_back(WeakWrapper(obstacle));
        return false;
      });

      return FilterEntities(obstacles, queryObject);
    },
    "highlight", &Battle::Tile::RequestHighlight,
    "get_tile", &Battle::Tile::GetTile,
    "contains_entity", sol::overload(
      [] (Battle::Tile& tile, WeakWrapper<Entity>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<Character>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedCharacter>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<Player>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedPlayer>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedSpell>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedObstacle>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<Obstacle>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedArtifact>& e) { return tile.ContainsEntity(e.Unwrap()); },
      [] (Battle::Tile& tile, WeakWrapper<HitboxSpell>& e) { return tile.ContainsEntity(e.Unwrap()); }
    ),
    "remove_entity_by_id", &Battle::Tile::RemoveEntityByID,
    "reserve_entity_by_id", &Battle::Tile::ReserveEntityByID,
    "add_entity", sol::overload(
      [] (Battle::Tile& tile, WeakWrapper<Entity>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<Character>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedCharacter>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<Player>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedPlayer>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedSpell>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedObstacle>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<Obstacle>& e) { return tile.AddEntity(e.UnwrapAndRelease()); },
      [] (Battle::Tile& tile, WeakWrapper<ScriptedArtifact>& e) { return tile.AddEntity(e.UnwrapAndRelease()); }
    )
  );

  state.new_enum("TileState",
    "Broken", TileState::broken,
    "Cracked", TileState::cracked,
    "DirectionDown", TileState::directionDown,
    "DirectionLeft", TileState::directionLeft,
    "DirectionRight", TileState::directionRight,
    "DirectionUp", TileState::directionUp,
    "Empty", TileState::empty,
    "Grass", TileState::grass,
    "Hidden", TileState::hidden,
    "Holy", TileState::holy,
    "Ice", TileState::ice,
    "Lava", TileState::lava,
    "Normal", TileState::normal,
    "Poison", TileState::poison,
    "Volcano", TileState::volcano
  );
}
#endif
