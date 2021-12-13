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

#include "bnCard.h"
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnElements.h"
#include "bnField.h"
#include "bnParticlePoof.h"
#include "bnPlayerCustScene.h"

#include "bindings/bnLuaLibrary.h"
#include "bindings/bnScriptedMob.h"
#include "bindings/bnScriptedCard.h"
#include "bindings/bnScriptedCardAction.h"
#include "bindings/bnWeakWrapper.h"
#include "bindings/bnUserTypeAnimation.h"
#include "bindings/bnUserTypeSceneNode.h"
#include "bindings/bnUserTypeSpriteNode.h"
#include "bindings/bnUserTypeSyncNode.h"
#include "bindings/bnUserTypeField.h"
#include "bindings/bnUserTypeTile.h"
#include "bindings/bnUserTypeEntity.h"
#include "bindings/bnUserTypeHitbox.h"
#include "bindings/bnUserTypeBasicCharacter.h"
#include "bindings/bnUserTypeScriptedCharacter.h"
#include "bindings/bnUserTypeBasicPlayer.h"
#include "bindings/bnUserTypeScriptedPlayer.h"
#include "bindings/bnUserTypeScriptedSpell.h"
#include "bindings/bnUserTypeScriptedObstacle.h"
#include "bindings/bnUserTypeScriptedArtifact.h"
#include "bindings/bnUserTypeScriptedComponent.h"
#include "bindings/bnUserTypeCardMeta.h"
#include "bindings/bnUserTypeBaseCardAction.h"
#include "bindings/bnUserTypeScriptedCardAction.h"
#include "bindings/bnUserTypeDefenseRule.h"

// Useful prefabs to use in scripts...
#include "bnExplosion.h"

// temporary proof of concept includes...
#include "bnBusterCardAction.h"

