#ifdef BN_MOD_SUPPORT
#include <memory>
#include <vector>
#include <functional>
#include "bnScriptResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnResourceHandle.h"
#include "bnCardPackageManager.h"
#include "bnPlayerPackageManager.h"
#include "bnMobPackageManager.h"
#include "bnBlockPackageManager.h"
#include "bnLuaLibraryPackageManager.h"

#include "bnAnimator.h"
#include "bnEntity.h"
#include "bnSpell.h"
#include "bnCharacter.h"
#include "bnElements.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnHitbox.h"
#include "bnSharedHitbox.h"
#include "bnDefenseNodrag.h"
#include "bnDefenseVirusBody.h"
#include "bnParticlePoof.h"
#include "bnParticleImpact.h"
#include "bnPlayerCustScene.h"

#include "bindings/bnLuaLibrary.h"
#include "bindings/bnScriptedArtifact.h"
#include "bindings/bnScriptedCardAction.h"
#include "bindings/bnScriptedCharacter.h"
#include "bindings/bnScriptedSpell.h"
#include "bindings/bnScriptedObstacle.h"
#include "bindings/bnScriptedPlayer.h"
#include "bindings/bnScriptedPlayerForm.h"
#include "bindings/bnScriptedDefenseRule.h"
#include "bindings/bnScriptedMob.h"
#include "bindings/bnScriptedCard.h"
#include "bindings/bnScriptedComponent.h"
#include "bindings/bnScriptedBlock.h"

// Useful prefabs to use in scripts...
#include "bnExplosion.h"

// temporary proof of concept includes...
#include "bnBusterCardAction.h"
#include "bnSwordCardAction.h"
#include "bnBombCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnCannonCardAction.h"

namespace {
  int exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
    if (maybe_exception) {
      const std::exception& ex = *maybe_exception;
      Logger::Log(ex.what());
    }
    else {
      std::string message(description.data(), description.size());
      Logger::Log(message.c_str());
    }

    return sol::stack::push(L, description);
  }
}

// Creates a std::list<OverrideFrame> object from a provided table.
// Expected format : { { unsigned int state_in_animation, double duration }, ... }
std::list<OverrideFrame> CreateFrameData( sol::lua_table table )
{
  std::list<OverrideFrame> frames;

  auto count = table.size();

  for( int ind = 1; ind <= count; ++ind )
  {
    unsigned animStateNumber = table.traverse_get<unsigned>(ind, 1);
    double duration = table.traverse_get<double>(ind, 2);

    frames.emplace_back( OverrideFrame { animStateNumber, duration } );
  }

  return frames;
}

void ScriptResourceManager::SetSystemFunctions( sol::state* state )
{
  state->open_libraries(sol::lib::base, sol::lib::debug, sol::lib::math, sol::lib::table);

  // 'include()' in Lua is intended to load a LIBRARY file of Lua code into the current lua state.
  // Currently only loads library files included in the SAME directory as the script file.
  // Has to capture a pointer to sol::state, the copy constructor was deleted, cannot capture a copy to reference.
  state->set_function( "include",
    [this, state]( const std::string fileName ) -> sol::protected_function_result {
      #ifdef _DEBUG
      std::cout << "Including library: " << fileName << std::endl;
      #endif

      sol::protected_function_result result;

        // Prefer using the shared libraries if possible.
        // i.e. ones that were present in "mods/libs/"
      if( auto sharedLibPath = FetchSharedLibraryPath( fileName ); !sharedLibPath.empty() )
      {
        #ifdef _DEBUG
        Logger::Log( "Including shared library: " + fileName);
        #endif
        
        AddDependencyNote( state, fileName );

        result = state->do_file( sharedLibPath, sol::load_mode::any );
      }
      else
      {
        #ifdef _DEBUG
        std::cout << "Including local library: " << fileName << std::endl;
        #endif

        std::string modPathRef = (*state)["_modpath"];
        result = state->do_file( modPathRef + fileName, sol::load_mode::any );
      }

      return result;
    }
  );
}

void ScriptResourceManager::AddDependencyNote( sol::state* state, const std::string& dependencyPackageID )
{
    // If "__dependencies" doesn't exist already, create it. We need it to exist.
  if( ! (*state)["__dependencies"].valid() )
    state->create_table("__dependencies");

  sol::lua_table t = (*state)["__dependencies"];

    // Add the package dependency to the table.
  t.set( t.size() + 1, dependencyPackageID );
}

void ScriptResourceManager::RegisterDependencyNotes( sol::state* state )
{
    // If "_package_id" isn't set, something's gone wrong, return.
  if( ! (*state)["_package_id"].valid() )
  {
    #ifdef _DEBUG
    Logger::Log( "-- No Valid Package ID" );
    #endif
    return;
  }
  
    // Retrieve the package ID from the state.
  std::string packageID = (*state)["_package_id"];

    // If this doesn't have any dependencies (no entries in "__dependencies" from earlier), return.
  if( ! (*state)["__dependencies"].valid() )
  {
    #ifdef _DEBUG
    Logger::Log( "-- " + packageID + ": No Dependency Table" );
    #endif
    return;
  }

    // Retrieve the table of dependencies set from earlier.
  sol::lua_table deps = (*state)["__dependencies"];
    // Get the number of keys in that table, to iterate over.
  auto count = deps.size();

  std::list<std::string> dependencies;

  std::string depsString = "";

  for( int ind = 1; ind <= count; ++ind )
  {
    depsString = depsString + deps.get<std::string>(ind) + ", ";

      // Get the KEY of each member in the table, as those are the package IDs that this depends on.
      // Add them to the list.
    dependencies.emplace_back( deps.get<std::string>(ind) );
  }

  #ifdef _DEBUG
  Logger::Log( "Package: " + packageID );
  Logger::Log( "- Depends on " + depsString );
  #endif

    // Register the dependencies with the list in ScriptResourceManager.
  scriptDependencies[packageID] = dependencies;

    // Remove the "__dependencies" table secretly inserted into the sol state, we don't need it anymore.
  (*state)["__dependencies"] = sol::nil;
}

void ScriptResourceManager::SetModPathVariable( sol::state* state, const std::filesystem::path& modDirectory )
{
  (*state)["_modpath"] = modDirectory.generic_string() + "/";
}

// Free Function provided to a number of Lua types that will print an error message when attempting to access a key that does not exist.
// Will print the file and line the error occured in, as well as the invalid key and type. 
sol::object ScriptResourceManager::PrintInvalidAccessMessage( sol::table table, const std::string typeName, const std::string key )
{
  Logger::Log( "[Script Error] in " + GetCurrentLine( table.lua_state() ) );
  Logger::Log( "[Script Error] : Attempted to access \"" + key + "\" in type \"" + typeName + "\"." );
  Logger::Log( "[Script Error] : " + key + " does not exist in " + typeName + "." );
  return sol::lua_nil;
}
// Free Function provided to a number of Lua types that will print an error message when attempting to assign to a key that exists in a system type.
// Will print the file and line the error occured in, as well as the invalid key and type. 
sol::object ScriptResourceManager::PrintInvalidAssignMessage( sol::table table, const std::string typeName, const std::string key )
{
  Logger::Log( "[Script Error] in " + GetCurrentLine( table.lua_state() ) );
  Logger::Log( "[Script Error] : Attempted to assign to \"" + key + "\" in type \"" + typeName + "\"." );
  Logger::Log( "[Script Error] : " + typeName + " is read-only. Cannot assign new values to it." );
  return sol::lua_nil;
}// Returns the current executing line in the Lua script.
// Format: \@[full_filename]:[line_number]
std::string ScriptResourceManager::GetCurrentLine( lua_State* L )
{
  lua_getglobal( L, "debug" );          // debug
  lua_getfield( L, -1, "getinfo" );     // debug.getinfo 
  lua_pushinteger( L, 2 );              // debug.getinfo ( 2 )
  lua_pushstring( L, "S" );             // debug.getinfo ( 2, "S" )

  if( lua_pcall( L, 2, 1, 0) != 0 )     // table
  {
    Logger::Log( "Error running function \"debug.getinfo\"");
    Logger::Log( std::string( lua_tostring(L, -1) ));
  }

  lua_pushstring( L, "source" );        // table.source
  lua_gettable(L, -2);                  // <value>
  
  auto fileName = std::string( lua_tostring( L, -1 ) );

  lua_getglobal( L, "debug" );          // debug
  lua_getfield( L, -1, "getinfo" );     // debug.getinfo 
  lua_pushinteger( L, 2 );              // debug.getinfo ( 2 )
  lua_pushstring( L, "l" );             // debug.getinfo ( 2, "S" )

  if( lua_pcall( L, 2, 1, 0) != 0 )     // table
  {
    Logger::Log( "Error running function \"debug.getinfo\"");
    Logger::Log( std::string( lua_tostring(L, -1) ));
  }

  lua_pushstring( L, "currentline" );        // table.source
  lua_gettable(L, -2);                       // <value>
  
  auto lineNumber = lua_tointeger( L, -1 );

  return fileName + ":" + std::to_string( lineNumber );
}

