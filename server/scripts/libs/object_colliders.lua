---@class ObjectColliders._MappedList<K, V>: {
---  map: table<`K`, number>,
---  list: `V`[],
---  insert: fun(self, key: `K`, value: `V`),
---  clear: fun(),
---  get: (fun(self, key: `K`): `V`),
---  swap_remove: (fun(self, key: `K`, callback: fun(swapped: `V`): `K`): `V`?),
---}

local MappedList = {}

---@generic K
---@generic V
---@return ObjectColliders._MappedList<`K`, `V`>
function MappedList:new()
  local m = { map = {}, list = {} }
  setmetatable(m, self)
  self.__index = self
  return m
end

function MappedList:insert(key, value)
  local i = self.map[key] or (#self.list + 1)
  self.map[key] = i
  self.list[i] = value
end

function MappedList:get(key)
  local i = self.map[key]
  return i and self.list[i]
end

function MappedList:clear()
  for key, i in pairs(self.map) do
    self.list[i] = nil
    self.map[key] = nil
  end
end

function MappedList:swap_remove(key, callback)
  local i = self.map[key]

  if not i then
    return
  end

  local value = self.list[i]
  local swapped_value = self.list[#self.list]

  if swapped_value then
    self.map[callback(swapped_value)] = i
    self.list[i] = swapped_value
  end

  self.list[#self.list] = nil
  self.map[key] = nil

  return value
end

---@class ObjectColliders.Collider
---@field object_id number
---@field area_id string
---@field original_area_id string?
---@field original_object_id number?
---@field offset_x number?
---@field offset_y number?
---@field z number
---@field height number
---@field on_enter fun(player_id: string)
---@field on_exit fun(player_id: string)
---@field ignore_transfer boolean?
---@field moved boolean?
---@field associated_actor Net.ActorId?

---@class ObjectColliders
---@field package _collider_origin_map table<string, table<number, ObjectColliders.Collider>>
---@field package _still_collider_map table<string, ObjectColliders._MappedList<number, ObjectColliders.Collider>>
---@field package _moved_collider_map table<string, ObjectColliders._MappedList<number, ObjectColliders.Collider>>
---@field package _actors ObjectColliders._MappedList<Net.ActorId, { id: Net.ActorId, area_id: string, x: number, y: number, z: number, colliders: ObjectColliders.Collider[] }>
---@field package _player_tracking ObjectColliders._MappedList<Net.ActorId, {
---  player_id: Net.ActorId,
---  area_id: string,
---  prev_area_id: string?,
---  moved: boolean,
---  x: number,
---  y: number,
---  z: number,
---  collisions: table<number>,
---  prev_collisions: table<number>,
---}>
---@field package _listeners table
local ObjectColliders = {}

---@param collider ObjectColliders.Collider
---@param collider_map table<string, ObjectColliders._MappedList<number, ObjectColliders.Collider>>
local function add_to_collider_map(collider, collider_map)
  local colliders = collider_map[collider.area_id]

  if not colliders then
    colliders = MappedList:new()
    collider_map[collider.area_id] = colliders
  end

  colliders:insert(collider.object_id, collider)
end

---@param collider ObjectColliders.Collider
---@param collider_map table<string, ObjectColliders._MappedList<number, ObjectColliders.Collider>>
---@return ObjectColliders.Collider?
local function remove_from_collider_map(collider, collider_map)
  local colliders = collider_map[collider.area_id]

  if colliders then
    return colliders:swap_remove(collider.object_id, function(other)
      return other.object_id
    end)
  end
end

---@param self ObjectColliders
---@param collider ObjectColliders.Collider
local function mark_collider_moved(self, collider)
  add_to_collider_map(collider, self._moved_collider_map)
  remove_from_collider_map(collider, self._still_collider_map)
end

---@param self ObjectColliders
---@param collider ObjectColliders.Collider
local function mark_collider_still(self, collider)
  add_to_collider_map(collider, self._still_collider_map)
  remove_from_collider_map(collider, self._moved_collider_map)
end

---@param self ObjectColliders
---@param collider ObjectColliders.Collider
local function move_collider(self, collider, x, y, z)
  collider.z = z
  Net.move_object(collider.area_id, collider.object_id, x + collider.offset_x, y + collider.offset_y, z)
  mark_collider_moved(self, collider)
end

---@param self ObjectColliders
---@param collider ObjectColliders.Collider
local function transfer_collider(self, collider, area_id, x, y, z)
  remove_from_collider_map(collider, self._still_collider_map)
  remove_from_collider_map(collider, self._moved_collider_map)

  local object = Net.get_object_by_id(collider.area_id, collider.object_id)
  Net.remove_object(collider.area_id, collider.object_id)
  object.id = nil
  object.x = x + collider.offset_x
  object.y = y + collider.offset_y
  object.z = z

  collider.object_id = Net.create_object(area_id, object --[[@as Net.ObjectOptions]])
  collider.area_id = area_id

  add_to_collider_map(collider, self._moved_collider_map)
end

---@param colliders ObjectColliders._MappedList<number, ObjectColliders.Collider>?
local function for_each_enter_collision(data, colliders, callback)
  if not colliders then
    return
  end

  for _, collider in ipairs(colliders.list) do
    local collision = data.z >= collider.z
        and data.z <= collider.z + collider.height
        and collider.associated_actor ~= data.player_id
        and Net.is_inside_object(data.area_id, collider.object_id, data.x, data.y)

    if collision then
      data.collisions[collider.object_id] = collider

      if not data.prev_collisions[collider.object_id] then
        callback(collider)
      end
    end
  end
end

local function clear_each_prev_collision(data, callback)
  for id, collider in pairs(data.prev_collisions) do
    data.prev_collisions[id] = nil
    callback(id, collider)
  end
end

---@param self ObjectColliders
local function tick(self)
  for _, actor in ipairs(self._actors.list) do
    local area_id
    local x, y, z

    if Net.is_bot(actor.id) then
      area_id = Net.get_bot_area(actor.id)
      x, y, z = Net.get_bot_position_multi(actor.id)
    elseif Net.is_player(actor.id) then
      area_id = Net.get_player_area(actor.id)
      x, y, z = Net.get_player_position_multi(actor.id)
    else
      goto continue
    end

    local transferred = area_id ~= actor.area_id
    local moved = transferred or actor.x ~= x or actor.y ~= y or actor.z ~= actor.z
    actor.x = x
    actor.y = y
    actor.z = z

    if transferred then
      for _, collider in ipairs(actor.colliders) do
        transfer_collider(self, collider, area_id, x, y, z)
      end
    elseif moved then
      for _, collider in ipairs(actor.colliders) do
        move_collider(self, collider, x, y, z)
      end
    else
      for _, collider in ipairs(actor.colliders) do
        mark_collider_still(self, collider)
      end
    end

    ::continue::
  end

  for _, data in ipairs(self._player_tracking.list) do
    data.collisions, data.prev_collisions = data.prev_collisions, data.collisions

    local prev_area_id = data.prev_area_id

    if prev_area_id ~= data.area_id then
      clear_each_prev_collision(data, function(_, collider)
        if not collider.ignore_transfer and collider.on_exit then
          collider.on_exit(data.player_id)
        end
      end)

      data.prev_area_id = data.area_id
    end

    local moved_colliders = self._moved_collider_map[data.area_id]
    local still_colliders = self._still_collider_map[data.area_id]

    local collision_enter_callback = function(collider)
      if (not collider.ignore_transfer or data.area_id == prev_area_id) and collider.on_enter then
        collider.on_enter(data.player_id)
      end
    end

    if data.moved then
      for_each_enter_collision(data, still_colliders, collision_enter_callback)
      for_each_enter_collision(data, moved_colliders, collision_enter_callback)
    else
      for_each_enter_collision(data, moved_colliders, collision_enter_callback)

      -- retain collisions against still colliders
      if still_colliders then
        for _, collider in ipairs(still_colliders.list) do
          if data.prev_collisions[collider.object_id] then
            data.collisions[collider.object_id] = collider
          end
        end
      end
    end


    clear_each_prev_collision(data, function(id, collider)
      if not data.collisions[id] and collider.on_exit then
        collider.on_exit(data.player_id)
      end
    end)

    data.moved = false
  end
end

---@return ObjectColliders
function ObjectColliders:new()
  local object_colliders = {
    _collider_origin_map = {},
    _still_collider_map = {},
    _moved_collider_map = {},
    _actors = MappedList:new(),
    _player_tracking = MappedList:new(),
  }
  setmetatable(object_colliders, self)
  self.__index = self

  local player_tracking = object_colliders._player_tracking

  local function init_player(player_id)
    local x, y, z = Net.get_player_position_multi(player_id)
    player_tracking:insert(player_id, {
      player_id = player_id,
      x = x,
      y = y,
      z = z,
      area_id = Net.get_player_area(player_id),
      collisions = {},
      prev_collisions = {}
    })
  end

  for _, area_id in ipairs(Net.list_areas()) do
    for _, player_id in ipairs(Net.list_players(area_id)) do
      init_player(player_id)
    end
  end

  object_colliders._listeners = {
    player_connect = function(event)
      init_player(event.player_id)
    end,

    player_move = function(event)
      local data = player_tracking:get(event.player_id)
      data.moved = data.x ~= event.x or data.y ~= event.y or data.z ~= event.z
      data.x = event.x
      data.y = event.y
      data.z = event.z
    end,

    player_area_transfer = function(event)
      local data = player_tracking:get(event.player_id)
      data.prev_area_id = data.area_id
      data.area_id = Net.get_player_area(event.player_id)
      data.x, data.y, data.z = Net.get_player_position_multi(event.player_id)
      data.moved = true
    end,

    tick = function()
      tick(object_colliders)
    end,

    player_disconnect = function(event)
      player_tracking[event.player_id] = nil
    end,
  }

  for key, value in pairs(object_colliders._listeners) do
    Net:on(key, value)
  end

  return object_colliders
end

---@class ObjectColliders.ColliderOptions
---@field object_id number
---@field area_id string
---@field z number
---@field height number?
---@field on_enter? fun(player_id: Net.ActorId)
---@field on_exit? fun(player_id: Net.ActorId)
---@field ignore_transfer boolean?

---Registers an object as a collider
---@param options ObjectColliders.ColliderOptions
function ObjectColliders:register_collider(options)
  local collider = {
    object_id = options.object_id,
    area_id = options.area_id,
    z = options.z,
    height = options.height or 0,
    on_enter = options.on_enter,
    on_exit = options.on_exit,
    ignore_transfer = options.ignore_transfer,
  }

  add_to_collider_map(collider, self._still_collider_map)

  local area_origin_map = self._collider_origin_map[options.area_id]

  if not area_origin_map then
    area_origin_map = {}
    self._collider_origin_map[options.area_id] = area_origin_map
  end

  area_origin_map[options.object_id] = collider
end

---@param self ObjectColliders
---@param area_id string
---@param object_id string|number
---@return ObjectColliders.Collider?
local function get_collider(self, area_id, object_id)
  local area_origin_map = self._collider_origin_map[area_id]

  if not area_origin_map then
    return
  end

  return area_origin_map[tonumber(object_id)]
end

---Moves the collider with the actor every tick.
---@param area_id string
---@param object_id string|number
---@param actor_id Net.ActorId
---@param offset_x number
---@param offset_y number
function ObjectColliders:attach_to_actor(area_id, object_id, actor_id, offset_x, offset_y)
  local collider = get_collider(self, area_id, object_id)

  if not collider then
    return
  end

  if collider.associated_actor then
    self:detach_from_actor(area_id, object_id, collider.associated_actor)
  else
    collider.original_area_id = area_id
    collider.original_object_id = tonumber(object_id)
  end

  collider.offset_x = offset_x
  collider.offset_y = offset_y

  local actor = self._actors:get(actor_id)
  local actor_area_id
  local x, y, z

  if Net.is_bot(actor_id) then
    actor_area_id = Net.get_bot_area(actor_id)
    x, y, z = Net.get_bot_position_multi(actor_id)

    if actor_area_id ~= area_id then
      transfer_collider(self, collider, actor_area_id, x, y, z)
    else
      move_collider(self, collider, x, y, z)
    end
  elseif Net.is_player(actor_id) then
    actor_area_id = Net.get_player_area(actor_id)
    x, y, z = Net.get_player_position_multi(actor_id)
  end

  if actor_area_id then
    local object = Net.get_object_by_id(collider.area_id, collider.object_id)

    if actor_area_id ~= area_id then
      transfer_collider(self, collider, actor_area_id, x, y, z)
    elseif x ~= object.x + offset_x or y ~= object.y + offset_y or object.z ~= z then
      move_collider(self, collider, x, y, z)
    end
  end

  if not actor then
    self._actors:insert(actor_id, {
      id = actor_id,
      area_id = area_id,
      x = x,
      y = y,
      z = z,
      colliders = { collider }
    })
  else
    actor.colliders[#actor.colliders + 1] = collider
  end
end

---Removes movement sync for the collider on this actor.
---@param area_id string
---@param object_id string|number
---@param actor_id Net.ActorId
function ObjectColliders:detach_from_actor(area_id, object_id, actor_id)
  local actor = self._actors:get(actor_id)

  if not actor then
    return
  end

  local object_id_number = tonumber(object_id)

  for i, collider in ipairs(actor.colliders) do
    if collider.original_area_id == area_id and collider.original_object_id == object_id_number then
      actor.colliders[i] = actor.colliders[#actor.colliders]
      actor.colliders[#actor.colliders] = nil
      collider.associated_actor = nil
      mark_collider_still(self, collider)
      break
    end
  end
end

---Removes movement sync for colliders on this actor.
---@param actor_id Net.ActorId
function ObjectColliders:detach_all_from_actor(actor_id)
  local actor = self._actors:get(actor_id)

  if not actor then
    return
  end

  for i, collider in ipairs(actor.colliders) do
    actor.colliders[i] = nil
    collider.associated_actor = nil
    mark_collider_still(self, collider)
  end
end

---Calls :drop_collider() and Net.remove_object() on all colliders attached to this actor.
---@param actor_id Net.ActorId
function ObjectColliders:remove_all_on_actor(actor_id)
  local actor = self._actors:get(actor_id)

  if not actor then
    return
  end

  for _, collider in ipairs(actor.colliders) do
    collider.associated_actor = nil
    self:drop_collider(collider.original_area_id, collider.original_object_id)
    Net.remove_object(collider.area_id, collider.object_id)
  end
end

---Removes tracking for all colliders attached to this actor.
---@param actor_id Net.ActorId
function ObjectColliders:drop_all_on_actor(actor_id)
  local actor = self._actors:get(actor_id)

  if not actor then
    return
  end

  for _, collider in ipairs(actor.colliders) do
    collider.associated_actor = nil
    self:drop_collider(collider.original_area_id, collider.original_object_id)
  end
end

---Removes tracking for this actor and detaches all colliders.
---@param actor_id Net.ActorId
function ObjectColliders:drop_actor(actor_id)
  local actor = self._actors:swap_remove(actor_id, function(actor)
    return actor.id
  end)

  if not actor then
    return
  end

  for _, collider in ipairs(actor.colliders) do
    collider.associated_actor = nil
    mark_collider_still(self, collider)
  end
end

---Removes the object from it's current area and drops tracking for it.
---@param area_id string
---@param object_id string|number
function ObjectColliders:remove_collider(area_id, object_id)
  local collider = get_collider(self, area_id, object_id)

  if not collider then
    return
  end

  self:drop_collider(area_id, object_id)
  Net.remove_object(collider.area_id, collider.object_id)
end

---Removes tracking for this object.
---@param area_id string
---@param object_id string|number
function ObjectColliders:drop_collider(area_id, object_id)
  local area_origin_map = self._collider_origin_map[area_id]

  if not area_origin_map then
    return
  end

  local object_id_number = tonumber(object_id)
  local collider = area_origin_map[object_id_number]

  if not collider or not object_id_number then
    return
  end

  if collider.associated_actor then
    self:detach_from_actor(area_id, object_id_number, collider.associated_actor)
  end

  area_origin_map[object_id_number] = nil

  remove_from_collider_map(collider, self._still_collider_map)
  remove_from_collider_map(collider, self._moved_collider_map)
end

---Removes tracking for all colliders in, or originating in, this area.
---@param area_id string
function ObjectColliders:drop_colliders_for_area(area_id)
  local area_origin_map = self._collider_origin_map[area_id]

  if area_origin_map then
    for object_id in pairs(area_origin_map) do
      self:drop_collider(area_id, object_id)
    end

    self._collider_origin_map[area_id] = nil
  end

  self._still_collider_map[area_id] = nil
  self._moved_collider_map[area_id] = nil
end

---Removes server event listeners.
function ObjectColliders:destroy()
  for key, value in pairs(self._listeners) do
    Net:remove_listener(key, value)
  end
end

return ObjectColliders
