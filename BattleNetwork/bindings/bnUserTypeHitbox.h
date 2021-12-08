#pragma once
#ifdef BN_MOD_SUPPORT

#include <optional>
#include <sol/sol.hpp>

void DefineHitboxUserTypes(sol::state& state, sol::table& battle_namespace);

#endif