void ScriptResourceManager::ConfigureEnvironment(sol::state& state) {

  sol::table battle_namespace =     state.create_table("Battle");
  sol::table overworld_namespace =  state.create_table("Overworld");
  sol::table engine_namespace =     state.create_table("Engine");

  engine_namespace.set_function("get_rand_seed", [this]() -> unsigned int { return randSeed; });

  // The function calls in Lua for what is normally treated like a member variable seem a little bit wonky
  const auto& tile_record = state.new_usertype<Battle::Tile>("Tile", sol::no_constructor,
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
    "attack_entities", &Battle::Tile::AffectEntities,
    "get_distance_to_tile", &Battle::Tile::Distance,
    "find_characters", &Battle::Tile::FindCharacters,
    "highlight", &Battle::Tile::RequestHighlight,
    "get_tile", &Battle::Tile::GetTile,
    "contains_entity", &Battle::Tile::ContainsEntity,
    "remove_entity_by_id", &Battle::Tile::RemoveEntityByID,
    "reserve_entity_by_id", &Battle::Tile::ReserveEntityByID,
    "add_entity", sol::overload(
      sol::resolve<void(Artifact&)>(&Battle::Tile::AddEntity),
      sol::resolve<void(Spell&)>(&Battle::Tile::AddEntity),
      sol::resolve<void(Obstacle&)>(&Battle::Tile::AddEntity),
      sol::resolve<void(Character&)>(&Battle::Tile::AddEntity)
    )
  );

  // Exposed "GetCharacter" so that there's a way to maintain a reference to other actors without hanging onto pointers.
  // If you hold onto their ID, and use that through Field::GetCharacter,
  // instead you will get passed a nullptr/nil in Lua after they've been effectively removed,
  // rather than potentially holding onto a hanging pointer to deleted data.
  const auto& field_record = battle_namespace.new_usertype<Field>("Field", sol::no_constructor,
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Field", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Field", key );
    },
    "tile_at", &Field::GetAt,
    "width", &Field::GetWidth,
    "height", &Field::GetHeight,
    "spawn", sol::overload(
      sol::resolve<Field::AddEntityStatus(std::unique_ptr<ScriptedObstacle>&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(std::unique_ptr<ScriptedArtifact>&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(std::unique_ptr<ScriptedSpell>&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(Artifact&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(Spell&, int, int)>(&Field::AddEntity)
    ),
    "get_character", &Field::GetCharacter,
    "get_entity", &Field::GetEntity,
    "find_characters", sol::overload(
      sol::resolve<std::vector<Character*>(std::function<bool(Character*)>)>(&Field::FindCharacters)
    ),
    "find_nearest_characters", sol::overload(
      sol::resolve<std::vector<Character*>(Character*, std::function<bool(Character*)>)>(&Field::FindNearestCharacters)
    ),
    "find_tiles", &Field::FindTiles,
    "notify_on_delete", &Field::NotifyOnDelete,
    "callback_on_delete", &Field::CallbackOnDelete
  );

  const auto& animation_record = engine_namespace.new_usertype<Animation>("Animation",
    sol::constructors<Animation(const std::string&), Animation(const Animation&)>(),
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Animation", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Animation", key );
    },
    "load", &Animation::Load,
    "update", &Animation::Update,
    "refresh", &Animation::Refresh,
    "copy_from", &Animation::CopyFrom,
    "set_state", &Animation::SetAnimation,
    "get_state", &Animation::GetAnimationString,
    "point", &Animation::GetPoint,
    "set_playback", sol::resolve<Animation& (char)>(&Animation::operator<<),
    "on_complete", sol::resolve<void(const FrameCallback&)>(&Animation::operator<<),
    "on_frame", &Animation::AddCallback,
    "on_interrupt", &Animation::SetInterruptCallback
  );

  const auto& node_record = engine_namespace.new_usertype<SpriteProxyNode>("SpriteNode",
    sol::constructors<SpriteProxyNode()>(),
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "SpriteNode", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "SpriteNode", key );
    },
    "get_texture", &SpriteProxyNode::getTexture,
    "set_texture", &SpriteProxyNode::setTexture,
    "show", &SpriteProxyNode::Reveal,
    "hide", &SpriteProxyNode::Hide,
    "set_layer", &SpriteProxyNode::SetLayer,
    "get_layer", &SpriteProxyNode::GetLayer,
    "add_node", &SpriteProxyNode::AddNode,
    "remove_node", &SpriteProxyNode::RemoveNode,
    "add_tag", &SpriteProxyNode::AddTags,
    "remove_tags", &SpriteProxyNode::RemoveTags,
    "has_tag", &SpriteProxyNode::HasTag,
    "find_child_nodes_with_tags", &SpriteProxyNode::GetChildNodesWithTag,
    "get_layer", &SpriteProxyNode::GetLayer,
    "set_position", sol::resolve<void(float, float)>(&SpriteProxyNode::setPosition),
    "get_position", &SpriteProxyNode::getPosition,
    "set_color", &SpriteProxyNode::setColor,
    "get_color", &SpriteProxyNode::getColor,
    "set_origin", sol::resolve<void(float, float)>(&SpriteProxyNode::setOrigin),
    "get_origin", &SpriteProxyNode::getOrigin,
    "get_width", [](SpriteProxyNode& self) -> float { return self.getLocalBounds().width * self.getScale().x; },
    "get_height", [](SpriteProxyNode& self) -> float { return self.getLocalBounds().height * self.getScale().y; },
    "get_color_mode", &SpriteProxyNode::GetColorMode,
    "set_color_mode", &SpriteProxyNode::SetColorMode,
    "unwrap", &SpriteProxyNode::getSprite,
    "enable_parent_shader", &SpriteProxyNode::EnableParentShader,
    sol::base_classes, sol::bases<SceneNode>()
  );

  const auto& component_record = battle_namespace.new_usertype<ScriptedComponent>("Component",
    sol::factories(
      [](Character* owner, Component::lifetimes lifetime) -> std::unique_ptr<ScriptedComponent> {
        return std::make_unique<ScriptedComponent>(owner, lifetime);
      },
      [](ScriptedCharacter* owner, Component::lifetimes lifetime) -> std::unique_ptr<ScriptedComponent> {
        return std::make_unique<ScriptedComponent>(owner, lifetime);
      },
      [](Player* owner, Component::lifetimes lifetime) -> std::unique_ptr<ScriptedComponent> {
        return std::make_unique<ScriptedComponent>(owner, lifetime);
      },
      [](ScriptedPlayer* owner, Component::lifetimes lifetime) -> std::unique_ptr<ScriptedComponent> {
        return std::make_unique<ScriptedComponent>(owner, lifetime);
      },
      [](Obstacle* owner, Component::lifetimes lifetime) -> std::unique_ptr<ScriptedComponent> {
        return std::make_unique<ScriptedComponent>(owner, lifetime);
      },
      [](ScriptedObstacle* owner, Component::lifetimes lifetime) -> std::unique_ptr<ScriptedComponent> {
        return std::make_unique<ScriptedComponent>(owner, lifetime);
      }
    ),
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); },
    "eject", &ScriptedComponent::Eject,
    "get_id", &ScriptedComponent::GetID,
    "is_injected", &ScriptedComponent::Injected,
    "get_owner", &ScriptedComponent::GetOwnerAsCharacter,
    "scene_inject_func", &ScriptedComponent::scene_inject_func,
    "update_func", &ScriptedComponent::update_func,
    sol::base_classes, sol::bases<Component>()
  );

