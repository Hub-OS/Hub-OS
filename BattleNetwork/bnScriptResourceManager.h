#pragma once
#include "bnScriptMetaType.h"
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
  struct FileMeta {
    ScriptMetaType type;
    std::string path;
    std::string name;

    FileMeta() {
      type = ScriptMetaType::error;
    }

    FileMeta(const FileMeta& rhs) {
      type = rhs.type;
      path = rhs.path;
      name = rhs.name;
    }
  };

private:
  std::vector<FileMeta> paths; /*!< Scripts to load */
  std::map<std::string, ScriptMetaType> nameToTypeHash; /*!< Script name to type hash */
  std::map<std::string, sol::table> scriptTableHash; /*!< Script name to sol table hash */
  sol::state luaState;

  void ConfigureEnvironment();

public:
  void AddToPaths(FileMeta pathInfo);

  static ScriptResourceManager& GetInstance() {
    static ScriptResourceManager instance;

    return instance;
  }

  /**
 * @brief Loads all scripts
 * @param status Increases the count after each shader loads
 */
  void LoadAllScripts(std::atomic<int> &status);
};