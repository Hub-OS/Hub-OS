#pragma once
#include "bnScriptMetaType.h"
#include "bnLogger.h"

#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <atomic>

#define SOL_ALL_SAFETIES_ON 1
#define SOL_USING_CXX_LUA 1
#include "sol/sol.hpp"

// Building the c lib on windows failed. 
// Including the c files directly into source avoids static linking
// until it can be resolved
#include <lapi.c>
#include <lauxlib.c>
#include <lbaselib.c>
#include <lbitlib.c>
#include <lcode.c>
#include <lcorolib.c>
#include <lctype.c>
#include <ldblib.c>
#include <ldebug.c>
#include <ldo.c>
#include <ldump.c>
#include <lfunc.c>
#include <lgc.c>
#include <linit.c>
#include <liolib.c>
#include <llex.c>
#include <lmathlib.c>
#include <lmem.c>
#include <loadlib.c>
#include <lobject.c>
#include <lopcodes.c>
#include <loslib.c>
#include <lparser.c>
#include <lstate.c>
#include <lstring.c>
#include <lstrlib.c>
#include <ltable.c>
#include <ltablib.c>
#include <ltm.c>
#include <lundump.c>
#include <lutf8lib.c>
#include <lvm.c>
#include <lzio.c>

class ScriptResourceManager {
public:
  struct FileMeta {
    ScriptMetaType type;
    std::string path;
    std::string name;

    FileMeta() {
      type = ScriptMetaType::ERROR_STATE;
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
    static ScriptResourceManager* instance = new ScriptResourceManager();

    return *instance;
  }

  /**
 * @brief Loads all scripts
 * @param status Increases the count after each shader loads
 */
  void LoadAllSCripts(std::atomic<int> &status);
};

/*! \brief Shorthand to get instance of the manager */
#define SCRIPTS ScriptResourceManager::GetInstance()