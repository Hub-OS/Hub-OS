#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>

void DefineBaseCardActionUserType(sol::state& state, sol::table& battle_namespace);

#endif
