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
    
  };

#endif