#include "bnScriptResourceManager.h"
#include "bnEntity.h"
#include "bnScriptedCharacter.h"
#include "bnElements.h"
#include "bnScriptedCardAction.h"

void ScriptResourceManager::ConfigureEnvironment() {
  luaState.open_libraries(sol::lib::base);

  auto battle_namespace = luaState.create_table("Battle");

  // make usertype metatable
  auto character_record = battle_namespace.new_usertype<ScriptedCharacter>("Character",
    sol::constructors<ScriptedCharacter(Character::Rank)>(),
    sol::base_classes, sol::bases<Entity>(),
    "GetName", &Character::GetName,
    "GetID", &Entity::GetID,
    "GetHealth", &Character::GetHealth,
    "GetMaxHealth", &Character::GetMaxHealth,
    "SetHealth", &Character::SetHealth
    );

  // TODO: register animation callback methods
  auto card_record = battle_namespace.new_usertype<ScriptedCardAction>("CardAction",
    sol::constructors<ScriptedCardAction(Character&, int)>(),
    sol::base_classes, sol::bases<CardAction>()
    );

  auto elements_table = battle_namespace.new_enum("Element");
  elements_table["FIRE"] = Element::fire;
  elements_table["AQUA"] = Element::aqua;
  elements_table["ELEC"] = Element::elec;
  elements_table["WOOD"] = Element::wood;
  elements_table["SWORD"] = Element::sword;
  elements_table["WIND"] = Element::wind;
  elements_table["CURSOR"] = Element::cursor;
  elements_table["SUMMON"] = Element::summon;
  elements_table["PLUS"] = Element::plus;
  elements_table["BREAK"] = Element::breaker;
  elements_table["NONE"] = Element::none;
  elements_table["ICE"] = Element::ice;

  try {
    luaState.script(
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
}

void ScriptResourceManager::AddToPaths(FileMeta pathInfo)
{
  paths.push_back(pathInfo);
}

void ScriptResourceManager::LoadAllSCripts(std::atomic<int>& status)
{
}
