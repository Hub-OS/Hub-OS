#pragma once
#ifdef BN_MOD_SUPPORT

#include "bnLogger.h"

#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <atomic>

#ifdef __unix__
#define LUA_USE_POSIX 1
#endif
#include <sol/sol.hpp>

class ScriptResourceManager {
public:
  struct LoadScriptResult {
    sol::protected_function_result result;
    sol::state* state{ nullptr };
  };

private:
  unsigned int randSeed{};
  std::vector<sol::state*> states;
  std::map<std::string, LoadScriptResult> scriptTableHash; /*!< Script path to sol table hash */
  std::map<std::string, std::string> characterFQN; /*! character FQN to script path */
  void ConfigureEnvironment(sol::state& state);

public:
  static ScriptResourceManager& GetInstance();

  ~ScriptResourceManager();

  LoadScriptResult& LoadScript(const std::string& path);

  void DefineCharacter(const std::string& fqn, const std::string& path) /* throw std::exception */;
  sol::state* FetchCharacter(const std::string& fqn);
  const std::string& CharacterToModpath(const std::string& fqn);
  void SeedRand(unsigned int seed);
};

#endif