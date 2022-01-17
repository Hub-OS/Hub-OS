#pragma once
#ifdef BN_MOD_SUPPORT

#include "bnLogger.h"

#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <atomic>
#include <filesystem>
#include <list>
#include "stx/result.h"

#ifdef __unix__
#define LUA_USE_POSIX 1
#endif

#include <sol/sol.hpp>
#include "bnPackageAddress.h"

class CardPackagePartitioner;

enum class ScriptPackageType : uint8_t {
  card,
  character,
  library,
  other,
  any
};

struct ScriptPackage {
  sol::state* state{ nullptr };
  ScriptPackageType type;
  PackageAddress address;
  std::filesystem::path path;
  std::vector<std::string> subpackages;
  std::vector<std::string> dependencies;
};

class ScriptResourceManager {
public:
  ~ScriptResourceManager();

  stx::result_t<sol::state*> LoadScript(const std::string& namespaceId, const std::filesystem::path& path, ScriptPackageType type = ScriptPackageType::other);

  void DropPackageData(sol::state* state);
  void DropPackageData(const PackageAddress& addr);
  ScriptPackage* DefinePackage(ScriptPackageType type, const std::string& namespaceId, const std::string& fqn, const std::filesystem::path& path); /* throws */
  ScriptPackage* FetchScriptPackage(const std::string& namespaceId, const std::string& fqn, ScriptPackageType type);
  void SetCardPackagePartitioner(CardPackagePartitioner& partition);
  CardPackagePartitioner& GetCardPackagePartitioner();

  static sol::object PrintInvalidAccessMessage(sol::table table, const std::string typeName, const std::string key );
  static sol::object PrintInvalidAssignMessage(sol::table table, const std::string typeName, const std::string key );

private:
  std::map<sol::state*, ScriptPackage*> state2package; /*!< lua state pointer to script package */
  std::map<PackageAddress, ScriptPackage*> address2package; /*!< PackageAddress to script package */
  CardPackagePartitioner* cardPartition{ nullptr };

  void ConfigureEnvironment(ScriptPackage& scriptPackage);
  void DefineSubpackage(ScriptPackage& parentPackage, ScriptPackageType type, const std::string& fqn, const std::filesystem::path& path); /* throws */
  void SetSystemFunctions(ScriptPackage& scriptPackage);
  void SetModPathVariable(sol::state& state, const std::filesystem::path& modDirectory);

  static std::string GetCurrentLine( lua_State* L );
  static stx::result_t<std::filesystem::path> GetCurrentFile(lua_State* L);
  static stx::result_t<std::filesystem::path> GetCurrentFolder(lua_State* L);
};

#endif
