#pragma once
#ifdef ONB_MOD_SUPPORT

#include <sol/sol.hpp>

#include "bnWeakWrapperChild.h"
#include "../bnAnimation.h"

using AnimationWrapper = WeakWrapperChild<void, Animation>;

void DefineAnimationUserType(sol::state& state, sol::table& engine_namespace);

#endif
