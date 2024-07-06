local tasks = {}

Net:on("tick", function(event)
  local completed_indexes = {}

  for i, task in ipairs(tasks) do
    if task(event.delta_time) then
      completed_indexes[#completed_indexes + 1] = i
    end
  end

  for i = #completed_indexes, 1, -1 do
    table.remove(tasks, completed_indexes[i])
  end
end)


function Async.await(promise)
  if type(promise) == "function" then
    -- awaiting an iterator that returns promises
    return function()
      return Async.await(promise())
    end
  else
    -- awaiting a promise object
    local co = coroutine.running()
    local pending = true
    local value

    promise.and_then(function(...)
      pending = false
      value = { ... }
      coroutine.resume(co)
    end)

    while pending do
      coroutine.yield()
    end

    return table.unpack(value)
  end
end

function Async.await_all(promises)
  local co = coroutine.running()
  local completed = 0
  local values = {}

  for i, promise in pairs(promises) do
    promise.and_then(function(value)
      values[i] = value
      completed = completed + 1
      coroutine.resume(co)
    end)
  end

  while completed < #promises do
    coroutine.yield()
  end

  return values
end

function Async.create_promise(task)
  local listeners = {}
  local resolved = false
  local value

  local function resolve(...)
    resolved = true
    value = { ... }

    for _, listener in ipairs(listeners) do
      local success, err = pcall(function()
        listener(table.unpack(value))
      end)

      if not success then
        printerr("runtime error: " .. tostring(err))
      end
    end
  end

  local promise = {}

  function promise.and_then(listener)
    if resolved then
      listener(table.unpack(value))
    else
      listeners[#listeners + 1] = listener
    end
  end

  task(resolve)

  return promise
end

function Async.create_scope(callback)
  return Async.create_promise(function(resolve)
    local co = coroutine.create(function()
      local result = table.pack(pcall(callback))

      if result[1] then
        resolve(table.unpack(result, 2))
      else
        printerr(result[2])
      end
    end)

    coroutine.resume(co)
  end)
end

function Async.create_function(callback)
  return function(...)
    local params = table.pack(...)

    return Async.create_scope(function()
      return callback(table.unpack(params))
    end)
  end
end

function Async.sleep(duration)
  local promise = Async.create_promise(function(resolve)
    local time = 0

    local function update(delta)
      time = time + delta

      if time >= duration then
        resolve()
        return true
      end

      return false
    end

    tasks[#tasks + 1] = update
  end)

  return promise
end

function Async._promise_from_id(id)
  local promise = Async.create_promise(function(resolve)
    local function update()
      if not Async._is_promise_pending(id) then
        resolve(Async._get_promise_value(id))
        return true
      end

      return false
    end

    tasks[#tasks + 1] = update
  end)

  return promise
end

-- asyncified

local AsyncifiedTracker = {}

function AsyncifiedTracker.new()
  local tracker = {
    resolvers = {},
    next_promise = { 0 },
  }

  setmetatable(tracker, AsyncifiedTracker)
  AsyncifiedTracker.__index = AsyncifiedTracker
  return tracker
end

function AsyncifiedTracker:increment_count()
  self.next_promise[#self.next_promise] = self.next_promise[#self.next_promise] + 1
end

function AsyncifiedTracker:create_promise()
  return Async.create_promise(function(resolve)
    self.resolvers[#self.resolvers + 1] = resolve
    self.next_promise[#self.resolvers + 1] = 0
  end)
end

function AsyncifiedTracker:resolve(value)
  local next_promise = self.next_promise

  if next_promise[1] == 0 then
    local resolve = table.remove(self.resolvers, 1)

    if resolve == nil then
      return
    end

    if #next_promise > 1 then
      table.remove(next_promise, 1)
    end

    resolve(value)
  else
    next_promise[1] = next_promise[1] - 1
  end
end

function AsyncifiedTracker:destroy()
  for _, resolve in ipairs(self.resolvers) do
    resolve(nil)
  end
end

-- asyncified shared

local textbox_trackers = {}
local battle_trackers = {}

Net:on("player_disconnect", function(event)
  local player_id = event.player_id

  textbox_trackers[player_id]:destroy()
  textbox_trackers[player_id] = nil
  battle_trackers[player_id]:destroy()
  battle_trackers[player_id] = nil
end)

Net:on("player_request", function(event)
  local player_id = event.player_id

  textbox_trackers[player_id] = AsyncifiedTracker.new()
  battle_trackers[player_id] = AsyncifiedTracker.new()
end)

local function create_asyncified_api(function_name, trackers)
  local delegate_name = "Net._" .. function_name

  Async[function_name] = function(player_id, ...)
    local tracker = trackers[player_id]

    if tracker == nil then
      -- player has disconnected or never existed
      return Async.create_promise(function(resolve) resolve(nil) end)
    end

    Net._delegate(delegate_name, player_id, ...)

    return tracker:create_promise()
  end

  Net[function_name] = function(player_id, ...)
    local tracker = trackers[player_id]

    if tracker == nil then
      -- player has disconnected or never existed
      return
    end

    tracker:increment_count()

    Net._delegate(delegate_name, player_id, ...)
  end
end

local function create_asyncified_netplay_api(function_name, trackers)
  local delegate_name = "Net._" .. function_name

  Async[function_name] = function(player_ids, ...)
    local promises = {}

    for _, player_id in ipairs(player_ids) do
      local tracker = trackers[player_id]

      if tracker then
        promises[#promises + 1] = tracker:create_promise()
      else
        -- player has disconnected or never existed
        promises[#promises + 1] = Async.create_promise(function(resolve) resolve(nil) end)
      end
    end

    Net._delegate(delegate_name, player_ids, ...)

    return promises
  end

  Net[function_name] = function(player_ids, ...)
    for _, player_id in ipairs(player_ids) do
      local tracker = trackers[player_id]

      if tracker then
        tracker:increment_count()
      end
    end

    Net._delegate(delegate_name, player_ids, ...)
  end
end

-- asyncified textboxes

create_asyncified_api("message_player", textbox_trackers)
create_asyncified_api("message_player_auto", textbox_trackers)
create_asyncified_api("question_player", textbox_trackers)
create_asyncified_api("quiz_player", textbox_trackers)
create_asyncified_api("prompt_player", textbox_trackers)

Net:on("textbox_response", function(event)
  local player_id = event.player_id

  textbox_trackers[player_id]:resolve(event.response)
end)

-- asyncified battles

create_asyncified_api("initiate_encounter", battle_trackers)
create_asyncified_netplay_api("initiate_netplay", battle_trackers)

function Async.initiate_pvp(player1_id, player2_id, ...)
  return Async.initiate_netplay({ player1_id, player2_id }, ...)
end

Net:on("battle_results", function(event)
  local player_id = event.player_id

  battle_trackers[player_id]:resolve(event)
end)

-- shops

local shop_emitters = {}

Net:on("player_request", function(event)
  shop_emitters[event.player_id] = {}
end)

Net:on("player_disconnect", function(event)
  for _, emitter in ipairs(shop_emitters[event.player_id]) do
    emitter:emit("shop_close", event)
    emitter:destroy()
  end

  shop_emitters[event.player_id] = nil
end)

function Net.open_shop(player_id, ...)
  local emitters = shop_emitters[player_id]

  if not emitters then
    -- player must have disconnected
    return
  end

  Net._delegate("Net._open_shop", player_id, ...)

  local emitter = Net.EventEmitter.new()
  emitters[#emitters + 1] = emitter
  return emitter
end

Net:on("shop_purchase", function(event)
  shop_emitters[event.player_id][1]:emit("shop_purchase", event)
end)

Net:on("shop_description_request", function(event)
  shop_emitters[event.player_id][1]:emit("shop_description_request", event)
end)

Net:on("shop_leave", function(event)
  shop_emitters[event.player_id][1]:emit("shop_leave", event)
end)

Net:on("shop_close", function(event)
  local emitter = table.remove(shop_emitters[event.player_id], 1)
  emitter:emit("shop_close", event)
  emitter:destroy()
end)

-- bbs

local bbs_emitters = {}

Net:on("player_request", function(event)
  bbs_emitters[event.player_id] = {}
end)

Net:on("player_disconnect", function(event)
  for _, emitter in ipairs(bbs_emitters[event.player_id]) do
    emitter:emit("board_close", event)
    emitter:destroy()
  end

  bbs_emitters[event.player_id] = nil
end)

function Net.open_board(player_id, ...)
  local emitters = bbs_emitters[player_id]

  if not emitters then
    -- player must have disconnected
    return
  end

  Net._delegate("Net._open_board", player_id, ...)

  local emitter = Net.EventEmitter.new()
  emitters[#emitters + 1] = emitter
  return emitter
end

Net:on("post_request", function(event)
  bbs_emitters[event.player_id][1]:emit("post_request", event)
end)

Net:on("post_selection", function(event)
  bbs_emitters[event.player_id][1]:emit("post_selection", event)
end)

Net:on("board_close", function(event)
  local emitter = table.remove(bbs_emitters[event.player_id], 1)
  emitter:emit("board_close", event)
  emitter:destroy()
end)
