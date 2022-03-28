#pragma once
#ifdef ONB_MOD_SUPPORT

#include <sol/sol.hpp>

class ScriptResourceManager;

void DefineScriptedCardActionUserType(const std::string& namespaceId, ScriptResourceManager* scriptManager, sol::table& battle_namespace);

#endif
