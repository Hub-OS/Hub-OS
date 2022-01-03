#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>

class ScriptResourceManager;

void DefineCardMetaUserTypes(ScriptResourceManager* scriptManager, sol::state& state, sol::table& battle_namespace, std::function<void(const std::string& packageId)> setPackageId);

#endif
