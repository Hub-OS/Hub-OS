#pragma once
#ifdef BN_MOD_SUPPORT
#include <sol/sol.hpp>

#include "../bnLogger.h"

/**
 * @class dynamic_object
 * @brief allows sol representations of classes have dynamic properties
 */
struct dynamic_object {
  std::unordered_map<std::string, sol::object> entries;

  void dynamic_set(std::string key, sol::stack_object value) {
    auto it = entries.find(key);
    if (it == entries.cend()) {
      entries.insert(it, { std::move(key), std::move(value) });
    }
    else {
      std::pair<const std::string, sol::object>& kvp = *it;
      sol::object& entry = kvp.second;
      entry = sol::object(std::move(value));
    }
  }

  sol::object dynamic_get(std::string key) {
    auto it = entries.find(key);
    if (it == entries.cend()) {        
      Logger::Log( "Attempted to access '" + key + "'" );
      return sol::lua_nil;
    }
    return it->second;
  }
};
#endif
