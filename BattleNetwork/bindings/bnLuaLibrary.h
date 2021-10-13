#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>

class LuaLibrary
{
    sol::state& script;

    public:
    LuaLibrary( sol::state& script );
};

#endif