const auto& spell_record = battle_namespace.new_usertype<Spell>( "BasicSpell",
    sol::no_constructor,
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "BasicSpell", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "BasicSpell", key );
    },
    "get_id", &Spell::GetID,
    "get_tile", &Spell::GetTile,
    "get_current_tile", &Spell::GetCurrentTile,
    "get_field", &Spell::GetField,
    "get_facing", &Spell::GetFacing,
    "get_facing", &Spell::GetFacingAway,
    "set_facing", &Spell::SetFacing,
    "sprite", &Spell::AsSpriteProxyNode,
    "get_alpha", &Spell::GetAlpha,
    "set_alpha", &Spell::SetAlpha,
    "get_color", &Spell::getColor,
    "set_color", &Spell::setColor,
    "slide", &Spell::Slide,
    "jump", &Spell::Jump,
    "teleport", &Spell::Teleport,
    "hide", &Spell::Hide,
    "reveal", &Spell::Reveal,
    "raw_move_event", &Spell::RawMoveEvent,
    "is_sliding", &Spell::IsSliding,
    "is_jumping", &Spell::IsJumping,
    "is_teleporting", &Spell::IsTeleporting,
    "is_moving", &Spell::IsMoving,
    "is_deleted", &Spell::IsDeleted,
    "will_remove_eof", &Spell::WillRemoveLater,
    "get_team", &Spell::GetTeam,
    "is_team", &Spell::Teammate,
    "remove", &ScriptedSpell::Remove,
    "delete", &ScriptedSpell::Delete,
    "get_texture", &ScriptedSpell::getTexture,
    "copy_hit_props", &ScriptedSpell::GetHitboxProperties,
    "get_position", &Spell::GetDrawOffset
  );

  const auto& scripted_spell_record = battle_namespace.new_usertype<ScriptedSpell>( "Spell",
    sol::factories([](Team team) -> std::unique_ptr<ScriptedSpell> {
        return std::make_unique<ScriptedSpell>(team);
    }),
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); },
    "get_id", &ScriptedSpell::GetID,
    "get_tile", &ScriptedSpell::GetTile,
    "get_current_tile", &ScriptedSpell::GetCurrentTile,
    "get_field", &ScriptedSpell::GetField,
    "get_facing", &ScriptedSpell::GetFacing,
    "set_facing", &ScriptedSpell::SetFacing,
    "get_facing_away", &ScriptedSpell::GetFacingAway,
    "sprite", &ScriptedSpell::AsSpriteProxyNode,
    "get_alpha", &ScriptedSpell::GetAlpha,
    "set_alpha", &ScriptedSpell::SetAlpha,
    "get_color", &ScriptedSpell::getColor,
    "set_color", &ScriptedSpell::setColor,
    "slide", &ScriptedSpell::Slide,
    "jump", &ScriptedSpell::Jump,
    "teleport", &ScriptedSpell::Teleport,
    "hide", &ScriptedSpell::Hide,
    "reveal", &ScriptedSpell::Reveal,
    "raw_move_event", &ScriptedSpell::RawMoveEvent,
    "is_sliding", &ScriptedSpell::IsSliding,
    "is_jumping", &ScriptedSpell::IsJumping,
    "is_teleporting", &ScriptedSpell::IsTeleporting,
    "is_moving", &ScriptedSpell::IsMoving,
    "is_deleted", &ScriptedSpell::IsDeleted,
    "will_remove_eof", &ScriptedSpell::WillRemoveLater,
    "get_team", &ScriptedSpell::GetTeam,
    "is_team", &ScriptedSpell::Teammate,
    "remove", &ScriptedSpell::Remove,
    "delete", &ScriptedSpell::Delete,
    "get_texture", &ScriptedSpell::getTexture,
    "set_texture", &ScriptedSpell::setTexture,
    "add_node", &ScriptedSpell::AddNode,
    "highlight_tile", &ScriptedSpell::HighlightTile,
    "copy_hit_props", &ScriptedSpell::GetHitboxProperties,
    "set_hit_props", &ScriptedSpell::SetHitboxProperties,
    "get_animation", &ScriptedSpell::GetAnimationObject,
    "set_animation", &ScriptedSpell::SetAnimation,
    "shake_camera", &ScriptedSpell::ShakeCamera,
    "set_height", &ScriptedSpell::SetHeight,
    "set_position", sol::overload(
      sol::resolve<void(float, float)>(&ScriptedSpell::SetDrawOffset)
    ),
    "get_position", &ScriptedSpell::GetDrawOffset,
    "show_shadow", &ScriptedSpell::ShowShadow,
    "never_flip", &ScriptedSpell::NeverFlip,
    "attack_func", &ScriptedSpell::attackCallback,
    "delete_func", &ScriptedSpell::deleteCallback,
    "update_func", &ScriptedSpell::updateCallback,
    "collision_func", &ScriptedSpell::collisionCallback,
    "can_move_to_func", &ScriptedSpell::canMoveToCallback,
    "on_spawn_func", &ScriptedSpell::spawnCallback,
    sol::base_classes, sol::bases<Spell>()
  );

  const auto& scripted_obstacle_record = battle_namespace.new_usertype<ScriptedObstacle>("Obstacle",
    sol::factories([](Team team) -> std::unique_ptr<ScriptedObstacle> {
        return std::make_unique<ScriptedObstacle>(team);
    }),
    sol::base_classes, sol::bases<Obstacle, Spell, Character>(),
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); },
    "get_id", &ScriptedObstacle::GetID,
    "get_element", &ScriptedObstacle::GetElement,
    "set_element", &ScriptedObstacle::SetElement,
    "get_facing_away", &ScriptedObstacle::GetFacingAway,
    "get_facing", &ScriptedObstacle::GetFacing,
    "set_facing", &ScriptedObstacle::SetFacing,
    "get_tile", &ScriptedObstacle::GetTile,
    "get_current_tile", &ScriptedObstacle::GetCurrentTile,
    "get_alpha", &ScriptedObstacle::GetAlpha,
    "set_alpha", &ScriptedObstacle::SetAlpha,
    "get_color", &ScriptedObstacle::getColor,
    "set_color", &ScriptedObstacle::setColor,
    "get_field", &ScriptedObstacle::GetField,
    "sprite", &ScriptedObstacle::AsSpriteProxyNode,
    "hide", &ScriptedObstacle::Hide,
    "reveal", &ScriptedObstacle::Reveal,
    "slide", &ScriptedObstacle::Slide,
    "jump", &ScriptedObstacle::Jump,
    "teleport", &ScriptedObstacle::Teleport,
    "raw_move_event", &ScriptedObstacle::RawMoveEvent,
    "is_sliding", &ScriptedObstacle::IsSliding,
    "is_jumping", &ScriptedObstacle::IsJumping,
    "is_teleporting", &ScriptedObstacle::IsTeleporting,
    "is_moving", &ScriptedObstacle::IsMoving,
    "is_team", &ScriptedObstacle::Teammate,
    "is_deleted", &ScriptedObstacle::IsDeleted,
    "is_passthrough", &ScriptedObstacle::IsPassthrough,
    "will_remove_eof", &ScriptedObstacle::WillRemoveLater,
    "get_team", &ScriptedObstacle::GetTeam,
    "remove", &ScriptedObstacle::Remove,
    "delete", &ScriptedObstacle::Delete,
    "get_name", &ScriptedObstacle::GetName,
    "get_health", &ScriptedObstacle::GetHealth,
    "get_max_health", &ScriptedObstacle::GetMaxHealth,
    "set_name", &ScriptedObstacle::SetName,
    "set_health", &ScriptedObstacle::SetHealth,
    "share_tile", &ScriptedObstacle::ShareTileSpace,
    "add_defense_rule", &ScriptedObstacle::AddDefenseRule,
    "remove_defense_rule", &ScriptedObstacle::RemoveDefenseRule,
    "get_texture", &ScriptedObstacle::getTexture,
    "set_texture", &ScriptedObstacle::setTexture,
    "get_animation", &ScriptedObstacle::GetAnimationObject,
    "set_animation", &ScriptedObstacle::SetAnimation,
    "add_node", &ScriptedObstacle::AddNode,
    "highlight_tile", &ScriptedObstacle::HighlightTile,
    "get_hit_props", &ScriptedObstacle::GetHitboxProperties,
    "set_hit_props", &ScriptedObstacle::SetHitboxProperties,
    "ignore_common_aggressor", &ScriptedObstacle::IgnoreCommonAggressor,
    "set_height", &ScriptedObstacle::SetHeight,
    "show_shadow", &ScriptedObstacle::ShowShadow,
    "shake_camera", &ScriptedObstacle::ShakeCamera,
    "register_component", &ScriptedObstacle::RegisterComponent,
    "set_position", sol::overload(
      sol::resolve<void(float, float)>(&ScriptedObstacle::SetDrawOffset)
    ),
    "get_position", &ScriptedObstacle::GetDrawOffset,
    "never_flip", &ScriptedObstacle::NeverFlip,
    "attack_func", &ScriptedObstacle::attackCallback,
    "delete_func", &ScriptedObstacle::deleteCallback,
    "update_func", &ScriptedObstacle::updateCallback,
    "collision_func", &ScriptedObstacle::collisionCallback,
    "can_move_to_func", &ScriptedObstacle::canMoveToCallback,
    "on_spawn_func", &ScriptedObstacle::spawnCallback
  );

  // Adding in bindings for Character* type objects to sol.
  // Without adding these in, it has no idea what to do with Character* objects passed up to it,
  // even though there's bindings for ScriptedCharacter already done.
  const auto& basic_character_record = battle_namespace.new_usertype<Character>( "BasicCharacter",
    "get_id", &Character::GetID,
    "get_element", &Character::GetElement,
    "set_element", &Character::SetElement,
    "get_tile", &Character::GetTile,
    "get_current_tile", &Character::GetCurrentTile,
    "get_field", &Character::GetField,
    "get_facing", &ScriptedCharacter::GetFacing,
    "get_facing_away", &ScriptedCharacter::GetFacingAway,
    "get_alpha", &Character::GetAlpha,
    "set_alpha", &Character::SetAlpha,
    "get_color", &Character::getColor,
    "set_color", &Character::setColor,
    "sprite", &Character::AsSpriteProxyNode,
    "slide", &Character::Slide,
    "jump", &Character::Jump,
    "teleport", &Character::Teleport,
    "raw_move_event", &Character::RawMoveEvent,
    "is_sliding", &Character::IsSliding,
    "is_jumping", &Character::IsJumping,
    "is_teleporting", &Character::IsTeleporting,
    "is_passthrough", &Character::IsPassthrough,
    "is_moving", &Character::IsMoving,
    "is_deleted", &Character::IsDeleted,
    "will_remove_eof", &Character::WillRemoveLater,
    "get_team", &Character::GetTeam,
    "is_team", &Character::Teammate,
    "hide", &Character::Hide,
    "reveal", &Character::Reveal,
    "get_texture", &Character::getTexture,
    "set_texture", &Character::setTexture,
    "add_node", &Character::AddNode,
    "get_name", &Character::GetName,
    "get_health", &Character::GetHealth,
    "get_max_health", &Character::GetMaxHealth,
    "set_name", &Character::SetName,
    "set_health", &Character::SetHealth,
    "get_rank", &Character::GetRank,
    "share_tile", &Character::ShareTileSpace,
    "add_defense_rule", &Character::AddDefenseRule,
    "register_component", &Character::RegisterComponent,
    "remove_defense_rule", &Character::RemoveDefenseRule,
    "set_position", sol::overload(
      sol::resolve<void(float, float)>(&Character::SetDrawOffset)
    ),
    "get_position", &Character::GetDrawOffset,
    "set_height", &Character::SetHeight,
    "toggle_counter", &Character::ToggleCounter,
    "get_animation", &Character::GetAnimationFromComponent // I don't want to do this, but sol2 makes me...
  );

  const auto& scripted_character_record = battle_namespace.new_usertype<ScriptedCharacter>("Character",
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); },
    sol::base_classes, sol::bases<Character>(),
    "get_id", &ScriptedCharacter::GetID,
    "get_element", &ScriptedCharacter::GetElement,
    "set_element", &ScriptedCharacter::SetElement,
    "get_tile", &ScriptedCharacter::GetTile,
    "get_current_tile", &ScriptedCharacter::GetCurrentTile,
    "get_field", &ScriptedCharacter::GetField,
    "get_facing", &ScriptedCharacter::GetFacing,
    "get_facing_away", &ScriptedCharacter::GetFacingAway,
    "get_target", &ScriptedCharacter::GetTarget,
    "get_alpha", &ScriptedCharacter::GetAlpha,
    "set_alpha", &ScriptedCharacter::SetAlpha,
    "get_color", &ScriptedCharacter::getColor,
    "set_color", &ScriptedCharacter::setColor,
    "sprite", &ScriptedCharacter::AsSpriteProxyNode,
    "slide", &ScriptedCharacter::Slide,
    "jump", &ScriptedCharacter::Jump,
    "teleport", &ScriptedCharacter::Teleport,
    "raw_move_event", &ScriptedCharacter::RawMoveEvent,
    "card_action_event", sol::overload(
      sol::resolve<void(std::unique_ptr<ScriptedCardAction>&, ActionOrder)>(&ScriptedCharacter::SimpleCardActionEvent),
      sol::resolve<void(std::unique_ptr<CardAction>&, ActionOrder)>(&ScriptedCharacter::SimpleCardActionEvent)
    ),
    "is_sliding", &ScriptedCharacter::IsSliding,
    "is_jumping", &ScriptedCharacter::IsJumping,
    "is_teleporting", &ScriptedCharacter::IsTeleporting,
    "is_moving", &ScriptedCharacter::IsMoving,
    "is_passthrough", &ScriptedCharacter::IsPassthrough,
    "is_deleted", &ScriptedCharacter::IsDeleted,
    "will_remove_eof", &ScriptedCharacter::WillRemoveLater,
    "get_team", &ScriptedCharacter::GetTeam,
    "is_team", &ScriptedCharacter::Teammate,
    "hide", &ScriptedCharacter::Hide,
    "reveal", &ScriptedCharacter::Reveal,
    "remove", &ScriptedCharacter::Remove,
    "delete", &ScriptedCharacter::Delete,
    "get_texture", &ScriptedCharacter::getTexture,
    "set_texture", &ScriptedCharacter::setTexture,
    "add_node", &ScriptedCharacter::AddNode,
    "get_name", &ScriptedCharacter::GetName,
    "get_health", &ScriptedCharacter::GetHealth,
    "get_max_health", &ScriptedCharacter::GetMaxHealth,
    "set_name", &ScriptedCharacter::SetName,
    "set_health", &ScriptedCharacter::SetHealth,
    "get_rank", &ScriptedCharacter::GetRank,
    "share_tile", &ScriptedCharacter::ShareTileSpace,
    "add_defense_rule", &ScriptedCharacter::AddDefenseRule,
    "register_component", &ScriptedCharacter::RegisterComponent,
    "remove_defense_rule", &ScriptedCharacter::RemoveDefenseRule,
    "set_position", sol::overload(
      sol::resolve<void(float, float)>(&ScriptedCharacter::SetDrawOffset)
    ),
    "get_position", &ScriptedCharacter::GetDrawOffset,
    "set_height", &ScriptedCharacter::SetHeight,
    "get_animation", &ScriptedCharacter::GetAnimationObject,
    "shake_camera", &ScriptedCharacter::ShakeCamera,
    "toggle_counter", &ScriptedCharacter::ToggleCounter,
    "register_status_callback", &ScriptedCharacter::RegisterStatusCallback,
    "delete_func", &ScriptedCharacter::deleteCallback,
    "update_func", &ScriptedCharacter::updateCallback,
    "can_move_to_func", &ScriptedCharacter::canMoveToCallback,
    "on_spawn_func", &ScriptedCharacter::spawnCallback,
    "battle_start_func", &ScriptedCharacter::onBattleStartCallback,
    "battle_end_func", &ScriptedCharacter::onBattleEndCallback,
    "set_explosion_behavior", &ScriptedCharacter::SetExplosionBehavior
  );

  const auto& player_record = battle_namespace.new_usertype<Player>("BasicPlayer",
    sol::base_classes, sol::bases<Character>(),
    "get_id", &Player::GetID,
    "get_tile", &Player::GetTile,
    "get_current_tile", &Player::GetCurrentTile,
    "get_field", &Player::GetField,
    "get_facing", &Player::GetFacing,
    "get_facing_away", &Player::GetFacingAway,
    "is_sliding", &Player::IsSliding,
    "is_jumping", &Player::IsJumping,
    "is_teleporting", &Player::IsTeleporting,
    "is_moving", &Player::IsMoving,
    "is_deleted", &Player::IsDeleted,
    "is_passthrough", &Player::IsPassthrough,
    "get_alpha", &Player::GetAlpha,
    "set_alpha", &Player::SetAlpha,
    "get_color", &Player::getColor,
    "set_color", &Player::setColor,
    "register_component", &Player::RegisterComponent,
    "remove", &Player::Remove,
    "delete", &Player::Delete,
    "hide", &Player::Hide,
    "reveal", &Player::Reveal,
    "will_remove_eof", &Player::WillRemoveLater,
    "get_team", &Player::GetTeam,
    "get_name", &Player::GetName,
    "get_health", &Player::GetHealth,
    "get_max_health", &Player::GetMaxHealth
  );

  const auto& scripted_player_record = battle_namespace.new_usertype<ScriptedPlayer>("Player",
    sol::base_classes, sol::bases<Player>(),
    "get_id", &ScriptedPlayer::GetID,
    "get_element", &ScriptedPlayer::GetElement,
    "set_element", &ScriptedPlayer::SetElement,
    "get_tile", &ScriptedPlayer::GetTile,
    "get_current_tile", &ScriptedPlayer::GetCurrentTile,
    "get_field", &ScriptedPlayer::GetField,
    "get_facing", &ScriptedPlayer::GetFacing,
    "get_facing_away", &ScriptedPlayer::GetFacingAway,
    "get_alpha", &ScriptedPlayer::GetAlpha,
    "set_alpha", &ScriptedPlayer::SetAlpha,
    "get_color", &ScriptedPlayer::getColor,
    "set_color", &ScriptedPlayer::setColor,
    "register_component", &ScriptedPlayer::RegisterComponent,
    "sprite", &ScriptedPlayer::AsSpriteProxyNode,
    "slide", &ScriptedPlayer::Slide,
    "jump", &ScriptedPlayer::Jump,
    "teleport", &ScriptedPlayer::Teleport,
    "hide", &ScriptedPlayer::Hide,
    "reveal", &ScriptedPlayer::Reveal,
    "raw_move_event", &ScriptedPlayer::RawMoveEvent,
    "is_sliding", &ScriptedPlayer::IsSliding,
    "is_jumping", &ScriptedPlayer::IsJumping,
    "is_teleporting", &ScriptedPlayer::IsTeleporting,
    "is_moving", &ScriptedPlayer::IsMoving,
    "is_deleted", &ScriptedPlayer::IsDeleted,
    "is_passthrough", &ScriptedPlayer::IsPassthrough,
    "will_remove_eof", &ScriptedPlayer::WillRemoveLater,
    "get_team", &ScriptedPlayer::GetTeam,
    "get_name", &ScriptedPlayer::GetName,
    "get_health", &ScriptedPlayer::GetHealth,
    "get_max_health", &ScriptedPlayer::GetMaxHealth,
    "get_attack_level", &ScriptedPlayer::GetAttackLevel,
    "get_charge_level", &ScriptedPlayer::GetChargeLevel,
    "set_name", &ScriptedPlayer::SetName,
    "set_health", &ScriptedPlayer::SetHealth,
    "get_texture", &ScriptedPlayer::getTexture,
    "set_texture", &ScriptedPlayer::setTexture,
    "set_animation", &ScriptedPlayer::SetAnimation,
    "set_height", &ScriptedPlayer::SetHeight,
    "set_fully_charge_color", &ScriptedPlayer::SetFullyChargeColor,
    "set_charge_position", &ScriptedPlayer::SetChargePosition,
    "get_animation", &ScriptedPlayer::GetAnimationObject,
    "set_float_shoe", &ScriptedPlayer::SetFloatShoe,
    "set_air_shoe", &ScriptedPlayer::SetAirShoe,
    "slide_when_moving", &ScriptedPlayer::SlideWhenMoving,
    "add_defense_rule", &ScriptedPlayer::AddDefenseRule,
    "delete", &ScriptedPlayer::Delete,
    "register_component", &ScriptedPlayer::RegisterComponent,
    "set_palette", &ScriptedPlayer::SetPalette,
    "store_base_palette", &ScriptedPlayer::StoreBasePalette,
    "get_base_palette", &ScriptedPlayer::GetBasePalette,
    "get_current_palette", &ScriptedPlayer::GetPalette,
    "create_form", &ScriptedPlayer::CreateForm,
    "add_form", &ScriptedPlayer::AddForm,
    "create_anim_sync_item", &ScriptedPlayer::CreateAnimSyncItem,
    "remove_anim_sync_item", &ScriptedPlayer::RemoveAnimSyncItem,
    "update_func", &ScriptedPlayer::on_update_func,
    "normal_attack_func", &ScriptedPlayer::on_buster_action,
    "charged_attack_func", &ScriptedPlayer::on_charged_action,
    "special_attack_func", &ScriptedPlayer::on_special_action
  );

  const auto& scripted_player_form_meta_record = battle_namespace.new_usertype<ScriptedPlayerFormMeta>("PlayerFormMeta",
    "set_mugshot_texture_path", &ScriptedPlayerFormMeta::SetUIPath,
    "update_func", &ScriptedPlayerFormMeta::on_update,
    "charged_attack_func", &ScriptedPlayerFormMeta::on_charge_action,
    "special_attack_func", &ScriptedPlayerFormMeta::on_special_action,
    "on_activate_func", &ScriptedPlayerFormMeta::on_activate,
    "on_deactivate_func", &ScriptedPlayerFormMeta::on_deactivate,
    "calculate_charge_time_func", &ScriptedPlayerFormMeta::on_calculate_charge_time
  );

  const auto& scripted_player_form_record = battle_namespace.new_usertype<ScriptedPlayerForm>("PlayerForm",
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); }
  );

  const auto& scripted_artifact_record = battle_namespace.new_usertype<ScriptedArtifact>("Artifact",
    sol::factories([](Team team) -> std::unique_ptr<ScriptedArtifact> {
      auto ptr = std::make_unique<ScriptedArtifact>();
      ptr->SetTeam(team);
      return ptr;
    }),
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); },
    "get_id", &ScriptedArtifact::GetID,
    "get_tile", &ScriptedArtifact::GetTile,
    "get_current_tile", &ScriptedArtifact::GetCurrentTile,
    "get_field", &ScriptedArtifact::GetField,
    "get_facing", &ScriptedArtifact::GetFacing,
    "get_facing_away", &ScriptedArtifact::GetFacingAway,
    "get_alpha", &ScriptedArtifact::GetAlpha,
    "set_alpha", &ScriptedArtifact::SetAlpha,
    "get_color", &ScriptedArtifact::getColor,
    "set_color", &ScriptedArtifact::setColor,
    "sprite", &ScriptedArtifact::AsSpriteProxyNode,
    "slide", &ScriptedArtifact::Slide,
    "jump", &ScriptedArtifact::Jump,
    "teleport", &ScriptedArtifact::Teleport,
    "hide", &ScriptedArtifact::Hide,
    "reveal", &ScriptedArtifact::Reveal,
    "remove", &ScriptedArtifact::Remove,
    "delete", &ScriptedArtifact::Delete,
    "raw_move_event", &ScriptedArtifact::RawMoveEvent,
    "is_sliding", &ScriptedArtifact::IsSliding,
    "is_jumping", &ScriptedArtifact::IsJumping,
    "is_teleporting", &ScriptedArtifact::IsTeleporting,
    "is_moving", &ScriptedArtifact::IsMoving,
    "is_deleted", &ScriptedArtifact::IsDeleted,
    "will_remove_eof", &ScriptedArtifact::WillRemoveLater,
    "get_team", &ScriptedArtifact::GetTeam,
    "set_animation", &ScriptedArtifact::SetAnimation,
    "get_texture", &ScriptedArtifact::getTexture,
    "set_texture", &ScriptedArtifact::setTexture,
    "set_height", &ScriptedArtifact::SetHeight,
    "set_position", sol::overload(
      sol::resolve<void(float, float)>(&ScriptedArtifact::SetDrawOffset)
    ),
    "get_animation", &ScriptedArtifact::GetAnimationObject,
    "never_flip", &ScriptedArtifact::NeverFlip,
    "delete_func", &ScriptedArtifact::deleteCallback,
    "update_func", &ScriptedArtifact::updateCallback,
    "can_move_to_func", &ScriptedArtifact::canMoveToCallback,
    "on_spawn_func", &ScriptedArtifact::spawnCallback
  );

  const auto& card_action_record = battle_namespace.new_usertype<CardAction>("BaseCardAction",
    "set_lockout", &CardAction::SetLockout,
    "set_lockout_group", &CardAction::SetLockoutGroup,
    "override_animation_frames", &CardAction::OverrideAnimationFrames,
    "add_attachment", sol::overload(
      sol::resolve<CardAction::Attachment& (Character*, const std::string&, SpriteProxyNode&)>(&CardAction::AddAttachment),
      sol::resolve<CardAction::Attachment& (Animation&, const std::string&, SpriteProxyNode&)>(&CardAction::CardAction::AddAttachment)
    ),
    "add_anim_action", &CardAction::AddAnimAction,
    "add_step", &CardAction::AddStep,
    "end_action", &CardAction::EndAction,
    "get_actor", sol::overload(
      sol::resolve<Character* ()>(&CardAction::GetActor)
    ),
    "set_metadata", &CardAction::SetMetaData,
    "copy_metadata", &CardAction::GetMetaData
  );

  // Game would crash when using ScriptedPlayer* values so had to expose other versions for it to cooperate.
  // Many things use Character* references but we will maybe have to consolidate all the interfaces for characters into one type.
  const auto& scripted_card_action_record = battle_namespace.new_usertype<ScriptedCardAction>("CardAction",
    sol::factories(
      [](Character* character, const std::string& state)-> std::unique_ptr<ScriptedCardAction> {
        return std::make_unique<ScriptedCardAction>(character, state);
      },
      [](ScriptedPlayer* character, const std::string& state) -> std::unique_ptr<ScriptedCardAction> {
        return std::make_unique<ScriptedCardAction>(character, state);
      }, 
      [](ScriptedCharacter* character, const std::string& state) -> std::unique_ptr<ScriptedCardAction> {
        return std::make_unique<ScriptedCardAction>(character, state);
      }
    ),
    sol::meta_function::index, &dynamic_object::dynamic_get,
    sol::meta_function::new_index, &dynamic_object::dynamic_set,
    sol::meta_function::length, [](dynamic_object& d) { return d.entries.size(); },
    "action_end_func", &ScriptedCardAction::onActionEnd,
    "animation_end_func", &ScriptedCardAction::onAnimationEnd,
    "execute_func", &ScriptedCardAction::onExecute,
    "update_func", &ScriptedCardAction::onUpdate,
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& step_record = battle_namespace.new_usertype<CardAction::Step>("Step",
    sol::factories([]() -> std::unique_ptr<CardAction::Step> {
      return std::make_unique<CardAction::Step>();
    }),
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Step", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Step", key );
    },
    "update_func", &CardAction::Step::updateFunc,
    "draw_func", &CardAction::Step::drawFunc,
    "complete_step", &CardAction::Step::markDone
  );

  state.set_function("make_animation_lockout",
    []() { return CardAction::LockoutProperties{ CardAction::LockoutType::animation }; }
  );

  state.set_function("make_async_lockout",
    [](double cooldown){ return CardAction::LockoutProperties{ CardAction::LockoutType::async, cooldown }; }
  );

  state.set_function("make_sequence_lockout",
    [](){ return CardAction::LockoutProperties{ CardAction::LockoutType::sequence }; }
  );

  const auto& lockout_type_record = state.new_enum("LockType",
    "Animation", CardAction::LockoutType::animation,
    "Async", CardAction::LockoutType::async,
    "Sequence", CardAction::LockoutType::sequence
  );

  const auto& lockout_group_record = state.new_enum("Lockout",
    "Weapons", CardAction::LockoutGroup::weapon,
    "Cards", CardAction::LockoutGroup::card,
    "Abilities", CardAction::LockoutGroup::ability
  );

  const auto& defense_frame_state_judge_record = state.new_usertype<DefenseFrameStateJudge>("DefenseFrameStateJudge",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "DefenseFrameStateJudge", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "DefenseFrameStateJudge", key );
    },
    "block_damage", &DefenseFrameStateJudge::BlockDamage,
    "block_impact", &DefenseFrameStateJudge::BlockImpact,
    "is_damage_blocked", &DefenseFrameStateJudge::IsDamageBlocked,
    "is_impact_blocked", &DefenseFrameStateJudge::IsImpactBlocked,
    /*"add_trigger", &DefenseFrameStateJudge::AddTrigger,*/
    "signal_defense_was_pierced", &DefenseFrameStateJudge::SignalDefenseWasPierced
  );

  const auto& defense_rule_record = battle_namespace.new_usertype<ScriptedDefenseRule>("DefenseRule",
    sol::factories(
        [](int priority, const DefenseOrder& order) -> std::unique_ptr<ScriptedDefenseRule>
        { return std::make_unique<ScriptedDefenseRule>(Priority(priority), order); }
    ),
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "DefenseRule", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "DefenseRule", key );
    },
    "is_replaced", &ScriptedDefenseRule::IsReplaced,
    "can_block_func", &ScriptedDefenseRule::canBlockCallback,
    "filter_statuses_func", &ScriptedDefenseRule::filterStatusesCallback,
    sol::base_classes, sol::bases<DefenseRule>()
  );

  const auto& defense_rule_nodrag = battle_namespace.new_usertype<DefenseNodrag>("DefenseNoDrag",
    sol::factories(
        [] { return std::make_unique<DefenseNodrag>(); }
    ),
    sol::base_classes, sol::bases<DefenseRule>()
  );

  const auto& defense_rule_virus_body = battle_namespace.new_usertype<DefenseVirusBody>("DefenseVirusBody",
    sol::factories(
        [] { return std::make_unique<DefenseVirusBody>(); }
    ),
    sol::base_classes, sol::bases<DefenseRule>()
  );

  const auto& attachment_record = battle_namespace.new_usertype<CardAction::Attachment>("Attachment",
    sol::constructors<CardAction::Attachment(Animation&, const std::string&, SpriteProxyNode&)>(),
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Attachment", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Attachment", key );
    },
    "use_animation", &CardAction::Attachment::UseAnimation,
    "add_attachment", &CardAction::Attachment::AddAttachment
  );

  const auto& hitbox_record = battle_namespace.new_usertype<Hitbox>("Hitbox",
    sol::factories([](Team team) { return new Hitbox(team); } ),
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Hitbox", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Hitbox", key );
    },
    "set_callbacks", &Hitbox::AddCallback,
    "set_hit_props", &Hitbox::SetHitboxProperties,
    "get_hit_props", &Hitbox::GetHitboxProperties,
    sol::base_classes, sol::bases<Spell>()
  );

  const auto& shared_hitbox_record = battle_namespace.new_usertype<SharedHitbox>("SharedHitbox",
    sol::constructors<SharedHitbox(Spell*, float)>(),
    sol::base_classes, sol::bases<Spell>()
  );

  const auto& busteraction_record = battle_namespace.new_usertype<BusterCardAction>("Buster",
    sol::factories(
      [](Character* character, bool charged, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<BusterCardAction>(character, charged, dmg); },
      [](ScriptedPlayer* character, bool charged, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<BusterCardAction>(character, charged, dmg); },
      [](ScriptedCharacter* character, bool charged, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<BusterCardAction>(character, charged, dmg); }
    ),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& swordaction_record = battle_namespace.new_usertype<SwordCardAction>("Sword",
    sol::factories(
      [](Character* character, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<SwordCardAction>(character, dmg); },
      [](ScriptedPlayer* character, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<SwordCardAction>(character, dmg); },
      [](ScriptedCharacter* character, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<SwordCardAction>(character, dmg); }
    ),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& bombaction_record = battle_namespace.new_usertype<BombCardAction>("Bomb",
    sol::factories(
      [](Character* character, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<BombCardAction>(character, dmg); },
      [](ScriptedPlayer* character, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<BombCardAction>(character, dmg); },
      [](ScriptedCharacter* character, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<BombCardAction>(character, dmg); }
    ),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& fireburn_record = battle_namespace.new_usertype<FireBurnCardAction>("FireBurn",
    sol::factories(
      [](Character* character, FireBurn::Type type, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<FireBurnCardAction>(character, type, dmg); },
      [](ScriptedPlayer* character, FireBurn::Type type, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<FireBurnCardAction>(character, type, dmg); },
      [](ScriptedCharacter* character, FireBurn::Type type, int dmg) -> std::unique_ptr<CardAction>
          { return std::make_unique<FireBurnCardAction>(character, type, dmg); }
    ),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& flame_cannon_type_record = battle_namespace.new_enum("FlameCannon",
    "_1", FireBurn::Type::_1,
    "_2", FireBurn::Type::_2,
    "_3", FireBurn::Type::_3
  );

  const auto& cannon_record = battle_namespace.new_usertype<CannonCardAction>("Cannon",
    sol::factories(
      [](Character* character, CannonCardAction::Type type, int dmg) -> std::unique_ptr<CardAction> {
          return std::make_unique<CannonCardAction>(character, type, dmg); }, 
      [](ScriptedPlayer* character, CannonCardAction::Type type, int dmg) -> std::unique_ptr<CardAction> {
          return std::make_unique<CannonCardAction>(character, type, dmg); }, 
      [](ScriptedCharacter* character, CannonCardAction::Type type, int dmg) -> std::unique_ptr<CardAction> {
          return std::make_unique<CannonCardAction>(character, type, dmg); }
    ),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& cannon_type_record = battle_namespace.new_enum("CannonType",
    "Green", CannonCardAction::Type::green,
    "Blue", CannonCardAction::Type::blue,
    "Red", CannonCardAction::Type::red
  );

  // make meta object info metatable
  const auto& playermeta_table = battle_namespace.new_usertype<PlayerMeta>("PlayerMeta",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "PlayerMeta", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "PlayerMeta", key );
    },
    "set_special_description", &PlayerMeta::SetSpecialDescription,
    "set_attack", &PlayerMeta::SetAttack,
    "set_charged_attack", &PlayerMeta::SetChargedAttack,
    "set_speed", &PlayerMeta::SetSpeed,
    "set_health", &PlayerMeta::SetHP,
    "set_uses_sword", &PlayerMeta::SetIsSword,
    "set_overworld_animation_path", &PlayerMeta::SetOverworldAnimationPath,
    "set_overworld_texture_path", &PlayerMeta::SetOverworldTexturePath,
    "set_mugshot_animation_path", &PlayerMeta::SetMugshotAnimationPath,
    "set_mugshot_texture_path", &PlayerMeta::SetMugshotTexturePath,
    "set_emotions_texture_path", &PlayerMeta::SetEmotionsTexturePath,
    "set_preview_texture", &PlayerMeta::SetPreviewTexture,
    "set_icon_texture", &PlayerMeta::SetIconTexture,
    "declare_package_id", &PlayerMeta::SetPackageID
  );

  const auto& cardpropsmeta_table = battle_namespace.new_usertype<Battle::Card::Properties>("Card::Properties",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Card::Properties", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Card::Properties", key );
    },
    "action", &Battle::Card::Properties::action,
    "can_boost", &Battle::Card::Properties::canBoost,
    "card_class", &Battle::Card::Properties::cardClass,
    "damage", &Battle::Card::Properties::damage,
    "description", &Battle::Card::Properties::description,
    "element", &Battle::Card::Properties::element,
    "limit", &Battle::Card::Properties::limit,
    "meta_classes", &Battle::Card::Properties::metaClasses,
    "secondary_element", &Battle::Card::Properties::secondaryElement,
    "shortname", &Battle::Card::Properties::shortname,
    "time_freeze", &Battle::Card::Properties::timeFreeze,
    "skip_time_freeze_intro", &Battle::Card::Properties::skipTimeFreezeIntro,
    "long_description", &Battle::Card::Properties::verboseDescription
  );

  const auto& cardmeta_table = battle_namespace.new_usertype<CardMeta>("CardMeta",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "CardMeta", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "CardMeta", key );
    },
    "get_card_props", &CardMeta::GetCardProperties,
    "set_preview_texture", &CardMeta::SetPreviewTexture,
    "set_icon_texture", &CardMeta::SetIconTexture,
    "set_codes", &CardMeta::SetCodes,
    "declare_package_id", &CardMeta::SetPackageID
  );

  const auto& mobmeta_table = battle_namespace.new_usertype<MobMeta>("MobMeta",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "MobMeta", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "MobMeta", key );
    },
    "set_description", &MobMeta::SetDescription,
    "set_name", &MobMeta::SetName,
    "set_preview_texture_path", &MobMeta::SetPlaceholderTexturePath,
    "set_speed", &MobMeta::SetSpeed,
    "set_attack", &MobMeta::SetAttack,
    "set_health", &MobMeta::SetHP,
    "declare_package_id", &MobMeta::SetPackageID
  );
  
  const auto& lualibrarymeta_table = battle_namespace.new_usertype<LuaLibraryMeta>("LuaLibraryMeta",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "LuaLibraryMeta", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "LuaLibraryMeta", key );
    },
    "declare_package_id", &LuaLibraryMeta::SetPackageID
  );

  const auto& blockmeta_table = battle_namespace.new_usertype<BlockMeta>("BlockMeta",
    "set_description", &BlockMeta::SetDescription,
    "set_name", &BlockMeta::SetName,
    "set_color", &BlockMeta::SetColor,
    "set_shape", &BlockMeta::SetShape,
    "as_program", &BlockMeta::AsProgram,
    "declare_package_id", &BlockMeta::SetPackageID
  );

  const auto& scriptedmob_table = battle_namespace.new_usertype<ScriptedMob>("Mob",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Mob", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Mob", key );
    },
    "create_spawner", &ScriptedMob::CreateSpawner,
    "set_background", &ScriptedMob::SetBackground,
    "stream_music", &ScriptedMob::StreamMusic,
    "get_field", &ScriptedMob::GetField,
    "enable_freedom_mission", &ScriptedMob::EnableFreedomMission
  );

  const auto& scriptedspawner_table = battle_namespace.new_usertype<ScriptedMob::ScriptedSpawner>("Spawner",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Spawner", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Spawner", key );
    },
    "spawn_at", &ScriptedMob::ScriptedSpawner::SpawnAt
  );  

  engine_namespace.set_function("load_texture",
    [](const std::string& path) {
      static ResourceHandle handle;
      return handle.Textures().LoadTextureFromFile(path);
    }
  );

  engine_namespace.set_function("load_shader",
    [](const std::string& path) {
      static ResourceHandle handle;
      return handle.Shaders().LoadShaderFromFile(path);
    }
  );

  engine_namespace.set_function("load_audio",
    [](const std::string& path) {
      static ResourceHandle handle;
      return handle.Audio().LoadFromFile(path);
    }
  );

  engine_namespace.set_function("play_audio",
    sol::factories(
      [](std::shared_ptr<sf::SoundBuffer> buffer, AudioPriority priority) {
      static ResourceHandle handle;
      return handle.Audio().Play(buffer, priority);
    },
    [](AudioType type, AudioPriority priority) {
      static ResourceHandle handle;
      return handle.Audio().Play(type, priority);
    })
  );

  engine_namespace.set_function("input_has",
    [](const InputEvent& event) {
      static InputHandle handle;
      return handle.Input().Has(event);
    }
  );

  engine_namespace.set_function("define_character",
    [this](const std::string& fqn, const std::string& path) {
        this->DefineCharacter(fqn, path);
    }
  );

  engine_namespace.set_function("requires_character",
    [this](const std::string& fqn)
    {
      // Handle built-ins...
      auto builtins = { "com.builtins.char.canodumb", "com.builtins.char.mettaur" };
      for (auto&& match : builtins) {
          if (fqn == match) return;
      }

      if (this->FetchCharacter(fqn) == nullptr) {
          std::string msg = "Failed to Require character with FQN " + fqn;
          Logger::Log(msg);
          throw std::runtime_error(msg);
      }
    }
  );

  const auto& tile_state_table = state.new_enum("TileState",
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

  const auto& defense_order_table = state.new_enum("DefenseOrder",
    "Always", DefenseOrder::always,
    "CollisionOnly", DefenseOrder::collisionOnly
  );

  const auto& particle_impact_type_table = state.new_enum("ParticleType",
    "Blue", ParticleImpact::Type::blue,
    "Fire", ParticleImpact::Type::fire,
    "Green", ParticleImpact::Type::green,
    "Thin", ParticleImpact::Type::thin,
    "Volcano", ParticleImpact::Type::volcano,
    "Vulcan", ParticleImpact::Type::vulcan,
    "Wind", ParticleImpact::Type::wind,
    "Yellow", ParticleImpact::Type::yellow
  );

  const auto& elements_table = state.new_enum("Element",
    "Fire", Element::fire,
    "Aqua", Element::aqua,
    "Elec", Element::elec,
    "Wood", Element::wood,
    "Sword", Element::sword,
    "Wind", Element::wind,
    "Cursor", Element::cursor,
    "Summon", Element::summon,
    "Plus", Element::plus,
    "Break", Element::breaker,
    "None", Element::none,
    "Ice", Element::ice
  );

  const auto& direction_table = state.new_enum("Direction",
    "None", Direction::none,
    "Up", Direction::up,
    "Down", Direction::down,
    "Left", Direction::left,
    "Right", Direction::right,
    "UpLeft", Direction::up_left,
    "UpRight", Direction::up_right,
    "DownLeft", Direction::down_left,
    "DownRight", Direction::down_right
  );

  const auto& animation_mode_record = state.new_enum("Playback",
    "Once", Animator::Mode::NoEffect,
    "Loop", Animator::Mode::Loop,
    "Bounce", Animator::Mode::Bounce,
    "Reverse", Animator::Mode::Reverse
  );

  const auto& team_record = state.new_enum("Team",
    "Red", Team::red,
    "Blue", Team::blue,
    "Other", Team::unknown
  );

  const auto& highlight_record = state.new_enum("Highlight",
    "Solid", Battle::Tile::Highlight::solid,
    "Flash", Battle::Tile::Highlight::flash,
    "None", Battle::Tile::Highlight::none
  );

  const auto& add_status_record = state.new_enum("EntityStatus",
    "Queued", Field::AddEntityStatus::queued,
    "Added", Field::AddEntityStatus::added,
    "Failed", Field::AddEntityStatus::deleted
  );

  const auto& action_order_record = state.new_enum("ActionOrder",
    "Involuntary", ActionOrder::involuntary,
    "Voluntary", ActionOrder::voluntary,
    "Immediate", ActionOrder::immediate
  );

  auto input_event_record = state.create_table("Input");

  const auto& pressed_input_event_record = input_event_record.new_enum("Pressed",
    "Up", InputEvents::pressed_move_up,
    "Left", InputEvents::pressed_move_left,
    "Right", InputEvents::pressed_move_right,
    "Down", InputEvents::pressed_move_down,
    "Use", InputEvents::pressed_use_chip,
    "Special", InputEvents::pressed_special,
    "Shoot", InputEvents::pressed_shoot
  );

  const auto& released_input_event_record = input_event_record.new_enum("Released",
    "Up", InputEvents::released_move_up,
    "Left", InputEvents::released_move_left,
    "Right", InputEvents::released_move_right,
    "Down", InputEvents::released_move_down,
    "Use", InputEvents::released_use_chip,
    "Special", InputEvents::released_special,
    "Shoot", InputEvents::released_shoot
  );

  const auto& hitbox_flags_record = state.new_enum("Hit",
    "None", Hit::none,
    "Flinch", Hit::flinch,
    "Flash", Hit::flash,
    "Stun", Hit::stun,
    "Root", Hit::root,
    "Impact", Hit::impact,
    "Shake", Hit::shake,
    "Pierce", Hit::pierce,
    "Retangible", Hit::retangible,
    "Breaking", Hit::breaking,
    "Bubble", Hit::bubble,
    "Freeze", Hit::freeze,
    "Drag", Hit::drag
  );

  const auto& character_rank_record = state.new_enum("Rank",
    "V1", Character::Rank::_1,
    "V2", Character::Rank::_2,
    "V3", Character::Rank::_3,
    "SP", Character::Rank::SP,
    "EX", Character::Rank::EX,
    "Rare1", Character::Rank::Rare1,
    "Rare2", Character::Rank::Rare2,
    "NM", Character::Rank::NM
  );

  const auto& audio_type_record = state.new_enum("AudioType", 
    "CounterBonus", AudioType::COUNTER_BONUS,
    "DirTile", AudioType::DIR_TILE,
    "Fanfare", AudioType::FANFARE,
    "Appear", AudioType::APPEAR,
    "AreaGrab", AudioType::AREA_GRAB,
    "AreaGrabTouchdown", AudioType::AREA_GRAB_TOUCHDOWN,
    "BusterPea", AudioType::BUSTER_PEA,
    "BusterCharged", AudioType::BUSTER_CHARGED,
    "BusterCharging", AudioType::BUSTER_CHARGING,
    "BubblePop", AudioType::BUBBLE_POP,
    "BubbleSpawn", AudioType::BUBBLE_SPAWN,
    "GuardHit", AudioType::GUARD_HIT,
    "Cannon", AudioType::CANNON,
    "Counter", AudioType::COUNTER,
    "Wind", AudioType::WIND,
    "ChipCancel", AudioType::CHIP_CANCEL,
    "ChipChoose", AudioType::CHIP_CHOOSE,
    "ChipConfirm", AudioType::CHIP_CONFIRM,
    "ChipDesc", AudioType::CHIP_DESC,
    "ChipDescClose", AudioType::CHIP_DESC_CLOSE,
    "ChipError", AudioType::CHIP_ERROR,
    "CustomBarFull", AudioType::CUSTOM_BAR_FULL,
    "CustomScreenOpen", AudioType::CUSTOM_SCREEN_OPEN,
    "ItemGet", AudioType::ITEM_GET,
    "Deleted", AudioType::DELETED,
    "Explode", AudioType::EXPLODE,
    "Gun", AudioType::GUN,
    "Hurt", AudioType::HURT,
    "PanelCrack", AudioType::PANEL_CRACK,
    "PanelReturn", AudioType::PANEL_RETURN,
    "Pause", AudioType::PAUSE,
    "PreBattle", AudioType::PRE_BATTLE,
    "Recover", AudioType::RECOVER,
    "Spreader", AudioType::SPREADER,
    "SwordSwing", AudioType::SWORD_SWING,
    "TossItem", AudioType::TOSS_ITEM,
    "TossItemLite", AudioType::TOSS_ITEM_LITE,
    "Wave", AudioType::WAVE,
    "Thunder", AudioType::THUNDER,
    "ElecPulse", AudioType::ELECPULSE,
    "Invisible", AudioType::INVISIBLE,
    "ProgramAdvance", AudioType::PA_ADVANCE,
    "LowHP", AudioType::LOW_HP,
    "DarkCard", AudioType::DARK_CARD,
    "PointSFX", AudioType::POINT_SFX,
    "NewGame", AudioType::NEW_GAME,
    "Text", AudioType::TEXT,
    "Shine", AudioType::SHINE,
    "TimeFreeze", AudioType::TIME_FREEZE,
    "Meteor", AudioType::METEOR,
    "Deform", AudioType::DEFORM
  );

  const auto& audio_priority_record = state.new_enum("AudioPriority",
    "Lowest", AudioPriority::lowest,
    "Low", AudioPriority::low,
    "High", AudioPriority::high,
    "Highest", AudioPriority::highest
  );

  const auto& hitbox_props_record = state.new_usertype<Hit::Properties>("HitProps",
    "aggressor", &Hit::Properties::aggressor,
    "damage", &Hit::Properties::damage,
    "drag", &Hit::Properties::drag,
    "element", &Hit::Properties::element,
    "flags", &Hit::Properties::flags
  );

  const auto& hitbox_drag_prop_record = state.new_usertype<Hit::Drag>("Drag",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Drag", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Drag", key );
    },
    "direction", &Hit::Drag::dir,
    "count", &Hit::Drag::count
  );

  const auto& card_class_record = state.new_enum("CardClass",
    "Standard", Battle::CardClass::standard,
    "Mega", Battle::CardClass::mega,
    "Giga", Battle::CardClass::giga,
    "Dark", Battle::CardClass::dark
  );

  const auto& component_lifetimes_record = state.new_enum("Lifetimes",
    "Local", Component::lifetimes::local,
    "Battlestep", Component::lifetimes::battlestep,
    "Scene", Component::lifetimes::ui
  );

  const auto& prog_blocks_record = state.new_enum("Blocks",
    "White", PlayerCustScene::Piece::Types::white,
    "Red", PlayerCustScene::Piece::Types::red,
    "Green", PlayerCustScene::Piece::Types::green,
    "Blue", PlayerCustScene::Piece::Types::blue,
    "Pink", PlayerCustScene::Piece::Types::pink,
    "Yellow", PlayerCustScene::Piece::Types::yellow
  );

  const auto& colormode_record = state.new_enum("ColorMode",
    "Multiply", ColorMode::multiply,
    "Additive", ColorMode::additive
  );

  state.set_function("drag",
    [](Direction dir, unsigned count) { return Hit::Drag{ dir, count }; }
  );

  state.set_function("no_drag",
    []() { return Hit::Drag{ Direction::none, 0 }; }
  );

  state.set_function("make_hit_props",
    [](int damage, Hit::Flags flags, Element element, Entity::ID_t aggressor, Hit::Drag drag) {
        return Hit::Properties{ damage, flags, element, aggressor, drag };
    }
  );

  state.set_function("create_move_event", []() { return MoveEvent{}; } );

  const auto& move_event_record = state.new_usertype<MoveEvent>("MoveEvent",
    "delta_frames", &MoveEvent::deltaFrames,
    "delay_frames", &MoveEvent::delayFrames,
    "endlag_frames",&MoveEvent::endlagFrames,
    "height", &MoveEvent::height,
    "dest_tile", &MoveEvent::dest,
    "on_begin_func", &MoveEvent::onBegin
  );

  const auto& card_event_record = state.new_usertype<CardEvent>("CardEvent");

  const auto& explosion_record = battle_namespace.new_usertype<Explosion>("Explosion",
    sol::factories([](int count, double speed) { return new Explosion(count, speed); }),
    sol::base_classes, sol::bases<Artifact>()
  );

  const auto& particle_poof = battle_namespace.new_usertype<ParticlePoof>("ParticlePoof",
    sol::constructors<ParticlePoof()>(),
    sol::base_classes, sol::bases<Artifact>()
  );

  const auto& particle_impact = battle_namespace.new_usertype<ParticleImpact>("ParticleImpact",
    sol::constructors<ParticleImpact(ParticleImpact::Type) >(),
    sol::base_classes, sol::bases<Artifact>()
  );

  const auto& color_record = state.new_usertype<sf::Color>("Color",
    sol::constructors<sf::Color(sf::Uint8, sf::Uint8, sf::Uint8, sf::Uint8)>(),
    "r", &sf::Color::r,
    "g", &sf::Color::g,
    "b", &sf::Color::b,
    "a", &sf::Color::a
  );

  const auto& vector_record = state.new_usertype<sf::Vector2f>("Vector2",
    "x", &sf::Vector2f::x,
    "y", &sf::Vector2f::y
  );

  state.set_function("frames",
    [](unsigned num) { return frames(num); }
  );

  state.set_function( "make_frame_data", &CreateFrameData );

  state.set_function("reverse_dir",
    [](Direction dir) { return Reverse(dir); }
  );

  state.set_function("flip_x_dir",
    [](Direction dir) { return FlipHorizontal(dir);  }
  );

  state.set_function("flip_y_dir",
    [](Direction dir) { return FlipVertical(dir);  }
  );

  engine_namespace.set_function("define_library",
    [this]( const std::string& fqn, const std::string& path )
    {
      Logger::Log("fqn: " + fqn + " , path: " + path );
      this->DefineLibrary( fqn, path );
    }
  );

  engine_namespace.set_function("action_from_card",
    [this](const std::string& fqn)
    {

    }
  );
}

