#pragma once
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
  std::vector<sol::state*> states;
  std::map<std::string, LoadScriptResult> scriptTableHash; /*!< Script name to sol table hash */

  void ConfigureEnvironment(sol::state& state);

public:
  static ScriptResourceManager& GetInstance();

  ~ScriptResourceManager();

  LoadScriptResult LoadScript(const std::string& path);
};