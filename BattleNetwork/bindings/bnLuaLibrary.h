#pragma once
#ifdef ONB_MOD_SUPPORT

#include <sol/sol.hpp>

class LuaLibrary
{
    sol::state& script;

    public:
    LuaLibrary( sol::state& script );
};

#endif