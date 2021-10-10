#ifdef BN_MOD_SUPPORT
#pragma once

#include <sol/sol.hpp>

#include "bnWeakWrapperChild.h"
#include "../bnSpriteProxyNode.h"

void DefineSpriteNodeUserType(sol::table& engine_namespace);

#endif
