#ifdef BN_MOD_SUPPORT
#include "bnScriptResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnResourceHandle.h"
#include "bnEntity.h"
#include "bnElements.h"

#include "bnNaviRegistration.h"
#include "bindings/bnScriptedCardAction.h"
#include "bindings/bnScriptedPlayer.h"

// temporary proof of concept includes...
#include "bnBusterCardAction.h"
#include "bnSwordCardAction.h"
#include "bnBombCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnCannonCardAction.h"

void ScriptResourceManager::ConfigureEnvironment(sol::state& state) {
  state.open_libraries(sol::lib::base);

  auto battle_namespace = state.create_table("Battle");
  auto overworld_namespace = state.create_table("Overworld");
  auto engine_namespace = state.create_table("Engine");

  // global namespace
  auto color_record = state.new_usertype<sf::Color>("Color",
    sol::constructors<sf::Color(sf::Uint8, sf::Uint8, sf::Uint8, sf::Uint8)>()
    );

  auto animation_record = battle_namespace.new_usertype<AnimationComponent>("Animation",
    "SetPath", &AnimationComponent::SetPath
  );

  auto player_record = battle_namespace.new_usertype<Player>("Player",
    sol::base_classes, sol::bases<Character>()
  );

  auto scriptedplayer_record = battle_namespace.new_usertype<ScriptedPlayer>("ScriptedPlayer",
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

  auto busteraction_record = battle_namespace.new_usertype<BusterCardAction>("Buster",
    sol::factories([](Character& character, bool charged, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<BusterCardAction>(character, charged, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  auto swordaction_record = battle_namespace.new_usertype<SwordCardAction>("Sword",
    sol::factories([](Character& character, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<SwordCardAction>(character, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  auto bombaction_record = battle_namespace.new_usertype<BombCardAction>("Bomb",
    sol::factories([](Character& character, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<BombCardAction>(character, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  auto fireburn_record = battle_namespace.new_usertype<FireBurnCardAction>("FireBurn",
    sol::factories([](Character& character, FireBurn::Type type, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<FireBurnCardAction>(character, type, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );


  auto cannon_record = battle_namespace.new_usertype<CannonCardAction>("Cannon",
    sol::factories([](Character& character, CannonCardAction::Type type, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<CannonCardAction>(character, type, dmg);
      }),
    sol::base_classes, sol::bases<CardAction>()
  );


  auto textureresource_record = engine_namespace.new_usertype<TextureResourceManager>("TextureResourceManager",
    "LoadFile", &TextureResourceManager::LoadTextureFromFile
  );

  auto audioresource_record = engine_namespace.new_usertype<AudioResourceManager>("AudioResourceMananger",
    "LoadFile", &AudioResourceManager::LoadFromFile
  );

  auto shaderresource_record = engine_namespace.new_usertype<ShaderResourceManager>("ShaderResourceManager",
    "LoadFile", &ShaderResourceManager::LoadShaderFromFile
  );

  // make resource handle metatable
  auto resourcehandle_record = engine_namespace.new_usertype<ResourceHandle>("ResourceHandle",
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
  auto navimeta_table = engine_namespace.new_usertype<NaviRegistration::NaviMeta>("NaviMeta",
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
    "FIRE", Element::fire,
    "AQUA", Element::aqua,
    "ELEC", Element::elec,
    "WOOD", Element::wood,
    "SWORD", Element::sword,
    "WIND", Element::wind,
    "CURSOR", Element::cursor,
    "SUMMON", Element::summon,
    "PLUS", Element::plus,
    "BREAK", Element::breaker,
    "NONE", Element::none,
    "ICE", Element::ice
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