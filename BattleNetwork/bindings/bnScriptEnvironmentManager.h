#pragma once
#ifdef BN_MOD_SUPPORT

#include <filesystem>
#include <sol/sol.hpp>

#ifdef __unix__
#define LUA_USE_POSIX 1
#endif

class ScriptEnvironmentManager
  {
    public:    
    static void SetModPathVariable( sol::state* state, const std::filesystem::path& path );
    static void SetSystemFunctions( sol::state* state );
    static void ConfigureEnvironment( sol::state& state );

    private:
    static void DefineObject_Tile( sol::state& state );
    static sol::object PrintInvalidAccessMessage( sol::table table, const std::string typeName, const std::string key );
    static sol::object PrintInvalidAssignMessage( sol::table table, const std::string typeName, const std::string key );

    static std::string GetCurrentLine( lua_State* L );
  };

#endif