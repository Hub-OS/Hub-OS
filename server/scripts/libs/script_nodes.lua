-- todo:
-- `Attach Sprite`
-- - `Id`
-- - `Target` "Player [1+]" "Bot [id]" "Widget" "Hud"
-- - `Private` boolean, displays only to players in the current context
-- `Remove Sprite`
-- - `Id`
-- - `Target`

local ScriptNodeVariables = require("scripts/libs/script_node_variables")
local Instancer = require("scripts/libs/instancer")
local BotPaths = require("scripts/libs/bot_paths")
local ObjectColliders = require("scripts/libs/object_colliders")
local Direction = require("scripts/libs/direction")

local function clone_table(t)
  local clone = {}

  for i, v in ipairs(t) do
    clone[i] = v
  end

  for k, v in pairs(t) do
    clone[k] = v
  end

  return clone
end

---@param s string
---@param value string
local function starts_with(s, value)
  return s:sub(1, #value) == value
end

local function parse_color(color_string)
  if not color_string or color_string == "" then
    return
  end

  return {
    a = tonumber(color_string:sub(2, 3), 16),
    r = tonumber(color_string:sub(4, 5), 16),
    g = tonumber(color_string:sub(6, 7), 16),
    b = tonumber(color_string:sub(8, 9), 16),
  }
end

local function for_each_player(context, callback)
  if context.player_ids then
    for _, player_id in ipairs(context.player_ids) do
      callback(player_id)
    end
  else
    callback(context.player_id)
  end
end

local function for_each_player_safe(context, callback)
  if context.player_ids then
    for _, player_id in ipairs(context.player_ids) do
      if Net.is_player(player_id) then
        callback(player_id)
      end
    end
  elseif Net.is_player(context.player_id) then
    callback(context.player_id)
  end
end

---An interpreter using map objects as script nodes.
---
---A script node is an object with Type: `Script Node: [node type]`.
---An entry node is an object with Type: `Script Entry: [entry type]`
---
---Call :load() on every area_id that scripts should be enabled on.
---
---Call :execute_by_id() or :execute_node() to execute or continue a script.
---@class ScriptNodes
---@field private _instancer Instancer
---@field private _variables ScriptNodeVariables
---@field private _node_types table<string, fun(context: table, object: Net.Object)>
---@field private _bot_script_ids table<string, table<string, Net.ActorId>>
---@field private _bot_script_ids_reversed table<Net.ActorId, [string, string]>
---@field private _tagged table<string, Net.ActorId[]>
---@field private _cached_object_map table<string, table<number, Net.Object>>
---@field private _loaded_areas table<string, boolean>
---@field private _load_start_callbacks fun(area_id: string)[]
---@field private _load_callbacks fun(area_id: string)[]
---@field private _entry_load_callbacks table<string, fun(area_id: string, object: Net.Object)[]>
---@field private _script_load_callbacks table<string, fun(area_id: string, object: Net.Object)[]>
---@field private _unload_callbacks fun(area_id: string)[]
---@field private _inventory_callbacks fun(player_id: Net.ActorId, item_id: string)[]
---@field private _money_callbacks fun(player_id: Net.ActorId)[]
---@field private _encounter_callbacks fun(results: Net.BattleResults)[]
---@field private _bot_remove_callbacks fun(results: Net.ActorId)[]
---@field private _server_listeners [string, fun()][]
---@field private _destroy_callbacks fun()[]
local ScriptNodes = {
  NODE_TYPE_PREFIX = "Script Node: ",
  ENTRY_TYPE_PREFIX = "Script Entry: ",
  ASSET_PREFIX = "/server/assets/",
}

---Creates a new script interpreter with no default api.
---@return ScriptNodes
function ScriptNodes:new_empty()
  local instancer = Instancer:new()
  local variables = ScriptNodeVariables:new()

  local s = {
    _instancer = instancer,
    _variables = variables,
    _node_types = {},
    _bot_script_ids = {},
    _bot_script_ids_reversed = {},
    _tagged = {},
    _cached_object_map = {},
    _loaded_areas = {},
    _load_start_callbacks = {},
    _load_callbacks = {},
    _entry_load_callbacks = {},
    _script_load_callbacks = {},
    _unload_callbacks = {},
    _inventory_callbacks = {},
    _money_callbacks = {},
    _encounter_callbacks = {},
    _bot_remove_callbacks = {},
    _server_listeners = {},
    _destroy_callbacks = {}
  }
  setmetatable(s, self)
  self.__index = self

  s:on_unload(function(area_id)
    variables:clear_area(area_id)
  end)

  instancer:on_instance_removed(function(instance_id)
    variables:clear_instance(instance_id)
  end)

  instancer:on_area_remove(function(area_id)
    for _, bot_id in ipairs(Net.list_bots(area_id)) do
      s:emit_bot_remove(bot_id)
      Net.remove_bot(bot_id)
    end

    s:unload(area_id)
  end)

  return s
end

---Creates a new script interpreter with all built in apis.
---@return ScriptNodes
function ScriptNodes:new()
  local s = self:new_empty()

  s:implement_event_entry_api()
  s:implement_area_api()
  s:implement_object_api()
  s:implement_tile_api()
  s:implement_textbox_api()
  s:implement_bbs_api()
  s:implement_shop_api()
  s:implement_sound_api()
  s:implement_camera_api()
  s:implement_encounter_api()
  s:implement_inventory_api()
  s:implement_link_api()
  s:implement_actor_api()
  s:implement_tag_api()
  s:implement_common_animations_api()
  s:implement_path_api()
  s:implement_collision_api()
  s:implement_delay_api()
  s:implement_random_api()
  s:implement_thread_api()
  s:implement_party_api()
  s:implement_variable_api()
  s:implement_debug_api()

  return s
end

function ScriptNodes:instancer()
  return self._instancer
end

function ScriptNodes:variables()
  return self._variables
end

---Adds a :destroy() listener, used to clean up Net:on() event listeners.
function ScriptNodes:on_destroy(callback)
  self._destroy_callbacks[#self._destroy_callbacks + 1] = callback
end

function ScriptNodes:destroy()
  self._instancer:destroy()

  for _, pair in ipairs(self._server_listeners) do
    Net:remove_listener(pair[1], pair[2])
  end

  for _, callback in ipairs(self._destroy_callbacks) do
    callback()
  end

  self._variables:destroy()
end

---Calls Net:on(event_name, callback), automatically cleans up the listener when :destroy() is called.
---@param event_name string
---@param callback fun(...: any)
function ScriptNodes:on_server_event(event_name, callback)
  Net:on(event_name, callback)
  self._server_listeners[#self._server_listeners + 1] = { event_name, callback }
end

---Adds a :load() listener, called before nodes are loaded.
---@param callback fun(area_id: string)
function ScriptNodes:on_load_start(callback)
  self._load_start_callbacks[#self._load_start_callbacks + 1] = callback
end

---Adds a :load() listener, called after script nodes are loaded, but before entry nodes are loaded.
---@param callback fun(area_id: string)
function ScriptNodes:on_load(callback)
  self._load_callbacks[#self._load_callbacks + 1] = callback
end

---Adds a listener to detect script nodes found during :load().
---@param node_type string
---@param callback fun(area_id: string, object: Net.Object)
function ScriptNodes:on_script_node_load(node_type, callback)
  local type = node_type:lower()
  local callbacks = self._script_load_callbacks[type]

  if callbacks then
    callback[#callback + 1] = callback
  else
    self._script_load_callbacks[type] = { callback }
  end
end

---Adds a listener to detect entry nodes found during :load().
---@param node_type string
---@param callback fun(area_id: string, object: Net.Object)
function ScriptNodes:on_entry_node_load(node_type, callback)
  local type = node_type:lower()
  local callbacks = self._entry_load_callbacks[type]

  if callbacks then
    callback[#callback + 1] = callback
  else
    self._entry_load_callbacks[type] = { callback }
  end
end

---Adds an :unload() listener, used to clean up data before removing an area.
---@param callback fun(area_id: string)
function ScriptNodes:on_unload(callback)
  self._unload_callbacks[#self._unload_callbacks + 1] = callback
end

---@param area_id string
function ScriptNodes:is_loaded(area_id)
  return self._loaded_areas[area_id] == true
end

---@param callback_map table<string, fun(area_id: string, object: Net.Object)[]>
local function call_node_load_callbacks(area_id, object, prefix, callback_map)
  local entry_type = object.type:sub(#prefix + 1):lower()
  local callbacks = callback_map[entry_type]

  if callbacks then
    for _, callback in ipairs(callbacks) do
      callback(area_id, object)
    end
  end
end

---Used to begin processing new areas.
---Calls :cache_object() and `Net.set_object_privacy()` for detected script and entry nodes.
---@param area_id string
function ScriptNodes:load(area_id)
  self._loaded_areas[area_id] = true
  local template_area_id = self._instancer:resolve_base_area_id(area_id)

  if template_area_id then
    local cached_objects = self._cached_object_map[template_area_id]

    if cached_objects then
      self._cached_object_map[area_id] = cached_objects

      for _, callback in ipairs(self._load_start_callbacks) do
        callback(area_id)
      end

      for _, object in pairs(cached_objects) do
        if self:is_script_node(object) then
          call_node_load_callbacks(area_id, object, self.NODE_TYPE_PREFIX, self._script_load_callbacks)
        elseif self:is_entry_node(object) then
          call_node_load_callbacks(area_id, object, self.ENTRY_TYPE_PREFIX, self._entry_load_callbacks)
        end
      end

      for _, callback in ipairs(self._load_callbacks) do
        callback(area_id)
      end

      return
    end
  end

  local cached_objects = {}
  self._cached_object_map[area_id] = cached_objects

  for _, callback in ipairs(self._load_start_callbacks) do
    callback(area_id)
  end

  for _, object_id in ipairs(Net.list_objects(area_id)) do
    local object = self:resolve_object(area_id, object_id)

    if self:is_script_node(object) then
      self:cache_object(area_id, object)
      Net.set_object_privacy(area_id, object.id, true)
      call_node_load_callbacks(area_id, object, self.NODE_TYPE_PREFIX, self._script_load_callbacks)
    elseif self:is_entry_node(object) then
      self:cache_object(area_id, object)
      Net.set_object_privacy(area_id, object.id, true)
    end
  end

  for _, callback in ipairs(self._load_callbacks) do
    callback(area_id)
  end

  for _, object in pairs(cached_objects) do
    if self:is_entry_node(object) then
      call_node_load_callbacks(area_id, object, self.ENTRY_TYPE_PREFIX, self._entry_load_callbacks)
    end
  end
end

---Used to perform cleanup before removing an area.
---Clears cached objects for the area.
---@param area_id string
function ScriptNodes:unload(area_id)
  self._loaded_areas[area_id] = nil

  for _, callback in ipairs(self._unload_callbacks) do
    callback(area_id)
  end

  self._cached_object_map[area_id] = nil
  self._bot_script_ids[area_id] = nil
end

---Adds a listener for inventory changes.
---@param callback fun(player_id: Net.ActorId, item_id: string?)
function ScriptNodes:on_inventory_update(callback)
  self._inventory_callbacks[#self._inventory_callbacks + 1] = callback
end

---Emit an inventory update for listeners.
---@param player_id Net.ActorId
---@param item_id string
function ScriptNodes:emit_inventory_update(player_id, item_id)
  for _, callback in ipairs(self._inventory_callbacks) do
    callback(player_id, item_id)
  end
end

---Adds a listener for money changes.
---@param callback fun(player_id: Net.ActorId, item_id: string?)
function ScriptNodes:on_money_update(callback)
  self._money_callbacks[#self._money_callbacks + 1] = callback
end

---Emit a money update for listeners.
---@param player_id Net.ActorId
function ScriptNodes:emit_money_update(player_id)
  for _, callback in ipairs(self._money_callbacks) do
    callback(player_id)
  end
end

---Adds a listener for encounter results.
---@param callback fun(result: Net.BattleResults)
function ScriptNodes:on_encounter_result(callback)
  self._encounter_callbacks[#self._encounter_callbacks + 1] = callback
end

---Emit an encounter result for listeners.
---@param result Net.BattleResults
function ScriptNodes:emit_encounter_result(result)
  for _, callback in ipairs(self._encounter_callbacks) do
    callback(result)
  end
end

---Adds a listener for bot removal, called before a bot is removed.
---@param callback fun(bot_id: Net.ActorId)
function ScriptNodes:on_bot_removed(callback)
  self._bot_remove_callbacks[#self._bot_remove_callbacks + 1] = callback
end

---Emit a bot removal event, call before removing a bot.
---@param bot_id Net.ActorId
function ScriptNodes:emit_bot_remove(bot_id)
  for _, callback in ipairs(self._bot_remove_callbacks) do
    callback(bot_id)
  end
end

---Used to create new script nodes
---@param node_type string
---@param callback fun(context: table, object: Net.Object)
function ScriptNodes:implement_node(node_type, callback)
  self._node_types[node_type:lower()] = callback
end

---@param object Net.Object
function ScriptNodes:is_script_node(object)
  return starts_with(object.type, self.NODE_TYPE_PREFIX)
end

---@param object Net.Object
function ScriptNodes:is_entry_node(object)
  return starts_with(object.type, self.ENTRY_TYPE_PREFIX)
end

---@param context table
---@param area_id string
---@param object_id string|number
function ScriptNodes:execute_by_id(context, area_id, object_id)
  local object = self:resolve_object(area_id, object_id)

  if not object then
    return
  end

  self:execute_node(context, object)
end

---@param context table
---@param object Net.Object
function ScriptNodes:execute_node(context, object)
  local node_type = object.type:sub(#self.NODE_TYPE_PREFIX + 1)
  local callback = self._node_types[node_type:lower()]

  if not callback then
    if self:is_script_node(object) then
      error('invalid script node: "' .. object.type .. '"')
    else
      error('"' .. object.type .. '" is not implemented')
    end
  else
    callback(context, object)
  end
end

---Resolves the id of the next script node. Initially checks `Next [n]`, and falls back to `Next`, then `Next 1`
---@param object Net.Object
---@param n number?
---@return string?
function ScriptNodes:resolve_next_id(object, n)
  n = n or 1

  return object.custom_properties["Next " .. n] or
      object.custom_properties["Next"] or
      object.custom_properties["Next 1"]
end

---Resolves the next script node. Initially checks `Next [n]`, and falls back to `Next`, then `Next 1`
---@param area_id string
---@param object Net.Object
---@param n number?
---@return Net.Object?
function ScriptNodes:resolve_next_node(area_id, object, n)
  local id = self:resolve_next_id(object, n)

  if id then
    return self:resolve_object(area_id, id)
  end
end

---Resolves the next script node and executes it. Initially checks `Next [n]`, and falls back to `Next`, then `Next 1`
---@param context table
---@param area_id string
---@param object Net.Object
---@param n number?
---@return Net.Object?
function ScriptNodes:execute_next_node(context, area_id, object, n)
  local next = self:resolve_next_node(area_id, object, n)

  if next then
    self:execute_node(context, next)
  end
end

---Caches objects to reduce generated garbage.
---
---Use :resolve_object() to access cached objects.
---@param area_id string
---@param object Net.Object
function ScriptNodes:cache_object(area_id, object)
  self._cached_object_map[area_id][object.id] = object
end

---@param area_id string
---@param object_id string|number
function ScriptNodes:is_object_cached(area_id, object_id)
  local map = self._cached_object_map[area_id]

  return (map and map[tonumber(object_id)]) ~= nil
end

---Resolves objects that may be cached by :cache_object()
---@param area_id string
---@param object_id number|string
function ScriptNodes:resolve_object(area_id, object_id)
  local object = Net.get_object_by_id(area_id, object_id)

  if object then
    return object
  end

  return self._cached_object_map[area_id][tonumber(object_id)]
end

---@param actor_id Net.ActorId
---@param tag string
function ScriptNodes:tag_actor(actor_id, tag)
  local tag_group = self:get_tag_actors(tag)
  tag_group[#tag_group + 1] = actor_id
end

---@param actor_id Net.ActorId
---@param tag string
function ScriptNodes:untag_actor(actor_id, tag)
  local tag_group = self:get_tag_actors(tag)

  for i, id in ipairs(tag_group) do
    if actor_id == id then
      tag_group[i] = tag_group[#i]
      tag_group[#i] = nil
      break
    end
  end
end

---@param tag string
function ScriptNodes:get_tag_actors(tag)
  local tag_group = self._tagged[tag]

  if not tag_group then
    tag_group = {}
    self._tagged[tag] = tag_group
  end

  return tag_group
end

---@param area_id string
---@param bot_id Net.ActorId
---@param bot_script_id string A human readable ID for the bot
function ScriptNodes:track_bot(area_id, bot_id, bot_script_id)
  local area_map = self._bot_script_ids[area_id]

  if not area_map then
    area_map = {}
    self._bot_script_ids[area_id] = area_map
  end

  area_map[bot_script_id] = bot_id
  self._bot_script_ids_reversed[bot_id] = { area_id, bot_script_id }
end

---@param bot_id Net.ActorId
function ScriptNodes:untrack_bot(bot_id)
  local area_id_script_id_pair = self._bot_script_ids_reversed[bot_id]

  if not area_id_script_id_pair then
    return
  end

  self._bot_script_ids_reversed[bot_id] = nil
  local area_id, bot_script_id = area_id_script_id_pair[1], area_id_script_id_pair[2]
  self._bot_script_ids[area_id][bot_script_id] = nil
end

---@param context table
---@param actor_id_property string?
---@return Net.ActorId?
function ScriptNodes:resolve_bot_id(context, actor_id_property)
  if not actor_id_property or not starts_with(actor_id_property, "Bot") then
    return
  end

  if actor_id_property == "Bot" then
    return context.bot_id
  end

  local bot_script_id = actor_id_property:sub(5)

  local area_map = self._bot_script_ids[context.area_id]

  if not area_map then
    return
  end

  return area_map[bot_script_id]
end

---@param context table
---@param actor_id_property string?
---@return Net.ActorId?
function ScriptNodes:resolve_player_id(context, actor_id_property)
  if not actor_id_property or not starts_with(actor_id_property, "Player") then
    return
  end

  local player_id = context.player_id

  if context.player_ids then
    -- resolve player index
    local n = tonumber(actor_id_property:sub(8)) or 1
    player_id = context.player_ids[n]
  end

  return player_id
end

---@param context table
---@param value string?
function ScriptNodes:resolve_actor_id(context, value)
  if not value then
    return
  end

  return self:resolve_bot_id(context, value) or self:resolve_player_id(context, value)
end

---@param object Net.Object
function ScriptNodes:resolve_mug(context, object)
  local mug_path = object.custom_properties["Mug"]

  if mug_path then
    local player_id = self:resolve_player_id(context, mug_path)

    if player_id then
      local mug = Net.get_player_mugshot(player_id)

      if mug then
        return mug.texture_path, mug.animation_path
      else
        return "", ""
      end
    end

    -- extensionless path
    return self.ASSET_PREFIX .. mug_path .. ".png",
        self.ASSET_PREFIX .. mug_path .. ".animation"
  end

  local texture = object.custom_properties["Mug Texture"]
  local animation = object.custom_properties["Mug Animation"]

  if texture then
    texture = self.ASSET_PREFIX .. texture
  else
    texture = ""
  end

  if animation then
    animation = self.ASSET_PREFIX .. animation
  else
    animation = ""
  end

  return texture, animation
end

---@param target_area_id string
---@param object Net.Object
function ScriptNodes:resolve_teleport_properties(object, target_area_id)
  local warp_in
  local x, y, z
  local direction

  if object.custom_properties.Target then
    local target_object = self:resolve_object(target_area_id, object.custom_properties.Target)

    if target_object then
      -- adopt from target object
      x, y, z = target_object.x, target_object.y, target_object.z
      warp_in = target_object.custom_properties.Warp == "true"
      direction = target_object.custom_properties.Direction
    end
  end

  -- adopt from node object (preferred)
  if object.custom_properties.Warp then
    warp_in = object.custom_properties.Warp == "true"
  end

  if object.custom_properties.Direction then
    direction = object.custom_properties.Direction
  end

  return warp_in, x, y, z, direction
end

---Implements support for the `Load`, `Server Event`, and `Player Interaction` entry types
---and the `On Load` property on all script nodes.
---
---When :load() is called on an area, any script nodes with `On Load` set to true will execute using a context containing `area_id`.
---
---As a reminder: entry nodes have a type starting with `Script Entry: ` such as `Script Entry: Load`
---
---Custom properties supported by `Load`:
--- - `Next [1]` a link to the next node (optional)
---
---Supplies a context with `area_id`.
---
---Custom properties supported by `Server Event`:
--- - `Event` string, an event name (optional)
--- - `Events` string, a list of event names separated by a comma and space ", " (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Supplies a context with `area_id`, `player_id`, `object_id`, `x`, `y`, `z` when possible.
---
---Custom properties supported by `Player Interaction`:
--- - `Next [1]` a link to the next node (optional)
---
---Supplies a context with `player_ids` and `area_id`
---
---Custom properties supported by `Tile Interaction`:
--- - `Next [1]` a link to the next node (optional)
---
---Supplies a context with `area_id`, `player_id`, `x`, `y`, and `z`.
---
---Custom properties supported by `Help`:
--- - `Next [1]` a link to the next node (optional)
---
---Supplies a context with `player_id` and `area_id`
---
---Custom properties supported by `Item Use`:
--- - `Item` string, item id
--- - `Next [1]` a link to the next node (optional)
---
---Supplies a context with `player_id` and `area_id`
function ScriptNodes:implement_event_entry_api()
  ---table<event_name, table<area_id, object_id[]>>
  ---@type table<string, table<string, (string|number)[]>>
  local event_map = {}
  ---@type table<string, (string|number)[]>
  local player_interaction_event_map = {}
  ---@type table<string, (string|number)[]>
  local tile_interaction_event_map = {}
  ---@type table<string, (string|number)[]>
  local help_event_map = {}

  local function add_listening_node(event_name, area_id, object_id)
    if event_name == "" then
      return
    end

    local area_map = event_map[event_name]

    if not area_map then
      area_map = {}
      event_map[event_name] = area_map
    end

    local object_id_list = area_map[area_id]

    if not object_id_list then
      object_id_list = {}
      area_map[area_id] = object_id_list
    end

    object_id_list[#object_id_list + 1] = object_id
  end

  self:on_load(function(area_id)
    for _, object_id in ipairs(Net.list_objects(area_id)) do
      if not self:is_object_cached(area_id, object_id) then
        goto continue
      end

      local object = self:resolve_object(area_id, object_id)

      if self:is_script_node(object) and object.custom_properties["On Load"] == "true" then
        local context = { area_id = area_id }
        self:execute_node(context, object)
      end

      ::continue::
    end
  end)

  self:on_entry_node_load("load", function(area_id, object)
    local context = { area_id = area_id }
    self:execute_next_node(context, area_id, object)
  end)

  self:on_entry_node_load("server event", function(area_id, object)
    local next_id = self:resolve_next_id(object)

    if object.custom_properties.Event then
      add_listening_node(object.custom_properties.Event, area_id, next_id)
    end

    local events_string = object.custom_properties.Events

    if events_string then
      local start = 1

      while true do
        local match_start, match_end = events_string:find(", ", start)

        if match_start then
          local event_name = events_string:sub(start, match_start - 1)
          add_listening_node(event_name, area_id, next_id)

          start = match_end + 1
        else
          local event_name = events_string:sub(start)
          add_listening_node(event_name, area_id, next_id)
          break
        end
      end
    end
  end)

  self:on_entry_node_load("player interaction", function(area_id, object)
    local object_ids = player_interaction_event_map[area_id]

    if not object_ids then
      object_ids = {}
      player_interaction_event_map[area_id] = object_ids
    end

    object_ids[#object_ids + 1] = self:resolve_next_id(object)
  end)

  self:on_entry_node_load("tile interaction", function(area_id, object)
    local object_ids = tile_interaction_event_map[area_id]

    if not object_ids then
      object_ids = {}
      tile_interaction_event_map[area_id] = object_ids
    end

    object_ids[#object_ids + 1] = self:resolve_next_id(object)
  end)

  self:on_entry_node_load("help", function(area_id, object)
    local object_ids = help_event_map[area_id]

    if not object_ids then
      object_ids = {}
      help_event_map[area_id] = object_ids
    end

    object_ids[#object_ids + 1] = self:resolve_next_id(object)
  end)

  self:on_unload(function(area_id)
    for _, area_map in pairs(event_map) do
      area_map[area_id] = nil
    end

    player_interaction_event_map[area_id] = nil
  end)

  local function execute_object_ids(area_id, object_ids, context)
    for _, object_id in ipairs(object_ids) do
      self:execute_by_id(clone_table(context), area_id, object_id)
    end
  end

  local any_listener = function(event_name, event)
    local area_map = event_map[event_name]

    if not area_map then
      return
    end

    if event.player_id then
      local area_id = Net.get_player_area(event.player_id)

      local object_ids = area_map[area_id]

      if object_ids then
        execute_object_ids(area_id, object_ids, {
          area_id = area_id,
          player_id = event.player_id,
          object_id = event.object_id,
          x = event.x,
          y = event.y,
          z = event.z,
        })
      end
    else
      for area_id, object_ids in pairs(area_map) do
        execute_object_ids(area_id, object_ids, {
          area_id = area_id,
        })
      end
    end
  end

  Net:on_any(any_listener)

  self:on_server_event("actor_interaction", function(event)
    if not Net.is_player(event.actor_id) or event.button ~= 0 then
      return
    end

    local area_id = Net.get_player_area(event.player_id)
    local object_ids = player_interaction_event_map[area_id]

    if not object_ids then
      return
    end

    execute_object_ids(area_id, object_ids, {
      area_id = area_id,
      player_ids = { event.player_id, event.actor_id }
    })
  end)

  self:on_server_event("tile_interaction", function(event)
    if event.button ~= 0 then
      return
    end

    local area_id = Net.get_player_area(event.player_id)
    local object_ids = tile_interaction_event_map[area_id]

    if not object_ids then
      return
    end

    execute_object_ids(area_id, object_ids, {
      area_id = area_id,
      player_id = event.player_id,
      x = event.x,
      y = event.y,
      z = event.z
    })
  end)

  local function help_listener(event)
    if event.button ~= 1 then
      return
    end

    local area_id = Net.get_player_area(event.player_id)
    local object_ids = help_event_map[area_id]

    if not object_ids then
      return
    end

    execute_object_ids(area_id, object_ids, {
      area_id = area_id,
      player_id = event.player_id
    })
  end

  self:on_server_event("actor_interaction", help_listener)
  self:on_server_event("object_interaction", help_listener)
  self:on_server_event("tile_interaction", help_listener)


  local item_event_map = {}

  self:on_entry_node_load("item use", function(area_id, object)
    local item_map = item_event_map[area_id]

    if not item_map then
      item_map = {}
      item_event_map[area_id] = item_map
    end

    local next_id = self:resolve_next_id(object)
    item_map[object.custom_properties.Item] = next_id
  end)

  self:on_server_event("item_use", function(event)
    local area_id = Net.get_player_area(event.player_id)
    local item_map = item_event_map[area_id]
    local next_id = item_map and item_map[event.item_id]

    if not next_id then
      return
    end

    local context = { area_id = area_id, player_id = event.player_id }
    self:execute_by_id(context, area_id, next_id)
  end)

  self:on_destroy(function()
    Net:remove_on_any_listener(any_listener)
  end)
end

---Implements support for `Set Area Name`, `Set Area Background`, `Set Area Foreground`,
--- `Set Area Music`, `Transfer To Area`, `Transfer To Instance`, `Transfer In Instance`, and `Remove Area` nodes.
---
---Expects `area_id` to be defined on the context table.
---
---Custom properties supported by `Set Area Name`:
--- - `Name` the new name of the area
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Set Area Background` and `Set Area Foreground`:
--- - `Texture` the path to the texture
--- - `Animation` the path to the animation (optional)
--- - `Vel X` number (optional)
--- - `Vel Y` number (optional)
--- - `Parallax` number (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Set Area Music`:
--- - `Music` the path to the new music
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Transfer To Area`:
--- - `Area` string (ignored when `Target` is set to a player or bot)
--- - `Target` object, decides the spawn position and can supply `Warp` and `Direction` values (optional)
--- - `Warp` boolean, decides whether players should play a warp animation (optional)
--- - `Direction` string (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Transfer To Instance`:
--- - `Area` string, a template area used to seed the new instance
--- - `Target` object, decides the spawn position and can supply `Warp` and `Direction` values (optional)
--- - `Warp` boolean, decides whether players should play a warp animation (optional)
--- - `Direction` string (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Transfer In Instance`:
--- - `Area` string, a template area used to seed a new area in the instance if the area doesn't exist yet
--- - `Target` object, decides the spawn position and can supply `Warp` and `Direction` values (optional)
--- - `Warp` boolean, decides whether players should play a warp animation (optional)
--- - `Direction` string (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Note: `Remove Area` does not support any custom properties
function ScriptNodes:implement_area_api()
  self:implement_node("set area name", function(context, object)
    Net.set_area_name(context.area_id, object.custom_properties.Name)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("set area background", function(context, object)
    local texture = object.custom_properties.Texture
    local animation = object.custom_properties.Animation

    Net.set_background(
      context.area_id,
      texture and (self.ASSET_PREFIX .. texture),
      animation and (self.ASSET_PREFIX .. animation),
      tonumber(object.custom_properties["Vel X"]),
      tonumber(object.custom_properties["Vel Y"]),
      tonumber(object.custom_properties["Parallax"])
    )

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("set area foreground", function(context, object)
    local texture = object.custom_properties.Texture
    local animation = object.custom_properties.Animation

    Net.set_foreground(
      context.area_id,
      texture and (self.ASSET_PREFIX .. texture),
      animation and (self.ASSET_PREFIX .. animation),
      tonumber(object.custom_properties["Vel X"]),
      tonumber(object.custom_properties["Vel Y"]),
      tonumber(object.custom_properties["Parallax"])
    )

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("set area music", function(context, object)
    Net.set_music(context.area_id, object.custom_properties.Music)

    self:execute_next_node(context, context.area_id, object)
  end)

  local function transfer_to_area(context, object)
    local area_id = object.custom_properties.Area
    local x, y, z
    local warp_in
    local direction

    warp_in, x, y, z, direction = self:resolve_teleport_properties(object, area_id)

    if not x then
      x, y, z = Net.get_spawn_position_multi(area_id)
    end

    if not direction then
      direction = Net.get_spawn_direction(area_id)
    end

    if context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        Net.transfer_player(player_id, area_id, warp_in, x, y, z, direction)
      end
    else
      Net.transfer_player(context.player_id, area_id, warp_in, x, y, z, direction)
    end

    self:execute_next_node(context, context.area_id, object)
  end

  self:implement_node("transfer to area", transfer_to_area)

  self:implement_node("transfer to instance", function(context, object)
    local template_id = object.custom_properties.Area or context.area_id
    local instancer = self:instancer()
    local instance_id = instancer:create_instance()
    local new_area_id = instancer:clone_area_to_instance(instance_id, template_id) --[[@as string]]
    self:load(new_area_id)

    local warp_in, x, y, z, direction = self:resolve_teleport_properties(object, new_area_id)

    if not x then
      x, y, z = Net.get_spawn_position_multi(new_area_id)
    end

    if not direction then
      direction = Net.get_spawn_direction(new_area_id)
    end

    for_each_player(context, function(player_id)
      Net.transfer_player(player_id, new_area_id, warp_in, x, y, z, direction)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("transfer in instance", function(context, object)
    local instance_id = self:instancer():resolve_instance_id(context.area_id)

    if not instance_id then
      transfer_to_area(context, object)
      return
    end

    local template_id = object.custom_properties.Area or context.area_id
    local new_area_id = template_id .. self._instancer.INSTANCE_MARKER .. instance_id

    if not self:is_loaded(new_area_id) then
      -- create an instance of the template area
      self:instancer():clone_area_to_instance(instance_id, template_id)
    end

    local warp_in, x, y, z, direction = self:resolve_teleport_properties(object, new_area_id)

    if not x then
      x, y, z = Net.get_spawn_position_multi(new_area_id)
    end

    if not direction then
      direction = Net.get_spawn_direction(new_area_id)
    end

    for_each_player(context, function(player_id)
      Net.transfer_player(player_id, new_area_id, warp_in, x, y, z, direction)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("remove area", function(context)
    if self:instancer():remove_area(context.area_id) then
      return
    end

    for _, bot_id in ipairs(Net.list_bots(context.area_id)) do
      self:emit_bot_remove(bot_id)
      Net.remove_bot(bot_id)
    end

    self:unload(context.area_id)
    Net.remove_area(context.area_id)
  end)
end

---Implements support for the `On Interact` property on objects,
---the `On Warp` property on `Custom Warp` objects,
---and support for the `Move Object`, `Hide Object`, `Show Object`, and `Remove Object` nodes
---
---`On Interact` should be applied on an interactable object, and the value should be an object usable as a script node.
---When players interact with the object, the node will execute using a context containing `area_id`, `player_id`, and `object_id`.
---
---`On Warp` should be applied on a `Custom Warp` object, and the value should be an object usable as a script node.
---When players warp using this object, the node will execute using a context containing `area_id`, `player_id`, and `object_id`.
---
---All nodes expect `area_id` and optionally `object_id` on the context table.
---
---Custom properties supported by `Move Object`:
--- - `Target` object (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Hide Object`, `Show Object`, and `Remove Object`:
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_object_api()
  self:on_server_event("object_interaction", function(event)
    if event.button ~= 0 then
      return
    end

    local area_id = Net.get_player_area(event.player_id)

    if not self:is_loaded(area_id) then
      return
    end

    local object = Net.get_object_by_id(area_id, event.object_id)
    local interact_id = object.custom_properties["On Interact"]

    if interact_id then
      local context = {
        area_id = area_id,
        player_id = event.player_id,
        object_id = object.id
      }
      self:execute_by_id(context, area_id, interact_id)
    end
  end)

  self:on_server_event("custom_warp", function(event)
    local area_id = Net.get_player_area(event.player_id)

    if not self:is_loaded(area_id) then
      return
    end

    local object = Net.get_object_by_id(area_id, event.object_id)
    local next_id = object.custom_properties["On Warp"]

    if next_id then
      local context = {
        area_id = area_id,
        player_id = event.player_id,
        object_id = event.object_id
      }

      self:execute_by_id(context, area_id, next_id)
    end
  end)

  self:implement_node("move object", function(context, object)
    local object_id = object.custom_properties.Object or context.object_id
    local target_id = object.custom_properties.Target
    local target_object = (target_id and self:resolve_object(context.area_id, target_id)) or object
    Net.move_object(context.area_id, object_id, target_object.x, target_object.y, target_object.z)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("clone object", function(context, object)
    local source_id = object.custom_properties.Source
    local dest_id = object.custom_properties.Destination
    local source_object = (source_id and self:resolve_object(context.area_id, source_id)) or object
    local dest_object = (dest_id and self:resolve_object(context.area_id, dest_id)) or object

    source_object = clone_table(source_object)
    source_object.x = dest_object.x
    source_object.y = dest_object.y
    source_object.z = dest_object.z
    Net.create_object(context.area_id, source_object --[[@as Net.ObjectOptions]])

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("hide object", function(context, object)
    local object_id = object.custom_properties.Object or context.object_id
    Net.set_object_visibility(context.area_id, object_id, false)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("show object", function(context, object)
    local object_id = object.custom_properties.Object or context.object_id
    Net.set_object_visibility(context.area_id, object_id, true)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("remove object", function(context, object)
    local object_id = object.custom_properties.Object or context.object_id
    Net.remove_object(context.area_id, object_id)

    self:execute_next_node(context, context.area_id, object)
  end)
end

---Implements support for the `Has Tile Id` and `Set Tile Id` nodes
---
---All nodes expect `area_id` and optionally `object_id` on the context table.
---
---Custom properties supported by `Has Tile Id`:
--- - `Target` object, expects a tile object or a tile under the object (optional)
--- - `Next [1]` a link to the default node (optional)
--- - `Next [1]` a link to the passing node (optional)
---
---Custom properties supported by `Set Tile Id`:
--- - `Target` object, expects a tile object or a tile under the object (optional)
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_tile_api()
  local function resolve_gid(area_id, tileset_path, id)
    local gid = tonumber(id)

    if tileset_path then
      local tileset = Net.get_tileset(area_id, tileset_path)

      if not tileset then
        error('no tileset matching "' .. tileset .. '" in ' .. area_id)
      end

      gid = tileset.first_gid + gid
    end

    return gid
  end

  local function object_or_tile(context, object, object_callback, tile_callback)
    local target = object.custom_properties.Target or context.object_id

    if target then
      local target_object = self:resolve_object(context.area_id, target)

      if target_object then
        if target_object.data.type == "tile" then
          object_callback(target_object)
        else
          tile_callback(target_object.x, target_object.y, target_object.z)
        end
      end
    elseif context.x then
      tile_callback(context.x, context.y, context.z)
    end
  end

  self:implement_node("has tile id", function(context, object)
    local gid = resolve_gid(context.area_id, object.custom_properties.Tileset, object.custom_properties.Id)
    local pass = false

    object_or_tile(context, object,
      function(target_object)
        pass = target_object and target_object.data.gid == gid
      end,
      function(x, y, z)
        pass = Net.get_tile(context.area_id, x, y, z) == gid
      end
    )

    if pass then
      self:execute_next_node(context, context.area_id, object, 2)
    else
      self:execute_next_node(context, context.area_id, object)
    end
  end)

  self:implement_node("set tile id", function(context, object)
    local gid = resolve_gid(context.area_id, object.custom_properties.Tileset, object.custom_properties.Id)

    object_or_tile(context, object,
      function(target_object)
        target_object.data.gid = gid
        Net.set_object_data(context.area_id, target_object.id, target_object.data)
      end,
      function(x, y, z)
        Net.set_tile(context.area_id, x, y, z, gid)
      end
    )

    self:execute_next_node(context, context.area_id, object)
  end)
end

---Implements support for `Message`, `Auto Message`, `Question`, and `Quiz` nodes
---
---All nodes expect `area_id` and either `player_id` or `player_ids` defined on the context table.
---
---If `player_ids` is defined, the script will wait for all players and resolve to the first option as the response.
---
---Supported custom properties:
--- - `Mug` "Random Player" | "Player [1+]" | string, the extensionless texture and animation for the mug. (optional)
--- - `Mug Texture` string, the texture for the mug. (optional)
--- - `Mug Animation` string, the animation for the mug. (optional)
--- - `Close Delay` a duration in seconds to wait before closing the textbox. (`Auto Message` specific, optional)
--- - `Text [1+]`
--- - `Next [1+]` a link to the next node based on the response (optional)
function ScriptNodes:implement_textbox_api()
  ---@param callback fun(player_id: Net.ActorId): Net.Promise
  local function single_response_for_all(context, callback)
    if context.player_ids then
      local promises = {}

      for _, player_id in ipairs(context.player_ids) do
        promises[#promises + 1] = callback(player_id)
      end

      return Async.create_scope(function()
        Async.await_all(promises)

        return 0
      end)
    end

    return callback(context.player_id)
  end


  self:implement_node("auto message", function(context, object)
    local mug_texture, mug_animation = self:resolve_mug(context, object)

    local promise = single_response_for_all(context, function(player_id)
      return Async.message_player_auto(
        player_id,
        object.custom_properties["Text"] or object.custom_properties["Text 1"],
        tonumber(object.custom_properties["Close Delay"]) or 0,
        mug_texture,
        mug_animation
      )
    end)

    promise.and_then(function(response)
      if response == nil then
        return
      end

      local next = self:resolve_next_node(context.area_id, object)

      if next then
        self:execute_node(context, next)
      end
    end)
  end)

  self:implement_node("message", function(context, object)
    local mug_texture, mug_animation = self:resolve_mug(context, object)

    local promise = single_response_for_all(context, function(player_id)
      return Async.message_player(
        player_id,
        object.custom_properties["Text"] or object.custom_properties["Text 1"],
        mug_texture,
        mug_animation
      )
    end)

    promise.and_then(function(response)
      if response == nil then
        return
      end

      local next = self:resolve_next_node(context.area_id, object)

      if next then
        self:execute_node(context, next)
      end
    end)
  end)

  self:implement_node("question", function(context, object)
    local mug_texture, mug_animation = self:resolve_mug(context, object)

    local promise = single_response_for_all(context, function(player_id)
      return Async.question_player(
        player_id,
        object.custom_properties["Text"] or object.custom_properties["Text 1"],
        mug_texture,
        mug_animation
      )
    end)

    promise.and_then(function(response)
      if response == nil then
        return
      end

      local next = self:resolve_next_node(context.area_id, object, response + 1)

      if next then
        self:execute_node(context, next)
      end
    end)
  end)

  self:implement_node("quiz", function(context, object)
    local mug_texture, mug_animation = self:resolve_mug(context, object)

    local promise = single_response_for_all(context, function(player_id)
      return Async.quiz_player(
        player_id,
        object.custom_properties["Text 1"],
        object.custom_properties["Text 2"],
        object.custom_properties["Text 3"],
        mug_texture,
        mug_animation
      )
    end)

    promise.and_then(function(response)
      if response == nil then
        return
      end

      local next = self:resolve_next_node(context.area_id, object, response + 1)

      if next then
        self:execute_node(context, next)
      end
    end)
  end)
end

---Implements support for the `BBS` node.
---
---Expects `area_id` and `player_id` to be defined on the context table.
---
---Custom properties supported by `BBS`:
--- - `Color` color (optional)
--- - `Name` string (optional)
--- - `Instant` boolean, skips the open and close animations (optional)
--- - `Post 1+` object (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by Post objects:
--- - `Mug` "Random Player" | "Player [1+]" | string, the extensionless texture and animation for the mug. (optional)
--- - `Mug Texture` string, the texture for the mug. (optional)
--- - `Mug Animation` string, the animation for the mug. (optional)
--- - `Title` string (optional)
--- - `Author` string (optional)
--- - `Text` string (optional)
--- - `On Interact` a link to a script node (optional)
function ScriptNodes:implement_bbs_api()
  ---@type table<string, table<number, Net.BoardPost[]>>
  local area_posts_map = {}
  local areas_completed = {}

  self:implement_node("bbs", function(context, object)
    if context.player_ids then
      error("the BBS node does not support parties. Use a Disband Party node before using")
    end

    local base_area_id = self:instancer():resolve_base_area_id(context.area_id)
    local posts = area_posts_map[base_area_id][object.id]

    local emitter = Net.open_board(
      context.player_id,
      object.custom_properties.Name,
      parse_color(object.custom_properties.Color) or { r = 255, g = 255, b = 255 },
      posts,
      object.custom_properties.Instant == "true"
    )

    if not emitter then
      self:execute_next_node(context, context.area_id, object)
      return
    end

    Async.create_scope(function()
      for event in Async.await(emitter:async_iter("post_selection")) do
        local post_object = self:resolve_object(context.area_id, event.post_id)

        local text = post_object.custom_properties.Text

        if text then
          local mug_texture, mug_animation = self:resolve_mug(context, object)
          Net.message_player(context.player_id, text, mug_texture, mug_animation)
        end

        local next_id = post_object.custom_properties["On Interact"]

        if next_id then
          self:execute_by_id(context, context.area_id, next_id)
        end
      end

      self:execute_next_node(context, context.area_id, object)

      return nil
    end)
  end)

  self:on_load_start(function(area_id)
    local base_area_id = self:instancer():resolve_base_area_id(area_id)

    if not area_posts_map[base_area_id] then
      area_posts_map[base_area_id] = {}
    end
  end)

  self:on_load(function(area_id)
    local base_area_id = self:instancer():resolve_base_area_id(area_id)
    areas_completed[base_area_id] = true
  end)

  self:on_script_node_load("bbs", function(area_id, object)
    local base_area_id = self:instancer():resolve_base_area_id(area_id)
    local posts = {}
    local i = 1

    while true do
      local post_id = object.custom_properties["Post " .. i]

      if not post_id then
        break
      end

      local post_object = self:resolve_object(area_id, post_id)

      if not post_object then
        break
      end

      self:cache_object(area_id, post_object)
      Net.set_object_privacy(area_id, post_object.id, true)

      if not areas_completed[base_area_id] then
        posts[#posts + 1] = {
          id = post_id,
          read = true,
          title = post_object.custom_properties.Title or "",
          author = post_object.custom_properties.Author or "",
        }
      end

      i = i + 1
    end

    if not areas_completed[base_area_id] then
      area_posts_map[area_id][object.id] = posts
    end
  end)
end

---Implements support for the `Shop` node.
---
---Expects `area_id` and `player_id` to be defined on the context table.
---
---Custom properties supported by `Shop`:
--- - `Mug` "Random Player" | "Player [1+]" | string, the extensionless texture and animation for the shop keeper's mug. (optional)
--- - `Mug Texture` string, the texture for the shop keeper's mug. (optional)
--- - `Mug Animation` string, the animation for the shop keeper's mug. (optional)
--- - `Text` string, the shop keeper's default message (optional)
--- - `Welcome Text` string, shop keeper's welcome message (optional)
--- - `Leave Text` string, the shop keeper's goodbye message (optional)
--- - `Purchase Question` string, the shop keeper's confirmation text (optional)
--- - `Item 1+` object (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by Item objects:
--- - `Name` string (optional)
--- - `Price` number (optional)
--- - `Description` string (optional)
--- - `Item` string, an item id, used to populate a name and description for this shop item (optional)
--- - `On Interact` a link to a script node (optional)
function ScriptNodes:implement_shop_api()
  ---@type table<string, table<number, Net.ShopItem[]>>
  local area_shop_map = {}
  local areas_completed = {}

  self:implement_node("shop", function(context, object)
    if context.player_ids then
      error("the Shop node does not support parties. Use a Disband Party node before using")
    end

    local base_area_id = self:instancer():resolve_base_area_id(context.area_id)
    local items = area_shop_map[base_area_id][object.id]

    local mug_texture, mug_animation = self:resolve_mug(context, object)

    local emitter = Net.open_shop(
      context.player_id,
      items,
      mug_texture,
      mug_animation
    )

    if not emitter then
      self:execute_next_node(context, context.area_id, object)
      return
    end

    local default_message = object.custom_properties["Text"]
    local welcome_message = object.custom_properties["Welcome Text"] or default_message
    local purchase_question = object.custom_properties["Purchase Question"]
    local leave_message = object.custom_properties["Leave Text"]

    if welcome_message then
      Net.set_shop_message(context.player_id, welcome_message)
    end

    Async.create_scope(function()
      for event_name, event in Async.await(emitter:async_iter_all()) do
        if event_name == "shop_description_request" then
          local item_object = self:resolve_object(context.area_id, event.item_id)
          local description = item_object.custom_properties.Description

          if not description and item_object.custom_properties.Item then
            description = Net.get_item_description(item_object.custom_properties.Item)
          end

          if description then
            Net.message_player(event.player_id, description)
          end
        elseif event_name == "shop_purchase" then
          local item_object = self:resolve_object(context.area_id, event.item_id)
          local next_id = item_object.custom_properties["On Interact"]

          if purchase_question then
            local promise = Async.question_player(context.player_id, purchase_question, mug_texture, mug_animation)
            local response = Async.await(promise)

            if response == 1 and next_id then
              self:execute_by_id(context, context.area_id, next_id)
            end
          elseif next_id then
            self:execute_by_id(context, context.area_id, next_id)
          end

          if default_message then
            Net.set_shop_message(context.player_id, default_message)
          end
        elseif event_name == "shop_leave" and leave_message then
          Net.set_shop_message(context.player_id, leave_message)
        end
      end

      self:execute_next_node(context, context.area_id, object)

      return nil
    end)
  end)

  self:on_load_start(function(area_id)
    local base_area_id = self:instancer():resolve_base_area_id(area_id)

    if not area_shop_map[base_area_id] then
      area_shop_map[base_area_id] = {}
    end
  end)

  self:on_load(function(area_id)
    local base_area_id = self:instancer():resolve_base_area_id(area_id)
    areas_completed[base_area_id] = true
  end)

  self:on_script_node_load("shop", function(area_id, object)
    local base_area_id = self:instancer():resolve_base_area_id(area_id)

    local items = {}
    local i = 1

    while true do
      local item_id = object.custom_properties["Item " .. i]

      if not item_id then
        break
      end

      local item_object = self:resolve_object(area_id, item_id)

      if not item_object then
        break
      end

      self:cache_object(area_id, item_object)
      Net.set_object_privacy(area_id, item_object.id, true)

      if not areas_completed[base_area_id] then
        local name = item_object.custom_properties.Name

        if not name and item_object.custom_properties.Item then
          name = Net.get_item_name(item_object.custom_properties.Item)
        end

        items[#items + 1] = {
          id = item_id,
          name = name or "",
          price = tonumber(item_object.custom_properties.Price) or 0,
        }
      end

      i = i + 1
    end

    if not areas_completed[base_area_id] then
      area_shop_map[base_area_id][object.id] = items
    end
  end)
end

---Implements support for the `Play Sound` node.
---
---Expects `area_id` and optionally `player_id` or `player_ids` to be defined on the context table.
---
---Supported custom properties:
--- - `Target` "Player [1+]" (optional)
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_sound_api()
  self:implement_node("play sound", function(context, object)
    local sound_path = self.ASSET_PREFIX .. object.custom_properties.Sound

    if not object.custom_properties.Target then
      Net.play_sound(context.area_id, sound_path)
    elseif object.custom_properties.Target == "Player" and context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        Net.play_sound_for_player(player_id, sound_path)
      end
    else
      local resolved_id = self:resolve_player_id(context, object.custom_properties.Target)

      if resolved_id then
        Net.play_sound_for_player(resolved_id, sound_path)
      else
        for_each_player(context, function(player_id)
          Net.play_sound_for_player(player_id, sound_path)
        end)
      end
    end

    self:execute_next_node(context, context.area_id, object)
  end)
end

---Implements support for the `Camera` node.
---
---Expects `area_id` and either `player_id` or `player_ids` to be defined on the context table.
---
---Supported custom properties:
--- - `Duration` the duration of the effect in seconds (optional)
--- - `Fade` a color to fade to (optional)
--- - `Shake` a number representing the strength of the shake effect (optional)
--- - `Motion` "Snap" | "Slide" | "Track" (optional)
--- - `Target` "Player [1+]" | "Bot [id]" | object (optional)
--- - `Unlocks` boolean, unlocks the camera at the end of the effect if true (optional)
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_camera_api()
  ---@param object Net.Object
  local function parse_properties(context, object)
    local properties = {}

    properties.Duration = tonumber(object.custom_properties.Duration) or 0
    properties.Shake = tonumber(object.custom_properties.Shake) or 0
    properties.Motion = object.custom_properties.Motion
    properties.Unlock = object.custom_properties.Unlocks == "true"

    properties.Fade = parse_color(object.custom_properties.Fade)

    if object.custom_properties.Target then
      local actor_id = self:resolve_actor_id(context, object.custom_properties.Target)

      if not actor_id then
        local target_object = self:resolve_object(context.area_id, object.custom_properties.Target)
        properties.x = target_object.x
        properties.y = target_object.y
        properties.z = target_object.z
      elseif Net.is_player(actor_id) then
        properties.x, properties.y, properties.z = Net.get_player_position_multi(actor_id)
        properties.actor_id = actor_id
      elseif Net.is_bot(actor_id) then
        properties.x, properties.y, properties.z = Net.get_bot_position_multi(actor_id)
        properties.actor_id = actor_id
      end
    else
      properties.x = object.x
      properties.y = object.y
      properties.z = object.z
    end

    return properties
  end

  local function process_camera_node(player_id, properties)
    local duration = properties.Duration

    if properties.Fade then
      Net.fade_player_camera(player_id, properties.Fade, duration)
    end

    if properties.Shake > 0 then
      Net.shake_player_camera(player_id, properties.Shake, duration)
    end

    if properties.Motion == "Track" and properties.actor_id then
      Net.track_with_player_camera(player_id, properties.actor_id)
    elseif properties.Motion == "Slide" then
      Net.slide_player_camera(player_id, properties.x, properties.y, properties.z, duration)
    else
      Net.move_player_camera(player_id, properties.x, properties.y, properties.z, duration)
    end
  end

  self:implement_node("camera", function(context, object)
    local properties = parse_properties(context, object)

    Net.synchronize(function()
      if context.player_id then
        process_camera_node(context.player_id, properties)
      elseif context.player_ids then
        for _, player_id in ipairs(context.player_ids) do
          if object.custom_properties.Target == "Player" then
            properties.x, properties.y, properties.z = Net.get_player_position_multi(player_id)
            properties.actor_id = player_id
          end

          process_camera_node(player_id, properties)
        end
      end
    end)

    Async.sleep(properties.Duration).and_then(function()
      if properties.Unlock then
        if context.player_id then
          Net.unlock_player_camera(context.player_id)
        elseif context.player_ids then
          for _, player_id in ipairs(context.player_ids) do
            Net.unlock_player_camera(player_id)
          end
        end
      end

      self:execute_next_node(context, context.area_id, object)
    end)
  end)
end

---Implements support for the `Encounter` node.
---
---Expects `area_id` and `player_id` or `player_ids` to be defined on the context table.
---
---Supported custom properties:
--- - `Encounter` the duration of the delay in seconds (optional)
--- - `Data` string, custom data to pass to the encounter (optional)
--- - `Forget Results` boolean (optional)
--- - `Next [1]` a link to the next node (optional)
--- - `On Win` a link to a node, threads per player (optional)
--- - `On Lose` a link to a node, threads per player (optional)
function ScriptNodes:implement_encounter_api()
  self:implement_node("encounter", function(context, object)
    local resolve
    local promise = Async.create_promise(function(r)
      resolve = r
    end)

    local remember_results = object.custom_properties["Forget Results"] ~= "true"

    local win_id = object.custom_properties["On Win"]
    local win_node = win_id and self:resolve_object(context.area_id, win_id)
    local lose_id = object.custom_properties["On Lose"]
    local lose_node = lose_id and self:resolve_object(context.area_id, lose_id)

    if context.player_ids and #context.player_ids > 0 then
      local promises = Async.initiate_netplay(
        context.player_ids,
        object.custom_properties.Encounter,
        object.custom_properties.Data
      )

      local completion_count = 0

      for _, player_promise in ipairs(promises) do
        player_promise.and_then(function(result)
          completion_count = completion_count + 1

          if completion_count == #promises then
            resolve(nil)
          end

          -- thread result
          if result then
            if result.won and win_node then
              context = clone_table(context)
              context.player_ids = nil
              context.player_id = result.player_id
              self:execute_node(context, win_node)
            elseif not result.won and lose_node then
              context = clone_table(context)
              context.player_ids = nil
              context.player_id = result.player_id
              self:execute_node(context, lose_node)
            end

            if remember_results then
              self:emit_encounter_result(result)
            end
          end
        end)
      end
    elseif context.player_id then
      local player_promise = Async.initiate_encounter(
        context.player_id,
        object.custom_properties.Encounter,
        object.custom_properties.Data
      )

      player_promise.and_then(function(result)
        resolve(nil)

        -- execute result node
        if result then
          if result.won and win_node then
            self:execute_node(context, win_node)
          elseif not result.won and lose_node then
            self:execute_node(context, lose_node)
          end

          if remember_results then
            self:emit_encounter_result(result)
          end
        end
      end)
    else
      resolve()
    end

    promise.and_then(function()
      self:execute_next_node(context, context.area_id, object)
    end)
  end)
end

---Implements support for the `Give Money`, `Take Money`, `Has Money`,
---`Give Item`, `Take Item`, and `Has Item` nodes.
---
---Expects `area_id` and `player_id` or `player_ids` to be defined on the context table.
---
---Supported custom properties for `Give Money`:
--- - `Amount` the amount of money to give
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Take Money`:
--- - `Amount` the amount of money to take, no money will be taken on failure
--- - `Next [1]` a link to the default node (optional)
--- - `Next 2` a link to the passing node (optional)
---
---Supported custom properties for `Has Money`:
--- - `Amount` the minumum amount of money to pass the check
--- - `Next [1]` a link to the default node (optional)
--- - `Next 2` a link to the passing node (optional)
---
---Supported custom properties for `Give Item`:
--- - `Item` the ID of the item
--- - `Amount` the amount of items to give (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Take Item`:
--- - `Item` the ID of the item
--- - `Amount` the amount of items to take, no items will be taken on failure (optional)
--- - `Next [1]` a link to the default node (optional)
--- - `Next 2` a link to the passing node (optional)
---
---Supported custom properties for `Has Item`:
--- - `Item` the ID of the item
--- - `Amount` the minumum amount of items to pass the check (optional)
--- - `Next [1]` a link to the default node (optional)
--- - `Next 2` a link to the passing node (optional)
function ScriptNodes:implement_inventory_api()
  self:implement_node("give money", function(context, object)
    local amount = tonumber(object.custom_properties.Amount)

    for_each_player_safe(context, function(player_id)
      Net.set_player_money(player_id, Net.get_player_money(player_id) + amount)
      self:emit_money_update(player_id)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("take money", function(context, object)
    local amount = tonumber(object.custom_properties.Amount)
    local pass = true

    for_each_player_safe(context, function(player_id)
      local money = Net.get_player_money(player_id)

      if money then
        pass = pass and money >= amount
      end
    end)

    if pass then
      for_each_player_safe(context, function(player_id)
        local money = Net.get_player_money(player_id)

        if money then
          Net.set_player_money(player_id, math.max(0, money - amount))
          self:emit_money_update(player_id)
        end
      end)

      self:execute_next_node(context, context.area_id, object, 2)
    else
      self:execute_next_node(context, context.area_id, object)
    end
  end)

  self:implement_node("has money", function(context, object)
    local amount = tonumber(object.custom_properties.Amount)
    local pass = true

    for_each_player_safe(context, function(player_id)
      pass = pass and Net.get_player_money(player_id) >= amount
    end)

    if pass then
      self:execute_next_node(context, context.area_id, object, 2)
    else
      self:execute_next_node(context, context.area_id, object)
    end
  end)

  self:implement_node("give item", function(context, object)
    local item_id = object.custom_properties.Item
    local amount = tonumber(object.custom_properties.Amount) or 1

    for_each_player_safe(context, function(player_id)
      Net.give_player_item(player_id, item_id, amount)
      self:emit_inventory_update(player_id, item_id)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("take item", function(context, object)
    local item_id = object.custom_properties.Item
    local amount = tonumber(object.custom_properties.Amount) or 1
    local pass = true

    for_each_player_safe(context, function(player_id)
      local count = Net.get_player_item_count(player_id, item_id)

      if count then
        pass = pass and count >= amount
      end
    end)

    if pass then
      for_each_player_safe(context, function(player_id)
        local count = Net.get_player_item_count(player_id, item_id)

        if count then
          Net.give_player_item(player_id, item_id, -amount)
          self:emit_inventory_update(player_id, item_id)
        end
      end)

      self:execute_next_node(context, context.area_id, object, 2)
    else
      self:execute_next_node(context, context.area_id, object)
    end
  end)

  self:implement_node("has item", function(context, object)
    local item_id = object.custom_properties.Item
    local amount = tonumber(object.custom_properties.Amount) or 1
    local pass = true

    for_each_player_safe(context, function(player_id)
      pass = pass and Net.get_player_item_count(player_id, item_id) >= amount
    end)

    if pass then
      self:execute_next_node(context, context.area_id, object, 2)
    else
      self:execute_next_node(context, context.area_id, object)
    end
  end)
end

---Implements support for the `Refer Link`, `Refer Server` and `Refer Package` nodes
---
---Expects `area_id` and `player_id` or `player_ids` to be defined on the context table.
---
---Supported custom properties for `Refer Link`:
--- - `Address` string
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Refer Server`:
--- - `Name` string
--- - `Address` string
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Refer Package`:
--- - `Id` string
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_link_api()
  self:implement_node("refer link", function(context, object)
    local address = object.custom_properties.Address

    for_each_player_safe(context, function(player_id)
      Net.refer_link(player_id, address)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("refer server", function(context, object)
    local name = object.custom_properties.Name
    local address = object.custom_properties.Address

    for_each_player_safe(context, function(player_id)
      Net.refer_server(player_id, name, address)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("refer package", function(context, object)
    local id = object.custom_properties.Id

    for_each_player_safe(context, function(player_id)
      Net.refer_package(player_id, id)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)
end

---Implements support for `Spawn Bot`, `Remove Bot`, `Emote`,
--- `Face`, `Lock Input`, and `Unlock Input` nodes.
---
---Expects `area_id` to be defined on the context table.
---Some nodes may also require `bot_id` and `player_id` or `player_ids` to be defined on the context table.
---
---Custom properties supported by `Spawn Bot`:
--- - `Id` the identifer for the bot for use in nodes matching "Bot [id]" (optional, tied to area)
--- - `Name` the displayed name of the bot (optional)
--- - `Warp In` boolean, whether the bot should warp in or not (optional)
--- - `Asset` the extensionless path to the texture and animation for the bot (optional if `Texture` and `Animation` are set)
--- - `Texture` the path to the texture for the bot (ignored if `Asset` is set)
--- - `Animation` the path to the animation for the bot (ignored if `Asset` is set)
--- - `Animation State` an initial animation to play on the bot (optional)
--- - `Direction` an initial direction for the bot to face (optional)
--- - `Solid` boolean, whether this bot should collide with players or not (optional)
--- - `Next [1]` a link to the next node (optional)
--- - `On Interact` object, a script node to execute on interactions (optional)
---
---    The node will execute using a context populated with `area_id`, `player_id`, and `bot_id`.
---
---Custom properties supported by `Remove Bot`:
--- - `Id` the identifier for the bot (optional)
--- - `Warp Out` boolean (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Emote`:
--- - `Actor` "Player [1+]" | "Bot [id]" (optional)
--- - `Emote` string
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Face`:
--- - `Actor` "Player [1+]" | "Bot [id]" (optional)
--- - `Direction` string (optional)
--- - `Target` "Player [1+]" | "Bot [id]" | object (optional)
--- - `Target Diagonally` boolean, when true the `Target` will resolve to a diagonal direction (optional)
--- - `Target Diagonal` (alias for `Target Diagonally`, optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Animate`:
--- - `Actor` "Player [1+]" | "Bot [id]" (optional)
--- - `Animation State` string
--- - `Loop` boolean (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Teleport`:
--- - `Target` object, decides the position and can supply `Warp` and `Direction` values (optional)
--- - `Warp` boolean, decides whether players should play a warp animation (optional)
--- - `Direction` string (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Lock Input` and `Unlock Input`:
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_actor_api()
  local interaction_map = {}

  self:on_server_event("actor_interaction", function(event)
    if event.button ~= 0 then
      return
    end

    local bot_id = event.actor_id
    local object_id = interaction_map[bot_id]

    if not object_id then
      return
    end

    local area_id = Net.get_player_area(event.player_id)
    local context = {
      area_id = area_id,
      player_id = event.player_id,
      bot_id = bot_id
    }
    self:execute_by_id(context, area_id, object_id)
  end)

  self:on_bot_removed(function(bot_id)
    interaction_map[bot_id] = nil
  end)

  self:implement_node("spawn bot", function(context, object)
    local options = {}
    options.name = object.custom_properties.Name
    options.area_id = context.area_id
    options.warp_in = object.custom_properties["Warp In"] == "true"
    options.animation = object.custom_properties["Animation State"]
    options.x = object.x
    options.y = object.y
    options.z = object.z
    options.direction = object.custom_properties.Direction

    if object.custom_properties.Texture then
      options.texture_path = self.ASSET_PREFIX .. object.custom_properties.Texture
    end

    if object.custom_properties.Animation then
      options.animation_path = self.ASSET_PREFIX .. object.custom_properties.Animation
    end

    if object.custom_properties.Asset then
      local prefixed_path = self.ASSET_PREFIX .. object.custom_properties.Asset
      options.texture_path = prefixed_path .. ".png"
      options.animation_path = prefixed_path .. ".animation"
    end

    if object.custom_properties.Solid == "true" then
      options.solid = true
    end

    context = clone_table(context)
    context.bot_id = Net.create_bot(options)

    if object.custom_properties.Id then
      local bot_script_id = object.custom_properties.Id
      self:track_bot(context.area_id, context.bot_id, bot_script_id)
    end

    if object.custom_properties["On Interact"] then
      interaction_map[context.bot_id] = object.custom_properties["On Interact"]
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("remove bot", function(context, object)
    local bot_id = context.bot_id

    if object.custom_properties.Id then
      bot_id = self:resolve_bot_id(context, "Bot " .. object.custom_properties.Id) --[[@as Net.ActorId]]
    end

    if bot_id then
      self:emit_bot_remove(bot_id)
      Net.remove_bot(bot_id, object.custom_properties["Warp Out"] == "true")
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("emote", function(context, object)
    local actor_string = object.custom_properties.Actor
    local actor_id = context.bot_id or context.player_id

    if actor_string then
      actor_id = self:resolve_actor_id(context, actor_string)
    end

    local emote_id = object.custom_properties.Emote

    if (not actor_string or actor_string == "Player") and context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        Net.set_player_emote(player_id, emote_id)
      end
    elseif actor_id and Net.is_player(actor_id) then
      Net.set_player_emote(actor_id, emote_id)
    elseif actor_id and Net.is_bot(actor_id) then
      Net.set_bot_emote(actor_id, emote_id)
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("face", function(context, object)
    local actor_string = object.custom_properties.Actor
    local actor_id = context.bot_id or context.player_id

    if actor_string then
      actor_id = self:resolve_actor_id(context, actor_string)
    end

    local direction = object.custom_properties.Direction
    local target_position = nil
    local resolve_direction = Direction.from_points

    if object.custom_properties["Target Diagonal"] == "true" or object.custom_properties["Target Diagonally"] == "true" then
      resolve_direction = Direction.diagonal_from_points
    end

    if not direction then
      local target_actor_id = self:resolve_actor_id(context, object.custom_properties.Target)

      if not target_actor_id then
        target_position = self:resolve_object(context.area_id, object.custom_properties.Target)
      elseif Net.is_player(target_actor_id) then
        target_position = Net.get_player_position(target_actor_id)
      elseif Net.is_bot(target_actor_id) then
        target_position = Net.get_bot_position(target_actor_id)
      end
    end

    if direction or target_position then
      if ((not actor_string and not context.bot_id) or actor_string == "Player") and context.player_ids then
        for _, player_id in ipairs(context.player_ids) do
          if not Net.is_player(player_id) then
            -- avoid error from attempting to read from a disconnected player
            goto continue
          end

          if target_position then
            direction = resolve_direction(Net.get_player_position(player_id), target_position)
          end

          local keyframes = { { properties = { { property = "Direction", value = direction } } } }

          Net.animate_player_properties(player_id, keyframes)

          ::continue::
        end
      elseif actor_id and Net.is_player(actor_id) then
        if target_position then
          direction = resolve_direction(Net.get_player_position(actor_id), target_position)
        end

        local keyframes = { { properties = { { property = "Direction", value = direction } } } }
        Net.animate_player_properties(actor_id, keyframes)
      elseif actor_id and Net.is_bot(actor_id) then
        if target_position then
          direction = resolve_direction(Net.get_bot_position(actor_id), target_position)
        end

        Net.set_bot_direction(actor_id, direction)
      end
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("animate", function(context, object)
    local actor_string = object.custom_properties.Actor
    local actor_id = context.bot_id or context.player_id

    if actor_string then
      actor_id = self:resolve_actor_id(context, actor_string)
    end

    local state = object.custom_properties["Animation State"]
    local loop = object.custom_properties["Loop"] == "true"

    if ((not context.bot_id and not actor_string) or actor_string == "Player") and context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        Net.animate_player(player_id, state, loop)
      end
    elseif actor_id and Net.is_player(actor_id) then
      Net.animate_player(actor_id, state, loop)
    elseif actor_id and Net.is_bot(actor_id) then
      Net.animate_bot(actor_id, state, loop)
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("teleport", function(context, object)
    local actor_string = object.custom_properties.Actor
    local actor_id = context.bot_id or context.player_id

    if actor_string then
      actor_id = self:resolve_actor_id(context, actor_string)
    end

    local x, y, z
    local warp_in
    local direction

    warp_in, x, y, z, direction = self:resolve_teleport_properties(object, context.area_id)

    if not x then
      x, y, z = object.x, object.y, object.z
    end

    if ((not context.bot_id and not actor_string) or actor_string == "Player") and context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        Net.teleport_player(player_id, warp_in, x, y, z, direction)
      end
    elseif actor_id and Net.is_player(actor_id) then
      Net.teleport_player(actor_id, warp_in, x, y, z, direction)
    elseif actor_id and Net.is_bot(actor_id) then
      Net.move_bot(actor_id, x, y, z)

      if direction then
        Net.set_bot_direction(actor_id, direction)
      end
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("lock input", function(context, object)
    if context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        Net.lock_player_input(player_id)
      end
    else
      Net.lock_player_input(context.player_id)
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("unlock input", function(context, object)
    if context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        Net.unlock_player_input(player_id)
      end
    else
      Net.unlock_player_input(context.player_id)
    end

    self:execute_next_node(context, context.area_id, object)
  end)
end

---Implements support for `Tag`, `Untag`, and `Clear Tag` nodes.
---
---Expects `area_id` and optionally `bot_id`, `player_id`, or `player_ids` to be defined on the context table.
---
---Custom properties supported by `Tag`:
--- - `Actor` "Player [1+]" | "Bot [id]" (optional)
--- - `Tag` the tag to give the actor
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Untag`:
--- - `Actor` "Player [1+]" | "Bot [id]" (optional)
--- - `Tag` the tag to remove from the actor
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Clear Tag`:
--- - `Tag` the tag to clear
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_tag_api()
  self:implement_node("tag", function(context, object)
    local tag_group = self:get_tag_actors(object.custom_properties.Tag)
    local actor_string = object.custom_properties.Actor
    local actor_id = context.bot_id or context.player_id

    if actor_string then
      actor_id = self:resolve_actor_id(context, actor_string)
    end

    if (not actor_string or actor_string == "Player") and context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        -- make sure the player is still connected
        if Net.is_player(player_id) then
          tag_group[#tag_group + 1] = player_id
        end
      end
    elseif actor_id and (Net.is_player(actor_id) or Net.is_bot(actor_id)) then
      tag_group[#tag_group + 1] = actor_id
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("untag", function(context, object)
    local tag = object.custom_properties.Tag
    local actor_string = object.custom_properties.Actor
    local actor_id = context.bot_id or context.player_id

    if actor_string then
      actor_id = self:resolve_actor_id(context, actor_string)
    end

    if (not actor_string or actor_string == "Player") and context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        self:untag_actor(player_id, tag)
      end
    elseif actor_id and (Net.is_player(actor_id) or Net.is_bot(actor_id)) then
      self:untag_actor(actor_id, tag)
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("clear tag", function(context, object)
    self._tagged[object.custom_properties.Tag] = nil

    self:execute_next_node(context, context.area_id, object)
  end)

  self:on_server_event("player_disconnect", function(event)
    for tag in pairs(self._tagged) do
      self:untag_actor(event.player_id, tag)
    end
  end)
end

---Implements support for the `Area Transition` node.
---
---Expects `area_id` and optionally `player_id`, or `player_ids` to be defined on the context table.
---
---Custom properties supported by `Area Transition`:
--- - `Fade` a color to fade to (optional)
--- - `Direction` number, the direction players should walk or face during the transition (optional)
--- - `Walk Distance` number, how far players should walk during the transition (optional)
--- - `Next [1]` a link to the next node, executes before the screen fades back in. (optional)
function ScriptNodes:implement_common_animations_api()
  self:implement_node("area transition", function(context, object)
    local FULL_DURATION = 45 / 60
    local HALF_DURATION = FULL_DURATION / 2
    local fade_color = parse_color(object.custom_properties.Fade) or { r = 0, g = 0, b = 0 }

    local direction = object.custom_properties.Direction and string.upper(object.custom_properties.Direction)
    local walk_distance = tonumber(object.custom_properties["Walk Distance"])

    local keyframe_properties = {}

    ---@type Net.ActorKeyframe[]
    local keyframes = {
      {
        properties = keyframe_properties,
        duration = HALF_DURATION,
      }
    }

    if walk_distance then
      keyframe_properties[1] = { property = "X", ease = "Linear", value = 0 }
      keyframe_properties[2] = { property = "Y", ease = "Linear", value = 0 }
    else
      keyframe_properties[1] = { property = "Direction", value = direction }
    end

    -- animate player and fade out
    for_each_player_safe(context, function(player_id)
      if walk_distance then
        local x, y = Net.get_player_position_multi(player_id)
        local offset_x, offset_y = Direction.unit_vector_multi(
          direction or Net.get_player_direction(player_id)
        )

        keyframe_properties[1].value = x + offset_x * walk_distance
        keyframe_properties[2].value = y + offset_y * walk_distance
      end

      Net.animate_player_properties(player_id, keyframes)
      Net.lock_player_input(player_id)
      Net.fade_player_camera(player_id, fade_color, HALF_DURATION)
    end)

    -- execute next node and fade in
    Async.sleep(HALF_DURATION).and_then(function()
      self:execute_next_node(context, context.area_id, object)

      local TRANSPARENT = { r = 0, g = 0, b = 0, a = 0 }

      for_each_player_safe(context, function(player_id)
        Net.fade_player_camera(player_id, TRANSPARENT, HALF_DURATION)
      end)
    end)

    -- transition complete, unlock input
    Async.sleep(FULL_DURATION).and_then(function()
      for_each_player_safe(context, function(player_id)
        Net.unlock_player_input(player_id)
      end)
    end)
  end)
end

---Implements support for `Set Path`, `Paused Path`, `Pause Path`, and `Resume Path` nodes.
---
---Expects `area_id` and optionally `bot_id`, `player_id`, or `player_ids` to be defined on the context table.
---
---Custom properties supported by `Set Path`:
--- - `Actor` "Player [1+]" | "Bot [id]" (optional)
--- - `Path Start` a link to the first path node. (optional)
---
---    Path nodes support optional `Path Next` and `Speed Multiplier` properties.
--- - `Speed Multiplier` number (optional)
--- - `Interrupt Radius` number, if a player enters this radius, movement will be blocked (optional, bot specific)
--- - `Wait` number, how long the actor should wait at this point before continuing movement (optional)
--- - `Loop` boolean (optional, bot specific)
---
---   If the path doesn't contain a loop it will be played in reverse when the path ends.
---   If `Loop` is false or unspecified, paths with a loop will end at the loop.
--- - `Next [1]` a link to the next node, executes immediately if `Loop` is set to true. (optional)
---
---Custom properties supported by `Paused Path`:
--- - `Actor` "Bot [id]" (optional)
--- - `Next [1]` a link to the next node (optional)
--- - `Next 2` a link to the passing node (optional)
---
---Custom properties supported by `Pause Path`:
--- - `Actor` "Bot [id]" (optional)
--- - `Next [1]` a link to the next node (optional)
---
---Custom properties supported by `Unpause Path`:
--- - `Actor` "Bot [id]" (optional)
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_path_api()
  local DEFAULT_SPEED = 6 / 64
  local DEFAULT_TILE_WIDTH = 64

  local bot_paths = BotPaths:new()

  self:implement_node("pause path", function(context, object)
    local actor_string = object.custom_properties.Actor
    local bot_id = context.bot_id

    if actor_string then
      bot_id = self:resolve_bot_id(context, actor_string)
    end

    if bot_id then
      if not bot_paths:bot_initialized(bot_id) then
        bot_paths:init_bot(bot_id)
      end

      local has_players = false

      for_each_player(context, function(player_id)
        bot_paths:pause_path_for(bot_id, player_id)
        has_players = true
      end)

      if not has_players then
        -- treat as if the area is the pauser
        bot_paths:pause_path_for(bot_id, context.area_id)
      end
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("paused path", function(context, object)
    local actor_string = object.custom_properties.Actor
    local bot_id = context.bot_id

    if actor_string then
      bot_id = self:resolve_bot_id(context, actor_string)
    end

    if bot_id and bot_paths:bot_initialized(bot_id) then
      if bot_paths:is_paused(bot_id) then
        self:execute_next_node(context, context.area_id, object, 2)
        return
      end
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("resume path", function(context, object)
    local actor_string = object.custom_properties.Actor
    local bot_id = context.bot_id

    if actor_string then
      bot_id = self:resolve_bot_id(context, actor_string)
    end

    if bot_id then
      if not bot_paths:bot_initialized(bot_id) then
        bot_paths:init_bot(bot_id)
      end

      local has_players = false

      for_each_player(context, function(player_id)
        bot_paths:resume_path_for(bot_id, player_id)
        has_players = true
      end)

      if not has_players then
        -- treat as if the area is the pauser
        bot_paths:resume_path_for(bot_id, context.area_id)
      end
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  local function screen_distance(area_id, x1, y1, z1, x2, y2, z2)
    x1, y1 = Net.world_to_screen_multi(area_id, x1, y1, z1)
    x2, y2 = Net.world_to_screen_multi(area_id, x2, y2, z2)

    local diff_x = x2 - x1
    local diff_y = y2 - y1

    return math.sqrt(diff_x * diff_x + diff_y * diff_y)
  end

  local function build_player_keyframes(area_id, path)
    local keyframes = {}
    local total_duration = 0

    local first_node = path[1]
    keyframes[1] = {
      properties = {
        { property = "X", ease = "Linear", value = first_node.x },
        { property = "Y", ease = "Linear", value = first_node.y },
        { property = "Z", ease = "Linear", value = first_node.z },
      }
    }

    if first_node.wait then
      total_duration = total_duration + first_node.wait
      keyframes[2] = {
        properties = keyframes[1].properties,
        duration = first_node.wait
      }
    end

    for i = 2, #path do
      local prev_node = path[i - 1]
      local path_node = path[i]

      local dist = screen_distance(
        area_id,
        prev_node.x, prev_node.y, prev_node.z,
        path_node.x, path_node.y, path_node.z
      )

      local duration = dist / (prev_node.speed * 20)
      total_duration = total_duration + duration

      local properties = {
        { property = "X", ease = "Linear", value = path_node.x },
        { property = "Y", ease = "Linear", value = path_node.y },
        { property = "Z", ease = "Linear", value = path_node.z },
      }

      keyframes[#keyframes + 1] = {
        properties = properties,
        duration = duration,
      }

      if path_node.wait then
        total_duration = total_duration + path_node.wait
        keyframes[#keyframes + 1] = {
          properties = properties,
          duration = path_node.wait
        }
      end
    end

    return keyframes, total_duration
  end

  local function execute_player_path(context, player_id, path, speed, keyframes)
    -- calculate the duration needed to reach the first point
    local first_node = path[1]
    local dist = screen_distance(
      context.area_id,
      first_node.x, first_node.y, first_node.z,
      Net.get_player_position_multi(player_id)
    )
    local duration = dist / (speed * 20)

    keyframes[1].duration = duration
    Net.animate_player_properties(player_id, keyframes)

    return duration
  end

  self:implement_node("set path", function(context, object)
    local actor_string = object.custom_properties.Actor
    local actor_id = context.bot_id or context.player_id

    if actor_string then
      actor_id = self:resolve_actor_id(context, actor_string)
    end

    local visited = {}

    local next_id = object.custom_properties["Path Start"]
    local tile_h_scale = DEFAULT_TILE_WIDTH / Net.get_tile_width(context.area_id)
    local base_speed = tile_h_scale * DEFAULT_SPEED * (tonumber(object.custom_properties["Speed Multiplier"]) or 1)

    local path = {}

    while next_id ~= nil and not visited[next_id] do
      local path_object = self:resolve_object(context.area_id, next_id)

      if not path_object then
        break
      end

      local path_node = {
        id = next_id,
        x = path_object.x,
        y = path_object.y,
        z = path_object.z,
        speed = base_speed * (tonumber(path_object.custom_properties["Speed Multiplier"]) or 1),
        wait = tonumber(path_object.custom_properties["Wait"]),
      }

      if path[#path] then
        path[#path].next = #path + 1
      end

      path[#path + 1] = path_node

      visited[next_id] = true
      next_id = path_object.custom_properties["Path Next"]
    end

    if #path == 0 then
      -- empty path, clear data and avoid building new paths
      if actor_id and Net.is_bot(actor_id) then
        bot_paths:set_path(actor_id, {})
      end

      self:execute_next_node(context, context.area_id, object)
      return
    end

    if actor_id and Net.is_bot(actor_id) then
      if object.custom_properties.Loop == "true" then
        if next_id and visited[next_id] then
          -- resolve the next index for the last node
          for i, path_node in ipairs(path) do
            if path_node.id == next_id then
              path[#path].next = i
              break
            end
          end
        elseif #path > 1 then
          -- bounce the path by appending reversed nodes
          for i = #path - 1, 2, -1 do
            local template = path[i]

            path[#path].next = #path + 1
            path[#path + 1] = {
              x = template.x,
              y = template.y,
              z = template.z,
              speed = path[i - 1].speed,
              wait = template.wait,
            }
          end

          path[#path].next = 1
        end
      else
        path[#path].callback = function()
          self:execute_next_node(context, context.area_id, object)
        end
      end

      local interrupt_radius = tonumber(object.custom_properties["Interrupt Radius"])

      if not bot_paths:bot_initialized(actor_id) then
        bot_paths:init_bot(actor_id)
      end

      bot_paths:set_speed(actor_id, base_speed)
      bot_paths:set_path(actor_id, path)
      bot_paths:set_interrupt_radius(actor_id, interrupt_radius or bot_paths.DEFAULT_INTERRUPT_RADIUS)
      bot_paths:resume_path(actor_id)

      if object.custom_properties.Loop ~= "true" then
        self:execute_next_node(context, context.area_id, object)
      end
    elseif (not actor_string or actor_string == "Player") and context.player_ids then
      local keyframes, total_duration = build_player_keyframes(context.area_id, path)

      for_each_player_safe(context, function(player_id)
        local added_duration = execute_player_path(context, player_id, path, base_speed, keyframes)
        total_duration = math.max(total_duration, total_duration + added_duration)
      end)

      Async.sleep(total_duration).and_then(function()
        self:execute_next_node(context, context.area_id, object)
      end)
    elseif actor_id and Net.is_player(actor_id) then
      local keyframes, total_duration = build_player_keyframes(context.area_id, path)
      local added_duration = execute_player_path(context, actor_id, path, base_speed, keyframes)
      total_duration = total_duration + added_duration

      Async.sleep(total_duration).and_then(function()
        self:execute_next_node(context, context.area_id, object)
      end)
    end
  end)

  self:on_destroy(function()
    bot_paths:destroy()
  end)

  self:on_bot_removed(function(bot_id)
    bot_paths:drop_bot(bot_id)
  end)
end

---Implements support for the `Collision` entry node, and the `Set Collider` script node.
---
---As a reminder: entry nodes have a type starting with `Script Entry: ` such as `Script Entry: Collision`
---
---Custom properties supported by `Collision`:
--- - `On Enter` a link to a script node (optional)
--- - `On Exit` a link to a script node (optional)
--- - `Ignore Transfer` boolean, whether collision changes should be detected when players transfer areas (optional)
---
---Custom properties supported by `Set Collider`:
--- - `Target` "Player [1+]" | "Bot [id]" (optional)
--- - `On Enter` a link to a script node (optional)
--- - `On Exit` a link to a script node (optional)
--- - `Radius` number (optional)
--- - `Ignore Transfer` boolean, whether collision changes should be detected when players transfer areas (optional)
function ScriptNodes:implement_collision_api()
  local object_colliders = ObjectColliders:new()

  self:on_entry_node_load("collision", function(area_id, object)
    local enter_id = tonumber(object.custom_properties["On Enter"])
    local exit_id = tonumber(object.custom_properties["On Exit"])

    object_colliders:register_collider({
      object_id = object.id,
      area_id = area_id,
      z = object.z,
      on_enter = enter_id and function(player_id)
        local context = { area_id = area_id, player_id = player_id }
        self:execute_by_id(context, area_id, enter_id)
      end,
      on_exit = exit_id and function(player_id)
        local context = { area_id = area_id, player_id = player_id }
        self:execute_by_id(context, area_id, exit_id)
      end,
      ignore_transfer = object.custom_properties["Ignore Transfer"] == "true",
    })
  end)

  self:implement_node("set collider", function(context, object)
    local actor_id = context.bot_id or context.player_id
    local target_string = object.custom_properties.Target

    if object.custom_properties.Target then
      actor_id = self:resolve_actor_id(context, object.custom_properties.Target)
    end

    local area_id = context.area_id
    local radius = tonumber(object.custom_properties.Radius)
    local diameter = radius and radius * 2
    local object_options = {
      width = diameter,
      height = diameter,
      privacy = true,
      data = {
        type = "ellipse"
      },
    }
    local collider_options = {
      area_id = area_id,
      z = object.z,
      ignore_transfer = object.custom_properties["Ignore Transfer"] == "true",
    }

    local enter_id = tonumber(object.custom_properties["On Enter"])
    local exit_id = tonumber(object.custom_properties["On Exit"])

    local function set_collider_for_player(player_id)
      object_colliders:remove_all_on_actor(player_id)

      if not radius then
        return
      end

      local x, y, z = Net.get_player_position_multi(player_id)
      object_options.x = x - radius
      object_options.y = y - radius
      object_options.z = z
      collider_options.object_id = Net.create_object(area_id, object_options)

      collider_options.on_enter = enter_id and function(other_player_id)
        local context = { area_id = area_id, player_ids = { player_id, other_player_id } }
        self:execute_by_id(context, area_id, enter_id)
      end
      collider_options.on_exit = exit_id and function(other_player_id)
        local context = { area_id = area_id, player_ids = { player_id, other_player_id } }
        self:execute_by_id(context, area_id, exit_id)
      end

      object_colliders:register_collider(collider_options)
      object_colliders:attach_to_actor(area_id, collider_options.object_id, player_id, -radius, -radius)
    end

    if (not target_string or target_string == "Player") and context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        set_collider_for_player(player_id)
      end
    elseif actor_id and Net.is_player(actor_id) then
      set_collider_for_player(actor_id)
    elseif actor_id and Net.is_bot(actor_id) then
      object_colliders:remove_all_on_actor(actor_id)

      if radius then
        local x, y, z = Net.get_bot_position_multi(actor_id)
        object_options.x = x - radius
        object_options.y = y - radius
        object_options.z = z
        collider_options.object_id = Net.create_object(area_id, object_options)

        collider_options.on_enter = enter_id and function(player_id)
          local context = { area_id = area_id, bot_id = actor_id, player_id = player_id }
          self:execute_by_id(context, area_id, enter_id)
        end
        collider_options.on_exit = exit_id and function(player_id)
          local context = { area_id = area_id, bot_id = actor_id, player_id = player_id }
          self:execute_by_id(context, area_id, exit_id)
        end

        object_colliders:register_collider(collider_options)
        object_colliders:attach_to_actor(area_id, collider_options.object_id, actor_id, -radius, -radius)
      end
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:on_unload(function(area_id)
    object_colliders:drop_colliders_for_area(area_id)
  end)

  self:on_bot_removed(function(bot_id)
    object_colliders:remove_all_on_actor(bot_id)
    object_colliders:drop_actor(bot_id)
  end)

  self:on_server_event("player_disconnect", function(event)
    object_colliders:remove_all_on_actor(event.actor_id)
    object_colliders:drop_actor(event.actor_id)
  end)

  self:on_destroy(function()
    object_colliders:destroy()
  end)
end

---Implements support for the `Delay` node.
---
---Expects `area_id` to be defined on the context table.
---
---Supported custom properties:
--- - `Duration` the duration of the delay in seconds (optional)
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_delay_api()
  self:implement_node("delay", function(context, object)
    Async.sleep(tonumber(object.custom_properties.Duration) or 0).and_then(function()
      self:execute_next_node(context, context.area_id, object)
    end)
  end)
end

---Implements support for the `Random` node. This node will select a random next node.
---
---Expects `area_id` to be defined on the context table.
---
---Supported custom properties:
--- - `Weight [1+]` a weight for the next node (optional)
--- - `Next [1+]` a link to the next node (optional)
function ScriptNodes:implement_random_api()
  self:implement_node("random", function(context, object)
    -- resolve next_ids and weights
    local next_ids = {}
    local weights = {}
    local max_weight = 0

    local next_id = object.custom_properties["Next"] or object.custom_properties["Next 1"]

    if not next_id then
      return
    end

    next_ids[1] = next_id
    weights[1] = tonumber(object.custom_properties["Weight 1"]) or 1
    max_weight = weights[1]

    local i = 2

    while true do
      next_id = object.custom_properties["Next " .. i]

      if not next_id then
        break
      end

      next_ids[i] = next_id
      weights[i] = tonumber(object.custom_properties["Weight " .. i]) or 1
      max_weight = max_weight + weights[i]
      i = i + 1
    end

    -- roll a random number and figure out where we land
    local roll = math.random() * max_weight

    for i, weight in ipairs(weights) do
      roll = roll - weight

      if roll < 0 then
        self:execute_by_id(context, context.area_id, next_ids[i])
        return
      end
    end

    -- fallback to the first node
    self:execute_by_id(context, context.area_id, next_ids[1])
  end)
end

---Implements support for the `Thread` node. This node will execute every supplied next node.
---
---Expects `area_id` to be defined on the context table.
---
---Supported custom properties:
--- - `Next [1+]` a link to the next node (optional)
function ScriptNodes:implement_thread_api()
  self:implement_node("thread", function(context, object)
    local next_id = object.custom_properties["Next 1"] or object.custom_properties["Next"]

    if not next_id then
      return
    end

    self:execute_by_id(context, context.area_id, next_id)

    local i = 2

    while true do
      next_id = object.custom_properties["Next " .. i]

      if not next_id then
        break
      end

      self:execute_by_id(context, context.area_id, next_id)
      i = i + 1
    end
  end)
end

---Implements support for the `Party All`, `Party Loaded`, `Party Instance`,
--- `Party Area`, `Party Near`, `Party Inside`, `Party Tag`, `Clear Party`, `Disband Party`, `Reunite Party`,
--- `Shuffle Party`, `Split Party`, and `Require Party Size` nodes.
---
---### `Party All`, `Party Loaded`, `Party Instance`, `Party Area`
---
---Expects `area_id` to be defined on the context table.
---
---Clears `player_id` and sets `player_ids` on the context.
---
---Supported custom properties:
--- - `Next [1]` a link to the next node (optional)
---
---### `Party Near`
---
---Expects `area_id` to be defined on the context table.
---
---Clears `player_id` and sets `player_ids` on the context.
---
---Supported custom properties for `Party Near`:
--- - `Target` "Player [1+]" | "Bot [id]" | object (optional)
--- - `Radius` number
--- - `Next [1]` a link to the next node (optional)
---
---### `Party Inside`
---
---Expects `area_id` to be defined on the context table.
---
---Clears `player_id` and sets `player_ids` on the context.
---
---Supported custom properties for `Party Inside`:
--- - `Target` the object to build the party from (optional)
--- - `Next [1]` a link to the next node (optional)
---
---### `Party Tag`
---
---Expects `area_id` to be defined on the context table.
---
---Clears `player_id` and sets `player_ids` on the context.
---
---Supported custom properties for `Party Tag`:
--- - `Tag` the tag to build the party from
--- - `Next [1]` a link to the next node (optional)
---
---### `Clear Party`
---
---Expects `area_id` to be defined on the context table.
---
---Removes `player_ids` and `player_id` from the context table.
---
---Supported custom properties for `Clear Party`:
--- - `Next [1+]` a link to the next node for each player (optional)
---
---### `Disband Party`
---
---Expects `area_id` and `player_ids` to be defined on the context table.
---
---Splits `player_ids` in the context to multiple contexts with `player_id` and `disbanded_party`.
---
---Supported custom properties for `Disband Party`:
--- - `Next [1+]` a link to the next node for each player (optional)
---
---### `Reunite Party`
---
---Expects `area_id` and `disbanded_party` to be defined on the context table.
---
---Clears `player_id` and renames `disbanded_party` to `player_ids` on the context.
---
---Supported custom properties for `Reunite Party`:
--- - `Next [1]` a link to the next node (optional)
---
---### `Exclude Busy`
---
---Expects `area_id` and `player_id` or `player_ids` to be defined on the context table.
---
---Filters out busy players from the party.
---
---Supported custom properties for `Exclude Busy`:
--- - `Next [1]` a link to the next node (optional)
---
---### `Shuffle Party`
---
---Expects `area_id` and optionally `player_id` or `player_ids` to be defined on the context table.
---
---Supported custom properties for `Shuffle Party`:
--- - `Start` integer, every player at and after the matching index will be shuffled (optional)
--- - `Next [1]` a link to the next node (optional)
---
---### `Split Party`
---
---Expects `area_id` and optionally `player_id` or `player_ids` to be defined on the context table.
---
---Attempts to split the party in two.
---
---Supported custom properties for `Split Party`:
--- - `Start` integer, the players at and after the matching index will be moved to a second party (optional)
--- - `Start Fraction` number, splits the party using a value between `0` and `1`
---    to decide what fraction of the party should be in the first party (an alternative to `Start`, optional)
--- - `Next [1]` a link to the default node (optional)
--- - `Next 2` a link to the next node for the second party (optional)
---
---### `Require Party Size`
---
---Expects `area_id` and optionally `player_id` or `player_ids` to be defined on the context table.
---
---Supported custom properties for `Require Party Size`:
--- - `Minimum` integer (optional)
--- - `Min` integer (alias for `Minimum`, optional)
--- - `Maximum` integer (optional)
--- - `Max` integer (alias for `Maximum`, optional)
--- - `Next [1]` a link to the default node (optional)
--- - `Next 2` a link to the passing node (optional)
function ScriptNodes:implement_party_api()
  local function party_prep(context)
    local player_ids = context.player_ids or { context.player_id }
    local exists_map = {}

    for _, player_id in ipairs(player_ids) do
      exists_map[player_id] = true
    end

    return player_ids, exists_map
  end

  local function add_list_to_party(player_ids, exists_map, add_list)
    for _, player_id in ipairs(add_list) do
      if not exists_map[player_id] then
        player_ids[#player_ids + 1] = player_id
      end
    end
  end

  self:implement_node("party all", function(context, object)
    local player_ids, exists_map = party_prep(context)

    for _, area_id in ipairs(Net.list_areas()) do
      local add_list = Net.list_players(area_id)
      add_list_to_party(player_ids, exists_map, add_list)
    end

    context = clone_table(context)
    context.player_id = nil
    context.player_ids = player_ids

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("party loaded", function(context, object)
    local player_ids, exists_map = party_prep(context)

    for area_id in pairs(self._loaded_areas) do
      local add_list = Net.list_players(area_id)
      add_list_to_party(player_ids, exists_map, add_list)
    end

    context = clone_table(context)
    context.player_id = nil
    context.player_ids = player_ids

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("party instance", function(context, object)
    local player_ids, exists_map = party_prep(context)

    local instancer = self:instancer()
    local instance_id = instancer:resolve_instance_id(context.area_id)
    local add_list = instance_id and instancer:instance_player_list(instance_id)

    if add_list then
      add_list_to_party(player_ids, exists_map, add_list)
    else
      -- fallback to party all, consider it the primary instance
      for _, area_id in ipairs(Net.list_areas()) do
        add_list = Net.list_players(area_id)
        add_list_to_party(player_ids, exists_map, add_list)
      end
    end

    context = clone_table(context)
    context.player_id = nil
    context.player_ids = player_ids

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("party area", function(context, object)
    local player_ids, exists_map = party_prep(context)
    local add_list = Net.list_players(context.area_id)
    add_list_to_party(player_ids, exists_map, add_list)

    context = clone_table(context)
    context.player_id = nil
    context.player_ids = player_ids

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("party near", function(context, object)
    local player_ids, exists_map = party_prep(context)
    local radius = tonumber(object.custom_properties.Radius)
    ---@type Net.ActorId
    local target_id = context.bot_id or context.player_id or context.object_id
    local target_string = object.custom_properties.Target

    if object.custom_properties.Target then
      target_id = self:resolve_actor_id(context, target_string) or tonumber(target_string) --[[@as Net.ActorId]]
    end

    local radius_sqr = radius * radius
    local center_x, center_y, center_z

    if target_id and Net.is_player(target_id) then
      center_x, center_y, center_z = Net.get_player_position_multi(target_id)
    elseif target_id and Net.is_bot(target_id) then
      center_x, center_y, center_z = Net.get_player_position_multi(target_id)
    else
      local target_object = self:resolve_object(context.area_id, target_id --[[@as number]])

      if target_object then
        center_x = target_object.x
        center_y = target_object.y
        center_z = target_object.z
      else
        radius_sqr = 0
      end
    end

    if radius_sqr > 0 then
      for _, player_id in ipairs(Net.list_players(context.area_id)) do
        if exists_map[player_id] then
          goto continue
        end

        local x, y, z = Net.get_player_position_multi(player_id)
        local x_diff = x - center_x
        local y_diff = y - center_y
        local z_diff = z - center_z

        if x_diff * x_diff + y_diff * y_diff + z_diff * z_diff <= radius_sqr then
          player_ids[#player_ids + 1] = player_id
        end

        ::continue::
      end
    end

    context = clone_table(context)
    context.player_id = nil
    context.player_ids = player_ids

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("party inside", function(context, object)
    local player_ids, exists_map = party_prep(context)
    local object_id = tonumber(object.custom_properties.Target) or context.object_id

    for _, player_id in ipairs(Net.list_players(context.area_id)) do
      if exists_map[player_id] then
        goto continue
      end

      local x, y, z = Net.get_player_position_multi(player_id)

      if object.z ~= z then
        goto continue
      end

      if Net.is_inside_object(context.area_id, object_id, x, y) then
        player_ids[#player_ids + 1] = player_id
      end

      ::continue::
    end

    context = clone_table(context)
    context.player_id = nil
    context.player_ids = player_ids

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("party tag", function(context, object)
    local tag = object.custom_properties.Tag
    local player_ids, exists_map = party_prep(context)

    for _, actor_id in ipairs(self:get_tag_actors(tag)) do
      if not exists_map[actor_id] and Net.is_player(actor_id) then
        player_ids[#player_ids + 1] = actor_id
      end
    end

    context = clone_table(context)
    context.player_id = nil
    context.player_ids = player_ids

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("clear party", function(context, object)
    context = clone_table(context)
    context.player_id = nil
    context.player_ids = nil

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("disband party", function(party_context, object)
    for i, player_id in ipairs(party_context.player_ids) do
      local next = self:resolve_next_node(party_context.area_id, object, i)

      if next then
        local context = clone_table(party_context)
        context.player_id = player_id
        context.disbanded_party = context.player_ids
        context.player_ids = nil

        self:execute_node(context, next)
      end
    end
  end)

  self:implement_node("reunite party", function(context, object)
    context = clone_table(context)
    context.player_id = nil
    context.player_ids = context.disbanded_party
    context.disbanded_party = nil

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("exclude busy", function(context, object)
    if context.player_ids then
      local player_ids = {}

      for _, player_id in ipairs(context.player_ids) do
        if not Net.is_player_busy(player_id) then
          player_ids[#player_ids + 1] = player_id
        end
      end

      context = clone_table(context)
      context.player_ids = player_ids
    elseif context.player_id and Net.is_player_busy(context.player_id) then
      context = clone_table(context)
      context.player_id = nil
      context.player_ids = {}
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  -- modified to shuffle everything at or after `start`
  -- https://stackoverflow.com/questions/35572435/how-do-you-do-the-fisher-yates-shuffle-in-lua
  local function shuffle(t, start)
    local s = {}
    for i = 1, #t do s[i] = t[i] end
    for i = #t, 1 + start, -1 do
      local j = math.random(start, i)
      s[i], s[j] = s[j], s[i]
    end
    return s
  end

  self:implement_node("shuffle party", function(context, object)
    if context.player_ids then
      local start = tonumber(object.custom_properties.Start) or 1

      context = clone_table(context)
      context.player_ids = shuffle(context.player_ids, start)
    end

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("split party", function(context, object)
    if not context.player_ids then
      self:execute_next_node(context, context.area_id, object)
      return
    end

    local a = {}
    local b = {}

    local start = tonumber(object.custom_properties.Start)

    if not start then
      start = tonumber(object.custom_properties["Start Fraction"])

      if start then
        start = math.max(math.floor(start * #context.player_ids), 1)
      else
        start = #context.player_ids // 2
      end
    end

    start = math.max(start, 1)

    for i, player_id in ipairs(context.player_ids) do
      if i < start then
        a[i] = player_id
      else
        b[#b + 1] = player_id
      end
    end

    local context_a = clone_table(context)
    context_a.player_ids = a
    self:execute_next_node(context_a, context.area_id, object)

    local context_b = clone_table(context)
    context_b.player_ids = a
    self:execute_next_node(context_b, context.area_id, object, 2)
  end)

  self:implement_node("require party size", function(context, object)
    local count = 0

    if context.player_ids then
      for _, player_id in ipairs(context.player_ids) do
        if Net.is_player(player_id) then
          count = count + 1
        end
      end
    elseif context.player_id then
      count = 1
    end

    local min = tonumber(object.custom_properties.Minimum or object.custom_properties.Min)
    local max = tonumber(object.custom_properties.Maximum or object.custom_properties.Max)

    local pass_min = not min or count >= min
    local pass_max = not max or count <= max

    if pass_min and pass_max then
      self:execute_next_node(context, context.area_id, object, 2)
    else
      self:execute_next_node(context, context.area_id, object)
    end
  end)
end

---Implements support for the `Set Variable`, `Increment Variable`, `Require Variable Value`,
---`Push Variable Scope`, `Pop Variable Scope`, `Push Function`, `Pop Function`,  and `Print Variables` nodes.
---
---Variable names have the following format: `[scope: ]Name`. (Example `Local: X`, `Global: X`, or `X`)
---
---Supported scopes:
--- - `Local` tied to the current context, used for temporary variables.
--- - `Up` tied to the current context, allows access to the previous local scope after pushing a scope.
--- - `Self` tied to an object, bot, or player.
--- - `Area` tied to the area.
--- - `Instance` tied to the instance or global scope when outside of an instance (the default)
--- - `Global`
---
---Note: Unless a function or variable scope is pushed, local variables will be shared between threads
---when `Thread` nodes are used. Variables stored in `Up` after a scope is pushed will still be shared between threads.
---
---Supported custom properties for `Set Variable`:
--- - `Variable` string
--- - `Target` "Player [1+]" | "Bot [id]" | object, allows specification for the `Self` scope (optional)
--- - `Value` number | string
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Increment Variable`:
--- - `Variable` string
--- - `Target` "Player [1+]" | "Bot [id]" | object, allows specification for the `Self` scope (optional)
--- - `Value` number (optional)
--- - `Amount` number (alias for `Value`, optional)
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Decrement Variable`:
--- - `Variable` string
--- - `Target` "Player [1+]" | "Bot [id]" | object, allows specification for the `Self` scope (optional)
--- - `Value` number (optional)
--- - `Amount` number (alias for `Value`, optional)
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Require Variable Value`:
--- - `Variable` string
--- - `Target` "Player [1+]" | "Bot [id]" | object, allows specification for the `Self` scope (optional)
--- - `Minimum` number (optional)
--- - `Min` number (alias for `Minimum`, optional)
--- - `Maximum` number (optional)
--- - `Max` number (alias for `Maximum`, optional)
--- - `Value` number | string (optional)
--- - `Next [1]` a link to the default node (optional)
--- - `Next 2` a link to the passing node (optional)
---
---Supported custom properties for `Push Variable Scope`:
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Pop Variable Scope`:
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Push Function`:
--- - `Function` a link to a script node
--- - `Next [1]` a link to node to run after the function script node finishes (optional)
---
---   Note: the next node will never run if `Pop Function` is never called.
---
---Automatically pushes a variable scope.
---
---Supported custom properties for `Pop Function`:
--- - None
---
---Automatically pops a variable scope.
---
---Supported custom properties for `Print Variables`:
--- - `Scope` string (optional)
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_variable_api()
  local function resolve_self_context(context, object)
    local target = object.custom_properties.Target

    if not target then
      return context
    end

    local original_context = context

    context = { variable_scopes = original_context.variable_scopes }

    if target == "Player" then
      context.player_ids = original_context.player_ids
      context.player_id = original_context.player_id
    else
      context.player_id = self:resolve_player_id(original_context, target)
      context.bot_id = self:resolve_bot_id(original_context, target)
      context.object_id = target
    end

    return context
  end

  local function for_each_self_scope(context, object, callback)
    local targetting_party = context.player_ids
        and (
          (not context.bot_id and not context.object_id)
          or (object.custom_properties.Target and object.custom_properties.Target == "Player")
        )

    if not targetting_party then
      local self_context = resolve_self_context(context, object)
      callback(self_context)
    else
      local self_context = {
        variable_scopes = context.variable_scopes
      }

      for _, player_id in ipairs(context.player_ids) do
        self_context.player_id = player_id
        callback(self_context)
      end
    end
  end

  self:implement_node("set variable", function(context, object)
    local self_context = resolve_self_context(context, object)

    local value = tonumber(object.custom_properties.Value) or object.custom_properties.Value
    local variable = object.custom_properties.Variable
    self:variables():set_variable(self_context, variable, value)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("increment variable", function(context, object)
    local variables = self:variables()

    local variable = object.custom_properties.Variable
    local amount = tonumber(object.custom_properties.Value) or tonumber(object.custom_properties.Amount) or 1

    for_each_self_scope(context, object, function(self_context)
      local value = (variables:resolve_variable(self_context, variable) or 0) + amount
      variables:set_variable(self_context, variable, value)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("decrement variable", function(context, object)
    local variables = self:variables()

    local variable = object.custom_properties.Variable
    local amount = tonumber(object.custom_properties.Value) or tonumber(object.custom_properties.Amount) or 1

    for_each_self_scope(context, object, function(self_context)
      local value = (variables:resolve_variable(self_context, variable) or 0) - amount
      variables:set_variable(self_context, variable, value)
    end)

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("require variable value", function(context, object)
    local variable = object.custom_properties.Variable
    local required_value = object.custom_properties.Value
    local min = tonumber(object.custom_properties.Minimum or object.custom_properties.Min)
    local max = tonumber(object.custom_properties.Maximum or object.custom_properties.Max)

    local pass = true
    local at_least_one = false

    for_each_self_scope(context, object, function(self_context)
      local value = self:variables():resolve_variable(self_context, variable) or 0
      at_least_one = true

      if required_value then
        pass = pass and (required_value == value or tonumber(required_value) == value)
      else
        local pass_min = not min or value >= min
        local pass_max = not max or value <= max

        pass = pass and pass_min and pass_max
      end
    end)

    if pass and at_least_one then
      self:execute_next_node(context, context.area_id, object, 2)
    else
      self:execute_next_node(context, context.area_id, object)
    end
  end)

  local function clone_context_and_scope_list(context)
    context = clone_table(context)

    if context.variable_scopes then
      context.variable_scopes = clone_table(context.variable_scopes)
    end

    return context
  end

  self:implement_node("push variable scope", function(context, object)
    context = clone_context_and_scope_list(context)
    self:variables():push_variable_scope(context)
    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("pop variable scope", function(context, object)
    context = clone_context_and_scope_list(context)
    self:variables():pop_variable_scope(context)
    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("push function", function(context, object)
    context = clone_context_and_scope_list(context)

    if context.function_return_list then
      context.function_return_list = clone_table(context.function_return_list)
    else
      context.function_return_list = {}
    end

    context.function_return_list[#context.function_return_list + 1] = self:resolve_next_id(object)

    self:variables():push_variable_scope(context)

    self:execute_by_id(context, context.area_id, object.custom_properties.Function)
  end)

  self:implement_node("pop function", function(context)
    context = clone_context_and_scope_list(context)

    if not context.function_return_list then
      return
    end

    context.function_return_list = clone_table(context.function_return_list)
    self:variables():pop_variable_scope(context)

    local i = #context.function_return_list
    local next_id = context.function_return_list[i]
    context.function_return_list[i] = nil

    if next_id then
      self:execute_by_id(context, context.area_id, next_id)
    end
  end)

  self:implement_node("print variables", function(context, object)
    local scope_name = object.custom_properties.Scope
    local variables = self:variables()
    local scope_callbacks = {
      Local = function()
        print(context.variable_scopes[#context.variable_scopes])
      end,
      Up = function()
        print(context.variable_scopes[#context.variable_scopes - 1])
      end,
      Self = function()
        local self_list = {}

        for_each_self_scope(context, object, function(self_context)
          self_list[#self_list + 1] = variables:self_variables(self_context)
        end)

        if #self_list == 1 then
          print(self_list[1])
        else
          print(self_list)
        end
      end,
      Area = function()
        print(variables:area_variables(context.area_id))
      end,
      Instance = function()
        local instance_id = Instancer:resolve_instance_id(context.area_id)

        if instance_id then
          print(variables:instance_variables(instance_id))
        else
          print(variables:global_variables())
        end
      end,
      Global = function()
        print(variables:global_variables())
      end
    }

    if not scope_name or scope_name == "" then
      scope_name = variables.DEFAULT_SCOPE
    end

    scope_callbacks[scope_name]()

    self:execute_next_node(context, context.area_id, object)
  end)
end

---Implements support for the `Print` and `Print Context` nodes.
---
---Supported custom properties for `Print`:
--- - `Text [1]` the message to print
--- - `Next [1]` a link to the next node (optional)
---
---Supported custom properties for `Print Context`:
--- - `Next [1]` a link to the next node (optional)
function ScriptNodes:implement_debug_api()
  self:implement_node("print", function(context, object)
    print(object.custom_properties.Text or object.custom_properties["Text 1"])

    self:execute_next_node(context, context.area_id, object)
  end)

  self:implement_node("print context", function(context, object)
    print(context)

    self:execute_next_node(context, context.area_id, object)
  end)
end

return ScriptNodes
