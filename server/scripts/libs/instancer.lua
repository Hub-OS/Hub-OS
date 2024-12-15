local TRANSFER_EVENTS = {
  "player_connect",
  "player_join",
  "player_area_transfer",
  "player_disconnect",
}

---Used to create and track instances, which contains isolated copies of areas.
---@class Instancer
---@field private _instances table<string, { areas: table<string>, player_list: Net.ActorId[] }>
---@field private _instance_create_callbacks fun(instance_id: string)[]
---@field private _instance_remove_callbacks fun(instance_id: string)[]
---@field private _area_instanced_callbacks fun(area_id: string)[]
---@field private _area_remove_callbacks fun(area_id: string)[]
---@field private _player_join_callbacks fun(player_id: Net.ActorId, area_id: string, previous_area_id: string?)[]
---@field private _player_leave_callbacks fun(player_id: Net.ActorId, previous_area_id: string?)[]
---@field private _transfer_listeners fun()[]
local Instancer = {
  INSTANCE_MARKER = ";instance:",
}

function Instancer:new()
  ---@type table<string, { areas: table<string>, player_list: Net.ActorId[] }>
  local instances = {}
  local transfer_listeners = {}
  local player_join_callbacks = {}
  local player_leave_callbacks = {}

  local instancer = {
    _instances = instances,
    _instance_create_callbacks = {},
    _instance_remove_callbacks = {},
    _area_instanced_callbacks = {},
    _area_remove_callbacks = {},
    _player_join_callbacks = player_join_callbacks,
    _player_leave_callbacks = player_leave_callbacks,
    _transfer_listeners = transfer_listeners,
  }
  setmetatable(instancer, self)
  self.__index = self

  ---@type Instancer
  instancer = instancer

  -- tracking to delete empty instances
  local player_areas = {}

  local function check_transfer(event_name, event)
    ---@type string | nil
    local previous_area = player_areas[event.player_id]
    ---@type string | nil
    local current_area = Net.get_player_area(event.player_id)

    if event_name == "player_disconnect" then
      current_area = nil
    end

    -- update area
    player_areas[event.player_id] = current_area

    if current_area == previous_area then
      -- no difference
      return
    end

    local previous_instance_id = previous_area and instancer:resolve_instance_id(previous_area)
    local current_instance_id = current_area and instancer:resolve_instance_id(current_area)

    if previous_instance_id == current_instance_id then
      -- no difference
      return
    end

    -- remove player from instance, and attempt to remove the instance if it's empty
    if previous_instance_id then
      local instance = instances[previous_instance_id]

      if instance then
        for i, player_id in ipairs(instance.player_list) do
          if player_id == event.player_id then
            -- swap remove
            instance.player_list[i] = instance[#instance.player_list]
            instance.player_list[#instance.player_list] = nil
            break
          end
        end

        if #instance.player_list == 0 then
          instancer:remove_instance(previous_instance_id)
        end

        for _, callback in ipairs(player_leave_callbacks) do
          callback(event.player_id, previous_area)
        end
      end
    end

    -- add player to instance
    if current_instance_id then
      local instance = instances[current_instance_id]

      if instance then
        instance.player_list[#instance.player_list + 1] = event.player_id

        for _, callback in ipairs(player_join_callbacks) do
          callback(event.player_id, current_area --[[@as string]], previous_area)
        end
      end
    end
  end

  for i, event_name in ipairs(TRANSFER_EVENTS) do
    transfer_listeners[i] = function(event)
      check_transfer(event_name, event)
    end
    Net:on(event_name, transfer_listeners[i])
  end

  return instancer
end

---Reads the instance id from the area id.
---@param area_id string
---@return string?
function Instancer:resolve_instance_id(area_id)
  local _, match_end = area_id:find(self.INSTANCE_MARKER)

  if match_end then
    return area_id:sub(match_end + 1)
  end
end

---Reads the template area id from the area id, or returns the area id.
---@param area_id string
function Instancer:resolve_base_area_id(area_id)
  local match_start = area_id:find(self.INSTANCE_MARKER)

  if match_start then
    return area_id:sub(1, match_start - 1)
  else
    return area_id
  end
end

---@param instance_id string
---@return Net.ActorId[]?
function Instancer:instance_player_list(instance_id)
  local instance = self._instances[instance_id]

  if instance then
    return instance.player_list
  end
end

---Adds a player join listener, called when a player is transferred to an instance
---@param callback fun(player_id: Net.ActorId, area_id: string, previous_area_id: string)
function Instancer:on_player_join(callback)
  self._player_join_callbacks[#self._player_join_callbacks + 1] = callback
end

---Adds a player leave listener, called when a player disconnects or is transferred out of an instance
---@param callback fun(player_id: Net.ActorId, previous_area_id: string)
function Instancer:on_player_leave(callback)
  self._player_leave_callbacks[#self._player_leave_callbacks + 1] = callback
end

---Used to create an instance, which contains isolated copies of areas.
function Instancer:create_instance()
  local instance_id = tostring(Net.system_random())
  self._instances[instance_id] = {
    areas = {},
    player_list = {}
  }

  for _, callback in ipairs(self._instance_create_callbacks) do
    callback(instance_id)
  end

  return instance_id
end

---Adds :create_instance() listener
---@param callback fun(instance_id: string)
function Instancer:on_instance_created(callback)
  self._instance_create_callbacks[#self._instance_create_callbacks + 1] = callback
end

---Calls :remove_area() for every area in the instance.
---@param instance_id string
function Instancer:remove_instance(instance_id)
  local instance = self._instances[instance_id]

  if not instance then
    return
  end

  for _, callback in ipairs(self._instance_remove_callbacks) do
    callback(instance_id)
  end

  self._instances[instance_id] = nil

  for area_id in pairs(instance.areas) do
    for _, callback in ipairs(self._area_remove_callbacks) do
      callback(area_id)
    end

    Net.remove_area(area_id)
  end
end

---Adds :remove_instance() listener, called before the instance is dropped and areas are removed.
---@param callback fun(instance_id: string)
function Instancer:on_instance_removed(callback)
  self._instance_remove_callbacks[#self._instance_remove_callbacks + 1] = callback
end

---@param instance_id string
---@param template_area_id string
function Instancer:is_template_area_instanced(instance_id, template_area_id)
  local instance = self._instances[instance_id]

  if not instance then
    return false
  end

  local area_id = template_area_id .. self.INSTANCE_MARKER .. instance_id
  return instance.areas[area_id] ~= nil
end

---Clones an area to the instance, does not clone bots.
---@param instance_id string
---@param template_area_id string
function Instancer:clone_area_to_instance(instance_id, template_area_id)
  local instance = self._instances[instance_id]

  if not instance then
    return
  end

  local new_area_id = template_area_id .. self.INSTANCE_MARKER .. instance_id
  Net.clone_area(template_area_id, new_area_id)
  instance.areas[new_area_id] = true

  for _, callback in ipairs(self._area_instanced_callbacks) do
    callback(new_area_id)
  end

  return new_area_id
end

---Adds a listener for instanced areas, called after the area is cloned.
---@param callback fun(area_id: string)
function Instancer:on_area_instanced(callback)
  self._area_instanced_callbacks[#self._area_instanced_callbacks + 1] = callback
end

---Removes the area from the server and instancer. Removes bots left in the area after listeners are notified.
---
---Returns true if the area was successfully removed, false if the area is not tied to an instance.
---@param area_id string
function Instancer:remove_area(area_id)
  -- handle the area getting removed while the instance is alive
  local instance_id = self:resolve_instance_id(area_id)
  local instance = self._instances[instance_id]

  if not instance or not instance.areas[area_id] then
    return false
  end

  instance.areas[area_id] = nil

  for _, callback in ipairs(self._area_remove_callbacks) do
    callback(area_id)
  end

  for _, bot_id in ipairs(Net.list_bots(area_id)) do
    Net.remove_bot(bot_id)
  end

  Net.remove_area(area_id)

  return true
end

---Adds a :remove_area() listener, called before the area is removed from the server.
---@param callback fun(area_id: string)
function Instancer:on_area_remove(callback)
  self._area_remove_callbacks[#self._area_remove_callbacks + 1] = callback
end

function Instancer:destroy()
  for _, event_name in ipairs(TRANSFER_EVENTS) do
    for _, listener in ipairs(self._transfer_listeners) do
      Net:remove_listener(event_name, listener)
    end
  end

  for _, instance in pairs(self._instances) do
    for area_id in pairs(instance.areas) do
      for _, callback in ipairs(self._area_remove_callbacks) do
        callback(area_id)
      end

      Net.remove_area(area_id)
    end
  end
end

return Instancer
