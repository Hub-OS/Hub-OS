#ifdef BN_MOD_SUPPORT

#include "bnLuaLibrary.h"

LuaLibrary::LuaLibrary( sol::state& script ) :
    script( script )
    {
        script["package_requires_scripts"]();
    }

#endif