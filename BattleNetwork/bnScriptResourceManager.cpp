#ifdef BN_MOD_SUPPORT
#include "bnScriptResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnResourceHandle.h"
#include "bnEntity.h"
#include "bnElements.h"
#include "bnField.h"
#include "bnTile.h"

#include "bnNaviRegistration.h"
#include "bindings/bnScriptedCardAction.h"
#include "bindings/bnScriptedCharacter.h"
#include "bindings/bnScriptedSpell.h"
#include "bindings/bnScriptedObstacle.h"
#include "bindings/bnScriptedPlayer.h"
#include "bindings/bnScriptedDefenseRule.h"

// Useful prefabs to use in scripts...
#include "bnExplosion.h"

// temporary proof of concept includes...
#include "bnBusterCardAction.h"
#include "bnSwordCardAction.h"
#include "bnBombCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnCannonCardAction.h"

void ScriptResourceManager::ConfigureEnvironment(sol::state& state) {
  state.open_libraries(sol::lib::base, sol::lib::math);

  auto& battle_namespace = state.create_table("Battle");
  auto& overworld_namespace = state.create_table("Overworld");
  auto& engine_namespace = state.create_table("Engine");

  // global namespace
  auto& color_record = state.new_usertype<sf::Color>("Color",
    sol::constructors<sf::Color(sf::Uint8, sf::Uint8, sf::Uint8, sf::Uint8)>()
  );

  auto& vector_record = state.new_usertype<sf::Vector2f>("Vector2",
    "x", &sf::Vector2f::x,
    "y", &sf::Vector2f::y
  );

  auto& animation_record = engine_namespace.new_usertype<Animation>("Animation",
    sol::constructors<Animation(const std::string&), Animation(const Animation&)>(),
    "Load", &Animation::Load,
    "Update", &Animation::Update,
    "Refresh", &Animation::Refresh,
    "SetState", &Animation::SetAnimation,
    "CopyFrom", &Animation::CopyFrom,
    "State", &Animation::GetAnimationString,
    "Point", &Animation::GetPoint,
    "SetPlayback", sol::resolve<Animation&(char)>(&Animation::operator<<),
    "OnComplete", sol::resolve<void(const FrameCallback&)>(&Animation::operator<<),
    "OnFrame", &Animation::AddCallback,
    "OnInterrupt", &Animation::SetInterruptCallback
  );

  auto& defense_frame_state_judge_record = engine_namespace.new_usertype<DefenseFrameStateJudge>("DefenseFrameStateJudge",
    "BlockDamage", &DefenseFrameStateJudge::BlockDamage,
    "BlockImpact", &DefenseFrameStateJudge::BlockImpact,
    "IsDamageBlocked", &DefenseFrameStateJudge::IsDamageBlocked,
    "IsImpactBlocked", &DefenseFrameStateJudge::IsImpactBlocked,
    /*"AddTrigger", &DefenseFrameStateJudge::AddTrigger,*/
    "SignalDefenseWasPierced", &DefenseFrameStateJudge::SignalDefenseWasPierced
  );

  auto& defense_rule_record = engine_namespace.new_usertype<ScriptedDefenseRule>("DefenseRule",
    sol::constructors <ScriptedDefenseRule(const Priority, const DefenseOrder&)>(),
    "IsReplaced", &DefenseRule::IsReplaced,
    "canBlockFunc", &DefenseRule::CanBlock,
    "filterStatusesFunc", &DefenseRule::FilterStatuses,
    sol::base_classes, sol::bases<DefenseRule>()
  );


  auto& node_record = engine_namespace.new_usertype<SpriteProxyNode>("SpriteNode",
    sol::constructors<SpriteProxyNode()>(),
    "SetTexture", &SpriteProxyNode::setTexture,
    "SetLayer", &SpriteProxyNode::SetLayer,
    "Show", &SpriteProxyNode::Reveal,
    "Hide", &SpriteProxyNode::Hide,
    "SetPosition", sol::resolve<void(float, float)>(&SpriteProxyNode::setPosition),
    "GetPosition", &SpriteProxyNode::getPosition,
    "Sprite", &SpriteProxyNode::getSprite,
    "EnableParentShader", &SpriteProxyNode::EnableParentShader,
    sol::base_classes, sol::bases<SceneNode>()
  );

  auto& tile_record = battle_namespace.new_usertype<Battle::Tile>("Tile",
    "X", &Battle::Tile::GetX,
    "Y", &Battle::Tile::GetY,
    "Width", &Battle::Tile::GetWidth,
    "Height", &Battle::Tile::GetHeight,
    "GetState", &Battle::Tile::GetState,
    "SetState", &Battle::Tile::SetState,
    "IsEdge", &Battle::Tile::IsEdgeTile,
    "IsCracked", &Battle::Tile::IsCracked,
    "IsHole", &Battle::Tile::IsHole,
    "IsWalkable", &Battle::Tile::IsWalkable,
    "IsReserved", &Battle::Tile::IsReservedByCharacter,
    "Team", &Battle::Tile::GetTeam,
    "AttackEntities", &Battle::Tile::AffectEntities
  );

  auto& field_record = battle_namespace.new_usertype<Field>("Field",
    "TileAt", &Field::GetAt,
    "Width", &Field::GetWidth,
    "Height", &Field::GetHeight,
    "Spawn", sol::overload(
      sol::resolve<Field::AddEntityStatus(std::unique_ptr<ScriptedSpell>&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(std::unique_ptr<ScriptedObstacle>&, int, int)>(&Field::AddEntity)
    ),
    "SpawnFX", sol::resolve<Field::AddEntityStatus(Artifact&, int, int)>(&Field::AddEntity)
  );

  auto& player_record = battle_namespace.new_usertype<Player>("Player",
    sol::base_classes, sol::bases<Character>()
  );

  auto& explosion_record = battle_namespace.new_usertype<Explosion>("Explosion",
    sol::factories([](int count, double speed) {
      return new Explosion(count, speed);
    }),
    sol::base_classes, sol::bases<Artifact>()
  );

  // auto& frame_time_record = state.new_usertype<frame_time_t>("Frame");
  auto& move_event_record = state.new_usertype<MoveEvent>("MoveEvent");

  auto& scriptedspell_record = battle_namespace.new_usertype<ScriptedSpell>("Spell",
    sol::factories([](Team team) -> std::unique_ptr<ScriptedSpell> {
      return std::make_unique<ScriptedSpell>(team);
    }),
    sol::meta_function::index,
    &dynamic_object::dynamic_get,
    sol::meta_function::new_index,
    &dynamic_object::dynamic_set,
    sol::meta_function::length,
    [](dynamic_object& d) { return d.entries.size(); },
    "GetID", &ScriptedSpell::GetID,
    "SetHeight", &ScriptedSpell::SetHeight,
    "SetTexture", &ScriptedSpell::setTexture,
    "SetLayer", &ScriptedSpell::SetLayer,
    "GetAnimation", &ScriptedSpell::GetAnimationObject,
    "GetTile", &ScriptedSpell::GetTile,
    "Tile", &ScriptedSpell::GetCurrentTile,
    "Field", &ScriptedSpell::GetField,
    "Slide", &ScriptedSpell::Slide,
    "Jump", &ScriptedSpell::Jump,
    "Teleport", & ScriptedSpell::Teleport,
    "RawMoveEvent", &ScriptedSpell::RawMoveEvent,
    "IsSliding", &ScriptedSpell::IsSliding,
    "IsJumping", &ScriptedSpell::IsJumping,
    "IsTeleporting", &ScriptedSpell::IsTeleporting,
    "IsMoving", &ScriptedSpell::IsMoving,
    "SetPosition", &ScriptedSpell::SetTileOffset,
    "GetPosition", &ScriptedSpell::GetTileOffset,
    "ShowShadow", &ScriptedSpell::ShowShadow,
    "Teammate", &ScriptedSpell::Teammate,
    "AddNode", &ScriptedSpell::AddNode,
    "Delete", &ScriptedSpell::Delete,
    "HighlightTile", &ScriptedSpell::HighlightTile,
    "GetHitProps", &ScriptedSpell::GetHitboxProperties,
    "SetHitProps", &ScriptedSpell::SetHitboxProperties,
    "GetTileOffset", &ScriptedSpell::GetTileOffset,
    "ShakeCamera", &ScriptedSpell::ShakeCamera,
    "Remove", &ScriptedSpell::Remove,
    "attackFunc", &ScriptedSpell::attackCallback,
    "deleteFunc", &ScriptedSpell::deleteCallback,
    "updateFunc", &ScriptedSpell::updateCallback,
    "canMoveToFunc", &ScriptedSpell::canMoveToCallback,
    "onSpawnFunc", &ScriptedSpell::spawnCallback,
    sol::base_classes, sol::bases<Spell>()
  );

  auto& scriptedobstacle_record = battle_namespace.new_usertype<ScriptedObstacle>("Obstacle",
    sol::factories([](Team team) -> std::unique_ptr<ScriptedObstacle> {
      return std::make_unique<ScriptedObstacle>(team);
    }),
    sol::meta_function::index,
    &dynamic_object::dynamic_get,
    sol::meta_function::new_index,
    &dynamic_object::dynamic_set,
    sol::meta_function::length,
    [](dynamic_object& d) { return d.entries.size(); },
    "GetID", &ScriptedObstacle::GetID,
    "SetHeight", &ScriptedObstacle::SetHeight,
    "SetTexture", &ScriptedObstacle::setTexture,
    "GetName", &ScriptedObstacle::GetName,
    "GetHealth", &ScriptedObstacle::GetHealth,
    "GetMaxHealth", &ScriptedObstacle::GetMaxHealth,
    "SetName", &ScriptedObstacle::SetName,
    "SetHealth", &ScriptedObstacle::SetHealth,
    "SetHeight", &ScriptedObstacle::SetHeight,
    "SetLayer", &ScriptedObstacle::SetLayer,
    "GetAnimation", &ScriptedObstacle::GetAnimationObject,
    "SetPosition", &ScriptedObstacle::SetTileOffset,
    "GetPosition", &ScriptedObstacle::GetTileOffset,
    "GetTile", &ScriptedObstacle::GetTile,
    "Tile", &ScriptedObstacle::GetCurrentTile,
    "Field", &ScriptedObstacle::GetField,
    "Slide", &ScriptedObstacle::Slide,
    "Jump", &ScriptedObstacle::Jump,
    "Teleport", &ScriptedObstacle::Teleport,
    "RawMoveEvent", &ScriptedObstacle::RawMoveEvent,
    "IsSliding", &ScriptedObstacle::IsSliding,
    "IsJumping", &ScriptedObstacle::IsJumping,
    "IsTeleporting", &ScriptedObstacle::IsTeleporting,
    "IsMoving", &ScriptedObstacle::IsMoving,
    "ShowShadow", &ScriptedObstacle::ShowShadow,
    "Teammate", &ScriptedObstacle::Teammate,
    "AddNode", &ScriptedObstacle::AddNode,
    "Delete", &ScriptedObstacle::Delete,
    "HighlightTile", &ScriptedObstacle::HighlightTile,
    "GetHitProps", &ScriptedObstacle::GetHitboxProperties,
    "SetHitProps", &ScriptedObstacle::SetHitboxProperties,
    "IgnoreCommonAggressor", &ScriptedObstacle::IgnoreCommonAggressor,
    "ShakeCamera", &ScriptedObstacle::ShakeCamera,
    "Remove", &ScriptedObstacle::Remove,
    "ShareTile", &ScriptedObstacle::ShareTileSpace,
    "AddDefenseRule", &ScriptedObstacle::AddDefenseRule,
    "attackFunc", &ScriptedObstacle::attackCallback,
    "deleteFunc", &ScriptedObstacle::deleteCallback,
    "updateFunc", &ScriptedObstacle::updateCallback,
    "canMoveToFunc", &ScriptedObstacle::canMoveToCallback,
    "onSpawnFunc", &ScriptedObstacle::spawnCallback,
    sol::base_classes, sol::bases<Obstacle, Spell, Character>()
  );

  auto& scriptedcharacter_record = battle_namespace.new_usertype<ScriptedCharacter>("Character",
    "GetName", &ScriptedCharacter::GetName,
    "GetID", &ScriptedCharacter::GetID,
    "GetRank", &ScriptedCharacter::GetRank,
    "SetRank", &ScriptedCharacter::SetRank,
    "GetHealth", &ScriptedCharacter::GetHealth,
    "GetMaxHealth", &ScriptedCharacter::GetMaxHealth,
    "SetPosition", &ScriptedCharacter::SetTileOffset,
    "GetPosition", &ScriptedCharacter::GetTileOffset,
    "SetName", &ScriptedCharacter::SetName,
    "SetHealth", &ScriptedCharacter::SetHealth,
    "SetHeight", &ScriptedCharacter::SetHeight,
    "SetTexture", &ScriptedCharacter::setTexture,
    "GetAnimation", &ScriptedCharacter::GetAnimationObject,
    "GetTile", &ScriptedCharacter::GetTile,
    "Tile", &ScriptedCharacter::GetCurrentTile,
    "Field", &ScriptedCharacter::GetField,
    "Target", &ScriptedCharacter::GetTarget,
    "Slide", &ScriptedCharacter::Slide,
    "Jump", &ScriptedCharacter::Jump,
    "Teleport", &ScriptedCharacter::Teleport,
    "RawMoveEvent", &ScriptedCharacter::RawMoveEvent,
    "IsSliding", &ScriptedCharacter::IsSliding,
    "IsJumping", &ScriptedCharacter::IsJumping,
    "IsTeleporting", &ScriptedCharacter::IsTeleporting,
    "IsMoving", &ScriptedCharacter::IsMoving,
    "ShareTile", &ScriptedCharacter::ShareTileSpace,
    "Teammate", &ScriptedCharacter::Teammate,
    "AddNode", &ScriptedCharacter::AddNode,
    "ShakeCamera", &ScriptedCharacter::ShakeCamera,
    "AddDefenseRule", &ScriptedCharacter::AddDefenseRule,
    sol::base_classes, sol::bases<Character>()
  );

  auto& scriptedplayer_record = battle_namespace.new_usertype<ScriptedPlayer>("Player",
    "GetName", &ScriptedPlayer::GetName,
    "GetID", &ScriptedPlayer::GetID,
    "GetHealth", &ScriptedPlayer::GetHealth,
    "GetMaxHealth", &ScriptedPlayer::GetMaxHealth,
    "SetName", &ScriptedPlayer::SetName,
    "SetHealth", &ScriptedPlayer::SetHealth,
    "SetTexture", &ScriptedPlayer::setTexture,
    "SetElement", &ScriptedPlayer::SetElement,
    "GetTile", &ScriptedPlayer::GetTile,
    "Tile", &ScriptedPlayer::GetCurrentTile,
    "SetHeight",  &ScriptedPlayer::SetHeight,
    "SetFullyChargeColor", &ScriptedPlayer::SetFullyChargeColor,
    "SetChargePosition", &ScriptedPlayer::SetChargePosition,
    "GetAnimation", &ScriptedPlayer::GetAnimationObject,
    "SetAnimation", &ScriptedPlayer::SetAnimation,
    sol::base_classes, sol::bases<Player>()
   );

  auto& busteraction_record = battle_namespace.new_usertype<BusterCardAction>("Buster",
    sol::factories([](Character& character, bool charged, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<BusterCardAction>(character, charged, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  auto& swordaction_record = battle_namespace.new_usertype<SwordCardAction>("Sword",
    sol::factories([](Character& character, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<SwordCardAction>(character, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  auto& bombaction_record = battle_namespace.new_usertype<BombCardAction>("Bomb",
    sol::factories([](Character& character, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<BombCardAction>(character, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  auto& fireburn_record = battle_namespace.new_usertype<FireBurnCardAction>("FireBurn",
    sol::factories([](Character& character, FireBurn::Type type, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<FireBurnCardAction>(character, type, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );


  auto& cannon_record = battle_namespace.new_usertype<CannonCardAction>("Cannon",
    sol::factories([](Character& character, CannonCardAction::Type type, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<CannonCardAction>(character, type, dmg);
      }),
    sol::base_classes, sol::bases<CardAction>()
  );


  auto& textureresource_record = engine_namespace.new_usertype<TextureResourceManager>("TextureResourceManager",
    "LoadFile", &TextureResourceManager::LoadTextureFromFile
  );

  auto& audioresource_record = engine_namespace.new_usertype<AudioResourceManager>("AudioResourceMananger",
    "LoadFile", &AudioResourceManager::LoadFromFile,
    "Stream", sol::resolve<int(std::string, bool)>(&AudioResourceManager::Stream)
  );

  auto& shaderresource_record = engine_namespace.new_usertype<ShaderResourceManager>("ShaderResourceManager",
    "LoadFile", &ShaderResourceManager::LoadShaderFromFile
  );

  // make resource handle metatable
  auto& resourcehandle_record = engine_namespace.new_usertype<ResourceHandle>("ResourceHandle",
    sol::constructors<ResourceHandle()>(),
    "Textures", sol::property(sol::resolve<TextureResourceManager& ()>(&ResourceHandle::Textures)),
    "Audio", sol::property(sol::resolve<AudioResourceManager& ()>(&ResourceHandle::Audio)),
    "Shaders", sol::property(sol::resolve<ShaderResourceManager& ()>(&ResourceHandle::Shaders))
  );

  // make loading resources easier
  // DOESNT WORK??
  /*state.script(
    "-- Shorthand load texture"
    "function LoadTexture(path)"
    "  return Engine.ResourceHandle.new().Textures:LoadFile(path)"
    "end"

    "-- Shorthand load audio"
    "function LoadAudio(path)"
    "  return Engine.ResourceHandle.new().Audio:LoadFile(path)"
    "end"

    "-- Shorthand load shader"
    "function LoadShader(path)"
    "  return Engine.ResourceHandle.new().Shaders:LoadFile(path)"
    "end"
  );*/

  // make meta object info metatable
  auto& navimeta_table = engine_namespace.new_usertype<NaviRegistration::NaviMeta>("NaviMeta",
    "SetSpecialDescription", &NaviRegistration::NaviMeta::SetSpecialDescription,
    "SetAttack", &NaviRegistration::NaviMeta::SetAttack,
    "SetChargedAttack", &NaviRegistration::NaviMeta::SetChargedAttack,
    "SetSpeed", &NaviRegistration::NaviMeta::SetSpeed,
    "SetHP", &NaviRegistration::NaviMeta::SetHP,
    "SetIsSword", &NaviRegistration::NaviMeta::SetIsSword,
    "SetOverworldAnimationPath", &NaviRegistration::NaviMeta::SetOverworldAnimationPath,
    "SetOverworldTexturePath", &NaviRegistration::NaviMeta::SetOverworldTexturePath,
    "SetMugshotTexturePath", &NaviRegistration::NaviMeta::SetMugshotTexturePath,
    "SetMugshotAnimationPath", &NaviRegistration::NaviMeta::SetMugshotAnimationPath,
    "SetPreviewTexture", &NaviRegistration::NaviMeta::SetPreviewTexture,
    "SetIconTexture", &NaviRegistration::NaviMeta::SetIconTexture
  );

  auto& defense_order_table = state.new_enum("DefenseOrder",
    "Always", DefenseOrder::always,
    "CollisionOnly", DefenseOrder::collisionOnly
  );

  auto& elements_table = state.new_enum("Element",
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

  auto& direction_table = state.new_enum("Direction",
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

  auto& animation_mode_record = state.new_enum("Playback",
    "Once", Animator::Mode::NoEffect,
    "Loop", Animator::Mode::Loop,
    "Bounce", Animator::Mode::Bounce,
    "Reverse", Animator::Mode::Reverse
  );

  auto& team_record = state.new_enum("Team",
    "Red", Team::red,
    "Blue", Team::blue,
    "Other", Team::unknown
  );

  auto& highlight_record = state.new_enum("Highlight",
    "Solid", Battle::Tile::Highlight::solid,
    "Flash", Battle::Tile::Highlight::flash,
    "None", Battle::Tile::Highlight::none
  );

  auto& add_status_record = state.new_enum("EntityStatus",
    "Queued", Field::AddEntityStatus::queued,
    "Added", Field::AddEntityStatus::added,
    "Failed", Field::AddEntityStatus::deleted
  );

  auto& action_order_record = state.new_enum("ActionOrder",
    "Involuntary", ActionOrder::involuntary,
    "Voluntary", ActionOrder::voluntary,
    "Immediate", ActionOrder::immediate
  );

  auto& hitbox_flags_record = state.new_enum("Hit",
    "None", Hit::none,
    "Recoil", Hit::recoil,
    "Flinch", Hit::flinch,
    "Stun", Hit::stun,
    "Impact", Hit::impact,
    "Shake", Hit::shake,
    "Pierce", Hit::pierce,
    "Retangible", Hit::retangible,
    "Breaking", Hit::breaking,
    "Bubble", Hit::bubble,
    "Freeze", Hit::freeze,
    "Drag", Hit::drag
  );

  auto& hitbox_drag_prop_record = state.new_usertype<Hit::Drag>("Drag",
    "direction", &Hit::Drag::dir,
    "count", &Hit::Drag::count
  );

  /**
    int damage{};
    Flags flags{ none };
    Element element{ Element::none };
    Character* aggressor{ nullptr };
    Direction drag{ Direction::none }; // Used by dragging payload
  */
  state.set_function("HitProps", 
    [](int damage, Hit::Flags flags, Element element, Character* aggressor, Hit::Drag drag) {
      return Hit::Properties{
        damage, flags, element, aggressor, drag
      };
    }
  );

  state.set_function("Drag",
    [](Direction dir, unsigned count) {
      return Hit::Drag{ dir, count };
    }
  );

  state.set_function("frames",
    [](unsigned num) {
      return frames(num);
    }
  );

  /*
  * // Script for field class support
  try {
    state.script(
      "--[[This system EXPECTS type Entity to have a GetID() function"
      "call to return a unique identifier"
      "--]]"

      "Handles = {}"
      "local memoizedFuncs = {}"

      "-- metatable which caches function calls"
      "local mt = {}"
      "mt.__index = function(handle, key)"
      "  if not handle.isValid then"
      "    print(debug.traceback())"
      "    error('Error: handle is not valid!', 2)"
      "  end"

      "  local f = memoizedFuncs[key]"
      "     if not f then"
      "       if handle.classType == 'Entity' then"
      "         f = function(handle, ...) return Battle.Entity[key](handle.cppRef, ...) end"
      "       elseif handle.classType == 'Character' then"
      "         f = function(handle, ...) return Battle.Character[key](handle.cppRef, ...) end"
      "       end"

      "       memoizedFuncs[key] = f"
      "     end"
      "  return f"
      "end"

      "-- exception catcher to avoid problems in C++"
      "function getWrappedSafeFunction(f)"
      "  return function(handle, ...)"
      "    if not handle.isValid then"
      "      print(debug.traceback())"
      "      error('Error: handle is not valid!', 2)"
      "    end"

      "    return f(handle.cppRef, ...)"
      "  end"
      "end"

      "-- To be called at the beginning of an Entity's lifetime"
      "  function createHandle(cppRef, classType)"
      "  local handle = {"
      "      cppRef = cppRef,"
      "      classType = classType,"
      "      isValid = true"
      "  }"

      "-- speedy access without __index call"
      "-- decrease call - time and wraps in an exception - catcher:"
      "-- handle.getName = getWrappedSafeFunction(Entity.getName)"

      "setmetatable(handle, mt)"
      "  Handles[cppRef:GetID()] = handle"
      "  return handle"
      "end"

      "-- To be called at the end of an Entity's lifetime"
      "function onEntityRemoved(cppRef)"
      "  local handle = Handles[cppRef:GetID()];"
      "  handle.cppRef = nil"
      "  handle.isValid = false"
      "  Handles[cppRef:GetID()] = nil"
      "end"
    );
  }
  catch (const sol::error& err) {
    Logger::Logf("[ShaderResourceManager] Something went wrong while configuring the environment: thrown error, %s", err.what());
  }
  */
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

ScriptResourceManager::LoadScriptResult& ScriptResourceManager::LoadScript(const std::string& path)
{
  auto iter = scriptTableHash.find(path);

  if (iter != scriptTableHash.end()) {
    return iter->second;
  }

  sol::state* lua = new sol::state;
  ConfigureEnvironment(*lua);
  states.push_back(lua);

  auto load_result = lua->safe_script_file(path, sol::script_pass_on_error);
  auto pair = scriptTableHash.emplace(path, LoadScriptResult{std::move(load_result), lua} );
  return pair.first->second;
}
#endif