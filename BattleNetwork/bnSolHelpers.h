#pragma once
#include <sol/sol.hpp>
#include "stx/result.h"

template<typename ...Args>
stx::result_t<sol::object> CallLuaFunction(sol::state& script, const std::string& functionName, Args&&... args)
{
  sol::object possible_func = script[functionName];

  if (possible_func.get_type() != sol::type::function) {
    std::string error = "Lua function \"" + functionName + "\" is not defined.";
    return stx::error<sol::object>(error);
  }

  sol::protected_function func = possible_func;
  auto result = func(std::forward<Args>(args)...);

  if(!result.valid()) {
    sol::error error = result;
    return stx::error<sol::object>(error.what());
  }

  sol::object obj = result;

  return stx::ok(obj);
}