ScriptResourceManager& ScriptResourceManager::GetInstance()
{
  static ScriptResourceManager instance;
  return instance;
}

ScriptResourceManager::~ScriptResourceManager()
{
  scriptTableHash.clear();
  for (auto ptr : states) {
    delete ptr;
  }
  states.clear();
}

ScriptResourceManager::LoadScriptResult& ScriptResourceManager::InitLibrary( const std::string& path )
{
  auto iter = scriptTableHash.find( path );

  if (iter != scriptTableHash.end()) {
    return iter->second;
  }

  sol::state* lua = new sol::state;

  lua->set_exception_handler(&::exception_handler);
  states.push_back(lua);

  auto load_result = lua->safe_script_file(path, sol::script_pass_on_error);
  auto pair = scriptTableHash.emplace(path, LoadScriptResult{std::move(load_result), lua} );

  return pair.first->second;
}

ScriptResourceManager::LoadScriptResult& ScriptResourceManager::LoadScript(const std::filesystem::path& modDirectory)
{
  auto entryPath = modDirectory / "entry.lua";
  auto modpath = modDirectory;

  auto iter = scriptTableHash.find(entryPath.filename().generic_string());

  if (iter != scriptTableHash.end()) {
    return iter->second;
  }

  sol::state* lua = new sol::state;
  
  SetModPathVariable( lua, modDirectory );
  SetSystemFunctions( lua );
  ConfigureEnvironment( *lua  );

  lua->set_exception_handler(&::exception_handler);
  states.push_back(lua);

  auto load_result = lua->safe_script_file(entryPath.generic_string(), sol::script_pass_on_error);
  auto pair = scriptTableHash.emplace(entryPath.generic_string(), LoadScriptResult{std::move(load_result), lua} );
  return pair.first->second;
}

