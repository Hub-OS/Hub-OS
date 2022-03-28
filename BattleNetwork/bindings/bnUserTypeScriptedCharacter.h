#pragma once
#ifdef ONB_MOD_SUPPORT

#include <sol/sol.hpp>

class ScriptResourceManager;

void DefineScriptedCharacterUserType(ScriptResourceManager* scriptManager, const std::string& namespaceId, sol::state& state, sol::table& battle_namespace);

#endif
