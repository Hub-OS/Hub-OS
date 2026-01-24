local TRANSFER_EVENTS = {
  "player_connect",
  "player_join",
  "player_area_transfer",
  "player_disconnect",
}

---Used to create and track instances, which contains isolated copies of areas.
---@class Instancer
---@field private _instances table<string, { areas: table<string>, player_list: Net.ActorId[] }>
---@field private _events Net.EventEmitter
---@field private _transfer_listeners fun()[]
local Instancer = {
  INSTANCE_MARKER = ";instance:",
}

function Instancer:new()
  ---@type table<string, { areas: table<string>, player_list: Net.ActorId[] }>
  local instances = {}
  local events = Net.EventEmitter.new()
  local transfer_listeners = {}

  local instancer = {
    _instances = instances,
    _events = events,
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

        events:emit("player_leave", {
          player_id = event.player_id,
          area_id = previous_area
        })
      end
    end

    -- add player to instance
    if current_instance_id then
      local instance = instances[current_instance_id]

      if instance then
        instance.player_list[#instance.player_list + 1] = event.player_id

        events:emit("player_join", {
          player_id = event.player_id,
          area_id = current_area,
          previous_area_id = previous_area,
        })
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

---Events:
--- - "instance_created", { instance_id }
--- - "area_instanced", { area_id }
--- - "area_removed", { area_id }
---   - Called for each area before "instance_removed" and before deleting the area
--- - "instance_removed", { instance_id }
--- - "player_join", { player_id, area_id }
---   - Called when a player joins an instance
--- - "player_leave", { player_id, area_id, prev_area_id? }
---   - Called when a player leaves an instance
function Instancer:events()
  return self._events
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

---@class Instancer.InstanceOptions
---@field auto_remove? boolean false by default, tracking may be innaccurate, areas may stay until restart if a player never joins.

---Used to create an instance, which contains isolated copies of areas.
---@param options Instancer.InstanceOptions?
function Instancer:create_instance(options)
  local instance_id = tostring(Net.system_random())
  self._instances[instance_id] = {
    areas = {},
    player_list = {}
  }

  self._events:emit("instance_created", { instance_id = instance_id })

  if options and options.auto_remove then
    local function cleanup()
      local instance = self._instances[instance_id]

      if not instance or #instance.player_list > 0 then
        return
      end

      self._events:remove_listener("player_leave", cleanup)
      self:remove_instance(instance_id)
    end

    self._events:on("player_leave", cleanup)
  end

  return instance_id
end

---Calls :remove_area() for every area in the instance.
---@param instance_id string
function Instancer:remove_instance(instance_id)
  local instance = self._instances[instance_id]

  if not instance then
    return
  end

  for area_id in pairs(instance.areas) do
    self._events:emit("area_removed", { area_id = area_id })
    Net.remove_area(area_id)
  end

  self._events:emit("instance_removed", { instance_id = instance_id })
  self._instances[instance_id] = nil
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

  self._events:emit("area_instanced", { area_id = new_area_id })

  return new_area_id
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

  self._events:emit("area_removed", { area_id = area_id })

  for _, bot_id in ipairs(Net.list_bots(area_id)) do
    Net.remove_bot(bot_id)
  end

  Net.remove_area(area_id)

  return true
end

function Instancer:destroy()
  for _, event_name in ipairs(TRANSFER_EVENTS) do
    for _, listener in ipairs(self._transfer_listeners) do
      Net:remove_listener(event_name, listener)
    end
  end

  for _, instance in pairs(self._instances) do
    for area_id in pairs(instance.areas) do
      self._events:emit("area_removed", { area_id = area_id })
      Net.remove_area(area_id)
    end
  end
end

return Instancer