void ScriptResourceManager::DefineCharacter(const std::string& fqn, const std::string& path)
{
  auto iter = characterFQN.find(fqn);

  if (iter == characterFQN.end()) {
    auto& res = LoadScript(path);

    if (res.result.valid()) {
      characterFQN[fqn] = path;
    }
    else {
      sol::error error = res.result;
      Logger::Logf("Failed to Require character with FQN %s. Reason: %s", fqn.c_str(), error.what());

      // bubble up to end this scene from loading that needs this character...
      throw std::exception(error);
    }
  }
}

void ScriptResourceManager::DefineLibrary( const std::string& fqn, const std::string& path )
{
  Logger::Log( "Loading Library ... " );
  auto iter = libraryFQN.find(fqn);

  if( iter == libraryFQN.end() )
  {
    auto& res = InitLibrary( path );

    if( res.result.valid() )
      libraryFQN[fqn] = path;
    else
    {
      sol::error error = res.result;
      Logger::Logf("Failed to Require library with FQN %s. Reason %s", fqn.c_str(), error.what() );

      throw std::exception( error );
    }
  }
}

sol::state* ScriptResourceManager::FetchCharacter(const std::string& fqn)
{
  auto iter = characterFQN.find(fqn);

  if (iter != characterFQN.end()) {
    return LoadScript(iter->second).state;
  }

  // else miss
  return nullptr;
}

const std::string& ScriptResourceManager::FetchSharedLibraryPath(const std::string& fqn)
{
  static std::string empty = "";

  auto iter = libraryFQN.find(fqn);

  if (iter != libraryFQN.end()) {
    return iter->second;
  }

  // else miss
  return empty;
}

const std::string& ScriptResourceManager::CharacterToModpath(const std::string& fqn) {
  return characterFQN[fqn];
}

void ScriptResourceManager::SeedRand(unsigned int seed)
{
  randSeed = seed;
}

#endif