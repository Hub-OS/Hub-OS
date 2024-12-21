local Instancer = require("scripts/libs/instancer")

---@class ScriptNodeVariables
---@field private _player_variables table<Net.ActorId, table>
---@field private _bot_variables table<Net.ActorId, table>
---@field private _object_variables table<string, table<number, table>>
---@field private _area_variables table<string, table>
---@field private _instance_variables table<string, table>
---@field private _global_variables table
---@field private _disconnect_listener fun()
local ScriptNodeVariables = {
  DEFAULT_SCOPE = "Instance"
}

---@return ScriptNodeVariables
function ScriptNodeVariables:new()
  local player_variables = {}

  local disconnect_listener = function(event)
    player_variables[event.player_id] = nil
  end

  Net:on("player_disconnect", disconnect_listener)

  local variables = {
    _player_variables = player_variables,
    _bot_variables = {},
    _object_variables = {},
    _area_variables = {},
    _instance_variables = {},
    _global_variables = {},
    _disconnect_listener = disconnect_listener
  }
  setmetatable(variables, self)
  self.__index = self

  return variables
end

function ScriptNodeVariables:destroy()
  Net:remove_listener("player_disconnect", self._disconnect_listener)
end

---@param area_id string
function ScriptNodeVariables:clear_area(area_id)
  self._area_variables[area_id] = nil
  self._object_variables[area_id] = nil
end

---@param instance_id string
function ScriptNodeVariables:clear_instance(instance_id)
  self._instance_variables[instance_id] = nil
end

---@param player_id Net.ActorId
function ScriptNodeVariables:player_variables(player_id)
  return self._player_variables[player_id]
end

---@param bot_id Net.ActorId
function ScriptNodeVariables:bot_variables(bot_id)
  return self._bot_variables[bot_id]
end

---@param area_id string
---@param object_id string|number
function ScriptNodeVariables:object_variables(area_id, object_id)
  local object_map = self._object_variables[area_id]
  return object_map and object_map[tonumber(object_id)]
end

function ScriptNodeVariables:self_variables(context)
  if context.object_id then
    return self:object_variables(context.area_id, context.object_id)
  elseif context.bot_id then
    return self:bot_variables(context.bot_id)
  elseif context.player_id then
    return self:player_variables(context.player_id)
  end
end

---@param area_id string
function ScriptNodeVariables:area_variables(area_id)
  return self._area_variables[area_id]
end

---@param instance_id string
function ScriptNodeVariables:instance_variables(instance_id)
  return self._instance_variables[instance_id]
end

function ScriptNodeVariables:global_variables()
  return self._global_variables
end

