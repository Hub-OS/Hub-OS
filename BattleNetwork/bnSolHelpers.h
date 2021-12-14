#pragma once
#include <sol/sol.hpp>
#include "stx/result.h"

template<typename Table, typename ...Args>
stx::result_t<sol::object> CallLuaFunction(Table& script, const std::string& functionName, Args... args)
{
  sol::object possible_func = script[functionName];

  if (possible_func.get_type() != sol::type::function) {
    std::string error = "Lua function \"" + functionName + "\" is not defined";
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

template<typename Result, typename Table, typename ...Args>
stx::result_t<Result> CallLuaFunctionExpectingValue(Table& script, const std::string& functionName, Args... args)
{
  auto result = CallLuaFunction(script, functionName, std::forward<Args>(args)...);

  if (result.is_error()) {
    return stx::error<Result>(result.error_cstr());
  }

  auto obj = result.value();

  if (!obj.valid() || !obj.template is<Result>()) {
    std::string error = "Lua function \"" + functionName + "\" returned an unexpected type";
    return stx::error<Result>(error);
  }

  Result value = obj.template as<Result>();
  return stx::ok<Result>(value);
}

template<typename ...Args>
stx::result_t<sol::object> CallLuaCallback(const sol::protected_function& func, Args... args) {
  auto result = func(std::forward<Args>(args)...);

  if(!result.valid()) {
    sol::error error = result;
    return stx::error<sol::object>(error.what());
  }

  sol::object obj = result;

  return stx::ok(obj);
}

template<typename Result, typename ...Args>
stx::result_t<Result> CallLuaCallbackExpectingValue(const sol::protected_function& func, Args... args)
{
  auto result = func(std::forward<Args>(args)...);

  if(!result.valid()) {
    sol::error error = result;
    return stx::error<Result>(error.what());
  }

  sol::object obj = result;

  if (!obj.valid() || !obj.template is<Result>()) {
    std::string error = "Lua callback returned an unexpected type";
    return stx::error<Result>(error);
  }

  Result value = obj.template as<Result>();
  return stx::ok<Result>(value);
}

template<typename ...Args>
stx::result_t<bool> CallLuaCallbackExpectingBool(const sol::protected_function& func, Args... args)
{
  auto result = func(std::forward<Args>(args)...);

  if(!result.valid()) {
    sol::error error = result;
    return stx::error<bool>(error.what());
  }

  sol::object obj = result;

  if (!obj.valid() || !obj.template is<bool>()) {
    return stx::ok(false);
  }

  return stx::ok(obj.template as<bool>());
}

template<typename ...Args>
stx::result_t<sol::object> CallLuaCallback(const sol::object& object, Args... args) {
  if (!object.valid() || object.get_type() != sol::type::function) {
    return stx::error<sol::object>("Expected function");
  }

  sol::protected_function func = object;
  return CallLuaCallback(func, std::forward<Args>(args)...);
}

template<typename Result, typename ...Args>
stx::result_t<Result> CallLuaCallbackExpectingValue(const sol::object& object, Args... args) {
  if (object.get_type() != sol::type::function) {
    return stx::error<Result>("Expected function");
  }

  sol::protected_function func = object;
  return CallLuaCallbackExpectingValue<Result>(func, std::forward<Args>(args)...);
}

template<typename ...Args>
stx::result_t<bool> CallLuaCallbackExpectingBool(const sol::object& object, Args... args) {
  if (object.get_type() != sol::type::function) {
    return stx::error<bool>("Expected function");
  }

  sol::protected_function func = object;
  return CallLuaCallbackExpectingBool(func, std::forward<Args>(args)...);
}

inline sol::object VerifyLuaCallback(sol::stack_object value) {
  auto object = sol::object(std::move(value));

  if (object.get_type() != sol::type::function) {
    throw std::runtime_error("Expected function");
  }

  return object;
}

inline void ExpectLuaFunction(sol::object object) {
  if (object.get_type() != sol::type::function) {
    throw std::runtime_error("Expected function");
  }
}