namespace {
  int exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
    if (maybe_exception) {
      const std::exception& ex = *maybe_exception;
    }
    else {
      std::string message(description.data(), description.size());
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

void ScriptResourceManager::SetSystemFunctions( sol::state& state )
{
  state.open_libraries(sol::lib::base, sol::lib::debug, sol::lib::math, sol::lib::table);

  // 'include()' in Lua is intended to load a LIBRARY file of Lua code into the current lua state.
  // Currently only loads library files included in the SAME directory as the script file.
  // Has to capture a pointer to sol::state, the copy constructor was deleted, cannot capture a copy to reference.
  state.set_function( "include",
    [this, &state]( const std::string fileName ) -> sol::protected_function_result {
      #ifdef _DEBUG
      std::cout << "Including library: " << fileName << std::endl;
      #endif

      sol::protected_function_result result;

      // Prefer using the shared libraries if possible.
      // i.e. ones that were present in "mods/libs/"
      if( auto sharedLibPath = FetchSharedLibraryPath( fileName ); !sharedLibPath.empty() )
      {
        #ifdef _DEBUG
        Logger::Log(LogLevel::info, "Including shared library: " + fileName);
        #endif
        
        AddDependencyNote(state, fileName );

        result = state.do_file( sharedLibPath, sol::load_mode::any );
      }
      else
      {
        #ifdef _DEBUG
        std::cout << "Including local library: " << fileName << std::endl;
        #endif

        std::string modPathRef = state["_modpath"];
        result = state.do_file( modPathRef + fileName, sol::load_mode::any );
      }

      return result;
    }
  );
}

void ScriptResourceManager::AddDependencyNote(sol::state& state, const std::string& dependencyPackageID )
{
  // If "__dependencies" doesn't exist already, create it. We need it to exist.
  if(!state["__dependencies"].valid() )
    state.create_table("__dependencies");

  sol::lua_table t = state["__dependencies"];

  // Add the package dependency to the table.
  t.set( t.size() + 1, dependencyPackageID );
}

void ScriptResourceManager::RegisterDependencyNotes(sol::state& state)
{
  // If "_package_id" isn't set, something's gone wrong, return.
  if(!state["_package_id"].valid() )
  {
    #ifdef _DEBUG
    Logger::Log(LogLevel::critical, "No valid package ID while registering dependencies");
    #endif
    return;
  }
  
  // Retrieve the package ID from the state.
  std::string packageID = state["_package_id"];

  // If this doesn't have any dependencies (no entries in "__dependencies" from earlier), return.
  if(!state["__dependencies"].valid() )
  {
    #ifdef _DEBUG
    Logger::Log(LogLevel::info, packageID + ": No Dependency Table" );
    #endif
    return;
  }

  // Retrieve the table of dependencies set from earlier.
  sol::lua_table deps = state["__dependencies"];

  // Get the number of keys in that table, to iterate over.
  auto count = deps.size();

  std::list<std::string> dependencies;

  std::string depsString = "";

  for( int ind = 1; ind <= count; ++ind )
  {
    #ifdef _DEBUG
    depsString = depsString + deps.get<std::string>(ind) + ", ";
    #endif

    // Get the KEY of each member in the table, as those are the package IDs that this depends on.
    // Add them to the list.
    dependencies.emplace_back( deps.get<std::string>(ind) );
  }

  #ifdef _DEBUG
  Logger::Log(LogLevel::info, "Package: " + packageID );
  Logger::Log(LogLevel::info, "- Depends on " + depsString );
  #endif

  // Register the dependencies with the list in ScriptResourceManager.
  scriptDependencies[packageID] = dependencies;

  // Remove the "__dependencies" table secretly inserted into the sol state, we don't need it anymore.
  state["__dependencies"] = sol::nil;
}

void ScriptResourceManager::SetModPathVariable( sol::state& state, const std::filesystem::path& modDirectory )
{
  state["_modpath"] = modDirectory.generic_string() + "/";
}

// Free Function provided to a number of Lua types that will print an error message when attempting to access a key that does not exist.
// Will print the file and line the error occured in, as well as the invalid key and type. 
sol::object ScriptResourceManager::PrintInvalidAccessMessage( sol::table table, const std::string typeName, const std::string key )
{
  Logger::Log(LogLevel::critical, "[Script Error] in " + GetCurrentLine( table.lua_state() ) );
  Logger::Log(LogLevel::critical, "[Script Error] : Attempted to access \"" + key + "\" in type \"" + typeName + "\"." );
  Logger::Log(LogLevel::critical, "[Script Error] : " + key + " does not exist in " + typeName + "." );
  return sol::lua_nil;
}
// Free Function provided to a number of Lua types that will print an error message when attempting to assign to a key that exists in a system type.
// Will print the file and line the error occured in, as well as the invalid key and type. 
sol::object ScriptResourceManager::PrintInvalidAssignMessage( sol::table table, const std::string typeName, const std::string key )
{
  Logger::Log(LogLevel::critical, "[Script Error] in " + GetCurrentLine( table.lua_state() ) );
  Logger::Log(LogLevel::critical, "[Script Error] : Attempted to assign to \"" + key + "\" in type \"" + typeName + "\"." );
  Logger::Log(LogLevel::critical, "[Script Error] : " + typeName + " is read-only. Cannot assign new values to it." );
  return sol::lua_nil;
}
// Returns the current executing line in the Lua script.
// Format: \@[full_filename]:[line_number]
std::string ScriptResourceManager::GetCurrentLine( lua_State* L )
{
  lua_getglobal( L, "debug" );          // debug
  lua_getfield( L, -1, "getinfo" );     // debug.getinfo 
  lua_pushinteger( L, 2 );              // debug.getinfo ( 2 )
  lua_pushstring( L, "S" );             // debug.getinfo ( 2, "S" )

  if( lua_pcall( L, 2, 1, 0) != 0 )     // table
  {
    Logger::Log(LogLevel::critical, "Error running function \"debug.getinfo\"");
    Logger::Log(LogLevel::critical, std::string( lua_tostring(L, -1) ));
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
    Logger::Log(LogLevel::critical, "Error running function \"debug.getinfo\"");
    Logger::Log(LogLevel::critical, std::string( lua_tostring(L, -1) ));
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

  DefineFieldUserType(battle_namespace);
  DefineTileUserType(state);
  DefineAnimationUserType(state, engine_namespace);
  DefineSceneNodeUserType(engine_namespace);
  DefineSpriteNodeUserType(engine_namespace);
  DefineSyncNodeUserType(engine_namespace);
  DefineEntityUserType(battle_namespace);
  DefineHitboxUserTypes(state, battle_namespace);
  DefineBasicCharacterUserType(battle_namespace);
  DefineScriptedCharacterUserType(this, state, battle_namespace);
  DefineBasicPlayerUserType(battle_namespace);
  DefineScriptedPlayerUserType(state, battle_namespace);
  DefineScriptedSpellUserType(battle_namespace);
  DefineScriptedObstacleUserType(state, battle_namespace);
  DefineScriptedArtifactUserType(battle_namespace);
  DefineScriptedComponentUserType(state, battle_namespace);
  DefineCardMetaUserTypes(this, battle_namespace);
  DefineBaseCardActionUserType(state, battle_namespace);
  DefineScriptedCardActionUserType(this, battle_namespace);
  DefineDefenseRuleUserTypes(state, battle_namespace);

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
    "set_mutator", [](BlockMeta& self, sol::object mutatorObject) {
      self.mutator = [mutatorObject](Player& player) {
        sol::protected_function mutator = mutatorObject;
        auto result = mutator(WeakWrapper(player.shared_from_base<Player>()));

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(LogLevel::critical, error.what());
        }
      };
    },
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
    "get_field", [](ScriptedMob& o) { return WeakWrapper(o.GetField()); },
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
      return handle.Textures().LoadFromFile(path);
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

  engine_namespace.set_function("pitch_music",
    [](float pitch) {
      static ResourceHandle handle;
      handle.Audio().SetPitch(pitch);
    }
  );

  engine_namespace.set_function("define_character",
    [this, &state](const std::string& fqn, const std::string& path) {
      AddDependencyNote(state, fqn);
      this->DefineCharacter(fqn, path);
    }
  );

  engine_namespace.set_function("requires_character",
    [this, &state](const std::string& fqn)
    {
      AddDependencyNote(state, fqn);
      if (this->FetchCharacter(fqn) == nullptr) {
        std::string msg = "Failed to Require character with FQN " + fqn;
        Logger::Log(LogLevel::critical, msg);
        throw std::runtime_error(msg);
      }
    }
  );

  engine_namespace.set_function("define_card",
    [this, &state](const std::string& fqn, const std::string& path) {
      AddDependencyNote(state, fqn);
      this->DefineCard(fqn, path);
    }
  );

  engine_namespace.set_function("requires_card",
    [this, &state](const std::string& fqn)
    {
      AddDependencyNote(state, fqn);
      if (this->FetchCard(fqn) == nullptr) {
        std::string msg = "Failed to Require card with FQN " + fqn;
        Logger::Log(LogLevel::critical, msg);
        throw std::runtime_error(msg);
      }
    }
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
    "flip_x", [](Direction direction) { return FlipHorizontal(direction); },
    "flip_y", [](Direction direction) { return FlipVertical(direction); },
    "reverse", [](Direction direction) { return Reverse(direction); },
    "join", [](Direction a, Direction b) { return Join(a, b); },
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

  const auto& team_record = state.new_enum("Team",
    "Red", Team::red,
    "Blue", Team::blue,
    "Other", Team::unknown
  );

  const auto& highlight_record = state.new_enum("Highlight",
    "Solid", Battle::TileHighlight::solid,
    "Flash", Battle::TileHighlight::flash,
    "None", Battle::TileHighlight::none
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

  const auto& shadow_type_record = state.new_enum("Shadow",
    "None", Entity::Shadow::none,
    "Small", Entity::Shadow::small,
    "Big", Entity::Shadow::big
  );

  auto input_event_record = state.create_table("Input");

  input_event_record.new_enum("Pressed",
    "Up", InputEvents::pressed_move_up,
    "Left", InputEvents::pressed_move_left,
    "Right", InputEvents::pressed_move_right,
    "Down", InputEvents::pressed_move_down,
    "Use", InputEvents::pressed_use_chip,
    "Special", InputEvents::pressed_special,
    "Shoot", InputEvents::pressed_shoot
  );

  input_event_record.new_enum("Held",
    "Up", InputEvents::held_move_up,
    "Left", InputEvents::held_move_left,
    "Right", InputEvents::held_move_right,
    "Down", InputEvents::held_move_down,
    "Use", InputEvents::held_use_chip,
    "Special", InputEvents::held_special,
    "Shoot", InputEvents::held_shoot
  );

  input_event_record.new_enum("Released",
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

  const auto& card_class_record = state.new_enum("CardClass",
    "Standard", Battle::CardClass::standard,
    "Mega", Battle::CardClass::mega,
    "Giga", Battle::CardClass::giga,
    "Dark", Battle::CardClass::dark
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

  const auto& move_event_record = state.new_usertype<MoveEvent>("MoveEvent",
    sol::factories([] {
      return MoveEvent{};
    }),
    "delta_frames", &MoveEvent::deltaFrames,
    "delay_frames", &MoveEvent::delayFrames,
    "endlag_frames",&MoveEvent::endlagFrames,
    "height", &MoveEvent::height,
    "dest_tile", &MoveEvent::dest,
    "on_begin_func", sol::property(
      [](MoveEvent& event, sol::object onBeginObject) {
        event.onBegin = [onBeginObject] {
          sol::protected_function onBegin = onBeginObject;
          auto result = onBegin();

          if (!result.valid()) {
            sol::error error = result;
            Logger::Log(LogLevel::critical, error.what());
          }
        };
      }
    )
  );

  const auto& card_event_record = state.new_usertype<CardEvent>("CardEvent");

  const auto& explosion_record = battle_namespace.new_usertype<Explosion>("Explosion",
    sol::factories([](int count, double speed) -> WeakWrapper<Entity> {
      std::shared_ptr<Entity> artifact = std::make_shared<Explosion>(count, speed);
      auto wrappedArtifact = WeakWrapper(artifact);
      wrappedArtifact.Own();
      return wrappedArtifact;
    })
  );

  const auto& particle_poof = battle_namespace.new_usertype<ParticlePoof>("ParticlePoof",
    sol::factories([]() -> WeakWrapper<Entity> {
      std::shared_ptr<Entity> artifact = std::make_shared<ParticlePoof>();
      auto wrappedArtifact = WeakWrapper(artifact);
      wrappedArtifact.Own();
      return wrappedArtifact;
    })
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

  engine_namespace.set_function("define_library",
    [this]( const std::string& fqn, const std::string& path )
    {
      Logger::Log(LogLevel::info, "fqn: " + fqn + " , path: " + path );
      this->DefineLibrary( fqn, path );
    }
  );
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
  
  SetModPathVariable(*lua, modDirectory);
  SetSystemFunctions(*lua);
  ConfigureEnvironment(*lua);

  lua->set_exception_handler(&::exception_handler);
  states.push_back(lua);

  auto load_result = lua->safe_script_file(entryPath.generic_string(), sol::script_pass_on_error);
  auto pair = scriptTableHash.emplace(entryPath.generic_string(), LoadScriptResult{std::move(load_result), lua} );
  return pair.first->second;
}

void ScriptResourceManager::DefineCard(const std::string& fqn, const std::string& path)
{
  auto iter = cardFQN.find(fqn);

  if (iter == cardFQN.end()) {
    auto& res = LoadScript(path);

    if (res.result.valid()) {
      cardFQN[fqn] = path;
    }
    else {
      sol::error error = res.result;
      Logger::Logf(LogLevel::critical, "Failed to Require card with FQN %s. Reason: %s", fqn.c_str(), error.what());

      // bubble up to end this scene from loading that needs this character...
      throw std::exception(error);
    }
  }
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
      Logger::Logf(LogLevel::critical, "Failed to Require character with FQN %s. Reason: %s", fqn.c_str(), error.what());

      // bubble up to end this scene from loading that needs this character...
      throw std::exception(error);
    }
  }
}

void ScriptResourceManager::DefineLibrary( const std::string& fqn, const std::string& path )
{
  Logger::Log(LogLevel::info, "Loading Library ... " );
  auto iter = libraryFQN.find(fqn);

  if( iter == libraryFQN.end() )
  {
    auto& res = InitLibrary( path );

    if( res.result.valid() )
      libraryFQN[fqn] = path;
    else
    {
      sol::error error = res.result;
      Logger::Logf(LogLevel::critical, "Failed to Require library with FQN %s. Reason %s", fqn.c_str(), error.what() );

      throw std::exception( error );
    }
  }
}

sol::state* ScriptResourceManager::FetchCard(const std::string& fqn)
{
  auto iter = cardFQN.find(fqn);

  if (iter != cardFQN.end()) {
    return LoadScript(iter->second).state;
  }

  // else miss
  return nullptr;
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

void ScriptResourceManager::SetCardPackageManager(CardPackageManager& packageManager)
{
  cardPackages = &packageManager;
}

CardPackageManager& ScriptResourceManager::GetCardPackageManager()
{
  return *cardPackages;
}

#endif