---@param variable string
---@param up number?
function ScriptNodeVariables:resolve_local_variable(context, variable, up)
  if not context.variable_scopes then
    return
  end

  up = up or 0

  local locals = context.variable_scopes[#context.variable_scopes - up]

  return locals and locals[variable]
end

---@param player_id Net.ActorId
---@param variable string
function ScriptNodeVariables:resolve_player_variable(player_id, variable)
  local player_variables = self._player_variables[player_id]

  return player_variables and player_variables[variable]
end

---@param bot_id Net.ActorId
---@param variable string
function ScriptNodeVariables:resolve_bot_variable(bot_id, variable)
  local bot_variables = self._bot_variables[bot_id]

  return bot_variables and bot_variables[variable]
end

---@param area_id string
---@param object_id string
---@param variable string
function ScriptNodeVariables:resolve_object_variable(area_id, object_id, variable)
  local object_map = self._object_variables[area_id]

  if not object_map then
    return
  end

  local object_variables = object_map[tonumber(object_id)]

  if not object_variables then
    return
  end

  return object_variables[variable]
end

---@param variable string
function ScriptNodeVariables:resolve_self_variable(context, variable)
  if context.object_id then
    return self:resolve_object_variable(context.area_id, context.object_id, variable)
  elseif context.bot_id then
    return self:resolve_bot_variable(context.bot_id, variable)
  elseif context.player_id then
    return self:resolve_player_variable(context.player_id, variable)
  end
end

---@param area_id string
---@param variable string
function ScriptNodeVariables:resolve_area_variable(area_id, variable)
  local area_variables = self._area_variables[area_id]

  return area_variables and area_variables[variable]
end

---@param instance_id string
---@param variable string
function ScriptNodeVariables:resolve_instance_variable(instance_id, variable)
  local instance_variables = self._instance_variables[instance_id]

  return instance_variables and instance_variables[variable]
end

---@param variable string
function ScriptNodeVariables:resolve_global_variable(variable)
  return self._global_variables[variable]
end

---Resolves a variable optionally tagged with a scope. Format: `[scope: ]name`, Examples: `Instance: X`, `X`
---
---Supported scopes:
--- - `Local` tied to the current context. Resolves using `context.variable_scopes` (a list of tables).
--- - `Up` tied to the current context, allows access to the previous local scope after pushing a scope.
--- - `Self` tied to a object, bot, or player. Resolves using `context.object_id`, `context.bot_id`, and `context.player_id` in that order.
--- - `Area` tied to the area. Resolves using `context.area_id`.
--- - `Instance` tied to the instance or global scope when outside of an instance (the default). Resolves using `context.area_id`.
--- - `Global`
---@param tagged_variable string
function ScriptNodeVariables:resolve_variable(context, tagged_variable)
  local match_start, match_end = tagged_variable:find(": ")

  local variable = tagged_variable

  if match_start then
    scope_name = variable:sub(1, match_start - 1)
    variable = variable:sub(match_end + 1)
  else
    scope_name = self.DEFAULT_SCOPE
  end

  local scopes = {
    Local = function()
      return self:resolve_local_variable(context, variable)
    end,
    Up = function()
      return self:resolve_local_variable(context, variable, 1)
    end,
    Self = function()
      return self:resolve_self_variable(context, variable)
    end,
    Area = function()
      return self:resolve_area_variable(context.area_id, variable)
    end,
    Instance = function()
      local instance_id = Instancer:resolve_instance_id(context.area_id)

      if instance_id then
        return self:resolve_instance_variable(instance_id, variable)
      else
        return self._global_variables[variable]
      end
    end,
    Global = function()
      return self._global_variables[variable]
    end,
  }

  local scope_callback = scopes[scope_name]

  if not scope_callback then
    return nil
  end

  return scope_callback()
end

---@param variable string
---@param up number?
function ScriptNodeVariables:set_local_variable(context, variable, value, up)
  if not context.variable_scopes then
    context.variable_scopes = { {} }
  end

  up = up or 0

  local locals = context.variable_scopes[#context.variable_scopes - up]

  if not locals then
    return
  end

  locals[variable] = value
end

---@param player_id Net.ActorId
---@param variable string
function ScriptNodeVariables:set_player_variable(player_id, variable, value)
  local player_variables = self._player_variables[player_id]

  if not player_variables then
    if not Net.is_player(player_id) then
      return
    end

    player_variables = {}
    self._player_variables[player_id] = player_variables
  end

  player_variables[variable] = value
end

---@param bot_id Net.ActorId
---@param variable string
function ScriptNodeVariables:set_bot_variable(bot_id, variable, value)
  local bot_variables = self._bot_variables[bot_id]

  if not bot_variables then
    bot_variables = {}
    self._bot_variables[bot_id] = bot_variables
  end

  bot_variables[variable] = value
end

---@param area_id string
---@param object_id string | number
---@param variable string
function ScriptNodeVariables:set_object_variable(area_id, object_id, variable, value)
  local object_map = self._object_variables[area_id]

  if not object_map then
    return
  end

  local object_number = tonumber(object_id)

  if not object_number then
    return
  end

  local object_variables = object_map[object_number]

  if not object_variables then
    object_variables = {}
    object_map[object_number] = object_variables
  end

  object_variables[variable] = value
end

---@param variable string
function ScriptNodeVariables:set_self_variable(context, variable, value)
  if context.object_id then
    return self:set_object_variable(context.area_id, context.object_id, variable, value)
  elseif context.bot_id then
    return self:set_bot_variable(context.bot_id, variable, value)
  elseif context.player_ids then
    for _, player_id in ipairs(context.player_ids) do
      self:set_player_variable(player_id, variable, value)
    end
  elseif context.player_id then
    self:set_player_variable(context.player_id, variable, value)
  end
end

---@param area_id string
---@param variable string
function ScriptNodeVariables:set_area_variable(area_id, variable, value)
  local area_variables = self._area_variables[area_id]

  if not area_variables then
    area_variables = {}
    self._area_variables[area_id] = area_variables
  end

  area_variables[variable] = value
end

---@param instance_id string
---@param variable string
function ScriptNodeVariables:set_instance_variable(instance_id, variable, value)
  local instance_variables = self._instance_variables[instance_id]

  if not instance_variables then
    instance_variables = {}
    self._instance_variables[instance_id] = instance_variables
  end

  instance_variables[variable] = value
end

---@param variable string
function ScriptNodeVariables:set_global_variable(variable, value)
  self._global_variables[variable] = value
end

---Resolves a variable optionally tagged with a scope. Format: `[scope: ]name`, Examples: `Instance: X`, `X`
---
---Supported scopes:
--- - `Local` tied to the current context. Resolves using `context.variable_scopes` (a list of tables).
--- - `Up` tied to the current context, allows access to the previous local scope after pushing a scope.
--- - `Self` tied to a object, bot, or player. Resolves using `context.object_id`, `context.bot_id`, `context.player_ids`, and `context.player_id` in that order.
--- - `Area` tied to the area. Resolves using `context.area_id`.
--- - `Instance` tied to the instance or global scope when outside of an instance (the default). Resolves using `context.area_id`.
--- - `Global`
---@param tagged_variable string
---@param value any
function ScriptNodeVariables:set_variable(context, tagged_variable, value)
  local match_start, match_end = tagged_variable:find(": ")

  local variable = tagged_variable

  if match_start then
    scope_name = variable:sub(1, match_start - 1)
    variable = variable:sub(match_end + 1)
  else
    scope_name = self.DEFAULT_SCOPE
  end

  local scopes = {
    Local = function()
      self:set_local_variable(context, variable, value)
    end,
    Up = function()
      self:set_local_variable(context, variable, value, 1)
    end,
    Self = function()
      self:set_self_variable(context, variable, value)
    end,
    Area = function()
      self:set_area_variable(context.area_id, variable, value)
    end,
    Instance = function()
      local instance_id = Instancer:resolve_instance_id(context.area_id)

      if instance_id then
        self:set_instance_variable(instance_id, variable, value)
      else
        self:set_global_variable(variable, value)
      end
    end,
    Global = function()
      self:set_global_variable(variable, value)
    end,
  }

  local scope_callback = scopes[scope_name]

  if scope_callback then
    scope_callback()
  end
end

function ScriptNodeVariables:push_variable_scope(context)
  if not context.variable_scopes then
    context.variable_scopes = { {}, {} }
  else
    context.variable_scopes[#context.variable_scopes + 1] = {}
  end
end

function ScriptNodeVariables:pop_variable_scope(context)
  if not context.variable_scopes then
    context.variable_scopes = { {} }
  elseif #context.variable_scopes == 1 then
    context.variable_scopes[1] = {}
  else
    context.variable_scopes[#context.variable_scopes] = nil
  end
end

return ScriptNodeVariables
