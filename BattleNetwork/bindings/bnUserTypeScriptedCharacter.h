#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>

class ScriptResourceManager;

void DefineScriptedCharacterUserType(ScriptResourceManager* scriptManager, sol::state& state, sol::table& battle_namespace);

#endif
