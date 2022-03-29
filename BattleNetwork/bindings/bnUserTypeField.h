#ifdef ONB_MOD_SUPPORT
#pragma once

#include <sol/sol.hpp>
#include "bnWeakWrapper.h"
#include "../bnLogger.h"
#include "../bnSolHelpers.h"

void DefineFieldUserType(sol::table& battle_namespace);

template <typename E>
sol::as_table_t<std::vector<WeakWrapper<E>>> FilterEntities(std::vector<WeakWrapper<E>> entities, sol::stack_object queryObject) {
  sol::protected_function query = queryObject;

  auto newEndIt = std::remove_if(entities.begin(), entities.end(), [query](WeakWrapper<E>& entity) -> bool {
    auto result = CallLuaCallbackExpectingBool(query, entity);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
      return true;
    }

    return !result.value();
  });

  entities.erase(newEndIt, entities.end());

  return sol::as_table(entities);
}

#endif
