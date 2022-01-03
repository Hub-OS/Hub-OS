#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>

void DefineScriptedPlayerUserType(sol::state& state, sol::table& battle_namespace);

#endif
