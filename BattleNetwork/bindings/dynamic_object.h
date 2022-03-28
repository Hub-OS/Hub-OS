#pragma once
#ifdef ONB_MOD_SUPPORT
#include <sol/sol.hpp>

#include "../bnLogger.h"

/**
 * @class dynamic_object
 * @brief allows sol representations of classes have dynamic properties
 */
struct dynamic_object {
  std::unordered_map<std::string, sol::object> entries;

  inline void dynamic_set(const std::string& key, sol::stack_object value) {
    auto it = entries.find(key);
    if (it == entries.cend()) {
      entries.insert(it, { key, std::move(value) });
    }
    else {
      std::pair<const std::string, sol::object>& kvp = *it;
      sol::object& entry = kvp.second;
      entry = sol::object(std::move(value));
    }
  }

  inline sol::object dynamic_get(const std::string& key) {
    auto it = entries.find(key);
    if (it == entries.cend()) {        
      Logger::Log(LogLevel::info, "Attempted to access '" + key + "'" );
      return sol::lua_nil;
    }
    return it->second;
  }
};
#endif
