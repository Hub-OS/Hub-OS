---@class BotPaths.PathNode
---@field x number
---@field y number
---@field z number
---@field next number? index to the next node
---@field id string?
---@field speed number?
---@field wait number
---@field callback fun()?

---@class BotPaths._BotData
---@field path BotPaths.PathNode[]
---@field path_index number?
---@field speed number
---@field wait number
---@field interrupt_radius number
---@field pause_count number
---@field paused_by table

---@class BotPaths
---@field private _bots table<Net.ActorId, BotPaths._BotData>
---@field private _tick_listener fun()
---@field private _disconnect_listener fun()
local BotPaths = {
  DEFAULT_SPEED = 6 / 64,
  DEFAULT_INTERRUPT_RADIUS = 0.3
}

---@return BotPaths
function BotPaths:new()
  ---@type table<Net.ActorId, BotPaths._BotData>
  local bots = {}

  local tick_listener = function()
    local pending_removal = {}

    for id, bot in pairs(bots) do
      if not Net.is_bot(id) then
        pending_removal[#pending_removal + 1] = id
        goto continue
      end

      if not bot.path_index or bot.pause_count > 0 then
        goto continue
      end

      if bot.wait > 0 then
        bot.wait = bot.wait - 1 / 20
        goto continue
      end

      local path_node = bot.path[bot.path_index]

      if not path_node then
        goto continue
      end

      local area_id = Net.get_bot_area(id)
      local x, y, z = Net.get_bot_position_multi(id)

      -- todo: use some type of spatial map
      if bot.interrupt_radius > 0 then
        local radius_sqr = bot.interrupt_radius * bot.interrupt_radius

        -- see if a player is in the way
        for _, player_id in ipairs(Net.list_players(area_id)) do
          local player_x, player_y, player_z = Net.get_player_position_multi(player_id)
          local player_diff_x = player_x - x
          local player_diff_y = player_y - y
          local player_diff_z = player_z - z
          local player_sqr_dist =
              player_diff_x * player_diff_x +
              player_diff_y * player_diff_y +
              player_diff_z * player_diff_z

          if player_sqr_dist < radius_sqr then
            -- block movement
            goto continue
          end
        end
      end

      local diff_x = path_node.x - x
      local diff_y = path_node.y - y
      local diff_z = path_node.z - z
      local speed = bot.speed

      local movement_x, movement_y, movement_z

      if diff_z < 0 then
        movement_z = -1
      elseif diff_z > 0 then
        movement_z = 1
      else
        movement_z = 0
      end

      diff_x, diff_y = Net.world_to_screen_multi(area_id, diff_x, diff_y)
      local diff_magnitude = math.sqrt(diff_x * diff_x + diff_y * diff_y)
      movement_x = diff_x / diff_magnitude
      movement_y = diff_y / diff_magnitude
      movement_x, movement_y = Net.screen_to_world_multi(area_id, movement_x, movement_y)

      x = x + movement_x * speed
      y = y + movement_y * speed
      z = z + movement_z * speed

      if diff_x * diff_x + diff_y * diff_y + diff_z * diff_z < speed * speed * 2 then
        -- reached point, snap to it, and pick next target
        bot.path_index = path_node.next
        bot.speed = path_node.speed or bot.speed
        bot.wait = path_node.wait or 0
        x = path_node.x
        y = path_node.y
        z = path_node.z

        if path_node.callback then
          path_node.callback()
        end
      end

      Net.move_bot(id, x, y, z)

      ::continue::
    end

    for i = #pending_removal, 1, -1 do
      bots[pending_removal[i]] = nil
    end
  end

  Net:on("tick", tick_listener)

  local disconnect_listener = function(event)
    for _, bot in pairs(bots) do
      if bot.paused_by[event.player_id] then
        bot.paused_by[event.player_id] = nil
        bot.pause_count = bot.pause_count - 1
      end
    end
  end

  Net:on("player_disconnect", disconnect_listener)

  local bot_paths = {
    _bots = bots,
    _tick_listener = tick_listener,
    _disconnect_listener = disconnect_listener
  }
  setmetatable(bot_paths, self)
  self.__index = self

  return bot_paths
end

---@class BotPaths.BotInitOptions
---@field path BotPaths.PathNode[]?
---@field speed number?
---@field interrupt_radius number?

---@param bot_id Net.ActorId
---@param options BotPaths.BotInitOptions?
function BotPaths:init_bot(bot_id, options)
  if not options then
    options = {}
  end

  self._bots[bot_id] = {
    path = options.path or {},
    path_index = 1,
    speed = options.speed or self.DEFAULT_SPEED,
    wait = 0,
    interrupt_radius = options.interrupt_radius or self.DEFAULT_INTERRUPT_RADIUS,
    pause_count = 0,
    paused_by = {},
  }
end

---@param bot_id Net.ActorId
function BotPaths:bot_initialized(bot_id)
  return self._bots[bot_id] ~= nil
end

---@param bot_id Net.ActorId
---@param path BotPaths.PathNode[]
function BotPaths:set_path(bot_id, path)
  local bot = self._bots[bot_id]
  bot.path = path
  bot.path_index = 1
  bot.wait = 0
end

---@param bot_id Net.ActorId
---@param speed number in tiles per tick
function BotPaths:set_speed(bot_id, speed)
  local bot = self._bots[bot_id]
  bot.speed = speed
end

---@param bot_id Net.ActorId
---@param radius number
function BotPaths:set_interrupt_radius(bot_id, radius)
  local bot = self._bots[bot_id]
  bot.interrupt_radius = radius
end

local PRIVATE_PAUSE_KEY = {}

---@param bot_id Net.ActorId
function BotPaths:pause_path(bot_id)
  self:pause_path_for(bot_id, PRIVATE_PAUSE_KEY)
end

---Fully clears pause tracking
---@param bot_id Net.ActorId
function BotPaths:resume_path(bot_id)
  local bot = self._bots[bot_id]

  bot.pause_count = 0
  bot.paused_by = {}
end

---@param bot_id Net.ActorId
---@param pause_key string|number|table
function BotPaths:pause_path_for(bot_id, pause_key)
  local bot = self._bots[bot_id]
  bot.pause_count = bot.pause_count + 1
  bot.paused_by[pause_key] = true
end

---@param bot_id Net.ActorId
---@param pause_key string|number|table
function BotPaths:resume_path_for(bot_id, pause_key)
  local bot = self._bots[bot_id]

  if bot.paused_by[pause_key] then
    bot.pause_count = bot.pause_count - 1
    bot.paused_by[pause_key] = nil
  end
end

---@param bot_id Net.ActorId
function BotPaths:is_paused(bot_id)
  local bot = self._bots[bot_id]
  return bot.pause_count > 0
end

---@param bot_id Net.ActorId
---@param pause_key string|number|table
function BotPaths:is_paused_by(bot_id, pause_key)
  local bot = self._bots[bot_id]
  return bot.paused_by[pause_key] ~= nil
end

---Removes tracking for this bot, requiring :init_bot() to be called again for future uses for this bot.
---@param bot_id Net.ActorId
function BotPaths:drop_bot(bot_id)
  self._bots[bot_id] = nil
end

function BotPaths:destroy()
  Net:remove_listener("tick", self._tick_listener)
  Net:remove_listener("player_disconnect", self._disconnect_listener)
end

return BotPaths
