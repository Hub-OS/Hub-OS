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
//#include "bindings/bnScriptedObstacle.h"
#include "bindings/bnScriptedPlayer.h"

// temporary proof of concept includes...
#include "bnBusterCardAction.h"
#include "bnSwordCardAction.h"
#include "bnBombCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnCannonCardAction.h"

SOL_BASE_CLASSES(ScriptedSpell, Spell);

void ScriptResourceManager::ConfigureEnvironment(sol::state& state) {
  state.open_libraries(sol::lib::base);

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
    "Point", &Animation::GetPoint,
    "SetPlayback", sol::resolve<Animation&(char)>(&Animation::operator<<)
  );

  auto& animationcomp_record = engine_namespace.new_usertype<AnimationComponent>("AnimationComponent",
    "Load", &AnimationComponent::Load,
    "SetPath", &AnimationComponent::SetPath,
    "SetState", sol::resolve<void(std::string, char, FrameFinishCallback)>(&AnimationComponent::SetAnimation),
    "Copy", &AnimationComponent::GetAnimationObject,
    "CopyFrom", &AnimationComponent::CopyFrom,
    "Point", &AnimationComponent::GetPoint
  );

  auto& node_record = engine_namespace.new_usertype<SpriteProxyNode>("SpriteNode",
    sol::constructors<SpriteProxyNode()>(),
    "SetTexture", &SpriteProxyNode::setTexture,
    "SetLayer", &SpriteProxyNode::SetLayer,
    "SetPosition", sol::resolve<void(float, float)>(&SpriteProxyNode::setPosition),
    "Sprite", &SpriteProxyNode::getSprite,
    sol::base_classes, sol::bases<SceneNode>()
  );

  auto& tile_record = battle_namespace.new_usertype<Battle::Tile>("Tile",
    "X", &Battle::Tile::GetX,
    "Y", &Battle::Tile::GetY,
    "GetState", &Battle::Tile::GetState,
    "SetState", &Battle::Tile::SetState,
    "IsEdge", &Battle::Tile::IsEdgeTile,
    "IsCracked", &Battle::Tile::IsCracked,
    "IsHidden", &Battle::Tile::IsHidden,
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
    "SpawnSpell", sol::resolve<Field::AddEntityStatus(std::unique_ptr<Spell>&, int, int)>(&Field::AddEntity)
  );

  auto& player_record = battle_namespace.new_usertype<Player>("Player",
    sol::base_classes, sol::bases<Character>()
  );

  auto& scriptedspell_record = battle_namespace.new_usertype<ScriptedSpell>("Spell",
    sol::factories([](Team team) {
      return std::make_unique<ScriptedSpell>(team);
    }),
    "GetID", &ScriptedSpell::GetID,
    "SetHeight", &ScriptedSpell::SetHeight,
    "SetTexture", &ScriptedSpell::setTexture,
    "GetAnimation", &ScriptedSpell::GetAnimationComponent,
    "Tile", &ScriptedSpell::GetTile,
    "Field", &ScriptedSpell::GetField,
    "Move", &ScriptedSpell::Move,
    "SlideToTile", &ScriptedSpell::SlideToTile,
    "IsSliding", &ScriptedSpell::IsSliding,
    "SetSlideFrames", &ScriptedSpell::SetSlideTimeFrames,
    "ShowShadow", &ScriptedSpell::ShowShadow,
    "Teammate", &ScriptedSpell::Teammate,
    "AddNode", &ScriptedSpell::AddNode,
    "Delete", &ScriptedSpell::Delete,
    "attackFunc", &ScriptedSpell::attackCallback,
    "deleteFunc", &ScriptedSpell::deleteCallback,
    "updateFunc", &ScriptedSpell::updateCallback,
    "canMoveToFunc", &ScriptedSpell::canMoveToCallback,
    "onSpawnFunc", &ScriptedSpell::spawnCallback,
    //"ShakeCamera", &ScriptedSpell::ShakeCamera,
    sol::base_classes, sol::bases<Spell>()
  );

  auto& scriptedcharacter_record = battle_namespace.new_usertype<ScriptedCharacter>("ScriptedCharacter",
    "GetName", &ScriptedCharacter::GetName,
    "GetID", &ScriptedCharacter::GetID,
    "GetRank", &ScriptedCharacter::GetRank,
    "SetRank", &ScriptedCharacter::SetRank,
    "GetHealth", &ScriptedCharacter::GetHealth,
    "GetMaxHealth", &ScriptedCharacter::GetMaxHealth,
    "SetName", &ScriptedCharacter::SetName,
    "SetHealth", &ScriptedCharacter::SetHealth,
    "SetHeight", &ScriptedCharacter::SetHeight,
    "SetTexture", &ScriptedCharacter::setTexture,
    "GetAnimation", &ScriptedCharacter::GetAnimationComponent,
    "Tile", &ScriptedCharacter::GetTile,
    "Field", &ScriptedCharacter::GetField,
    "Target", &ScriptedCharacter::GetTarget,
    "Move", &ScriptedCharacter::Move,
    "SlideToTile", &ScriptedCharacter::SlideToTile,
    "IsSliding", &ScriptedCharacter::IsSliding,
    "SetSlideFrames", &ScriptedCharacter::SetSlideTimeFrames,
    "ShareTile", &ScriptedCharacter::ShareTileSpace,
    "Teammate", &ScriptedCharacter::Teammate,
    "AddNode", &ScriptedCharacter::AddNode,
    "ShakeCamera", &ScriptedCharacter::ShakeCamera,
    sol::base_classes, sol::bases<Character>()
  );

  auto& scriptedplayer_record = battle_namespace.new_usertype<ScriptedPlayer>("ScriptedPlayer",
    "GetName", &ScriptedPlayer::GetName,
    "GetID", &ScriptedPlayer::GetID,
    "GetHealth", &ScriptedPlayer::GetHealth,
    "GetMaxHealth", &ScriptedPlayer::GetMaxHealth,
    "SetName", &ScriptedPlayer::SetName,
    "SetHealth", &ScriptedPlayer::SetHealth,
    "SetTexture", &ScriptedPlayer::setTexture,
    "SetFullyChargeColor", &ScriptedPlayer::SetFullyChargeColor,
    "SetChargePosition", &ScriptedPlayer::SetChargePosition,
    "GetAnimation", &ScriptedPlayer::GetAnimationComponent,
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
    "SetOverworldTexture", &NaviRegistration::NaviMeta::SetOverworldTexture,
    "SetPreviewTexture", &NaviRegistration::NaviMeta::SetPreviewTexture,
    "SetIconTexture", &NaviRegistration::NaviMeta::SetIconTexture
  );

  auto elements_table = battle_namespace.new_enum("Element",
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