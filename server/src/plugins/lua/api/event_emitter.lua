local EventEmitter = {}

function EventEmitter.new()
  local emitter = {
    _listeners = {},
    _any_listeners = {},
    _pending_removal = {},
    _any_pending_removal = nil,
    _destroy_listeners = {}
  }

  setmetatable(emitter, EventEmitter)
  EventEmitter.__index = EventEmitter
  return emitter
end

function EventEmitter:emit(name, ...)
  local listeners = self._listeners[name]

  -- clean up dead listeners
  local pending_removal = self._pending_removal[name]

  if pending_removal then
    for _, dead_listener in ipairs(pending_removal) do
      -- find and remove this listener
      for i, listener in ipairs(listeners) do
        if listener == dead_listener then
          table.remove(listeners, i)
          break
        end
      end
    end

    self._pending_removal[name] = nil
  end

  if self._any_pending_removal then
    for _, dead_listener in ipairs(self._any_pending_removal) do
      -- find and remove this listener
      for i, listener in ipairs(self._any_listeners) do
        if listener == dead_listener then
          table.remove(self._any_listeners, i)
          break
        end
      end
    end

    self._any_pending_removal = nil
  end

  -- call listeners
  if listeners then
    for _, listener in ipairs(listeners) do
      listener(...)
    end
  end

  for _, listener in ipairs(self._any_listeners) do
    listener(name, ...)
  end
end

function EventEmitter:on(name, callback)
  local listeners = self._listeners[name]

  if listeners then
    listeners[#listeners + 1] = callback
  else
    self._listeners[name] = { callback }
  end
end

function EventEmitter:once(name, callback)
  local cleanup

  cleanup = function()
    self:remove_listener(name, callback)
    self:remove_listener(name, cleanup)
  end

  self:on(name, callback)
  self:on(name, cleanup)
end

function EventEmitter:on_any(callback)
  local listeners = self._any_listeners

  listeners[#listeners + 1] = callback
end

function EventEmitter:on_any_once(callback)
  local cleanup

  cleanup = function()
    self:remove_on_any_listener(callback)
    self:remove_on_any_listener(cleanup)
  end

  self:on_any(callback)
  self:on_any(cleanup)
end

function EventEmitter:remove_listener(event_name, callback)
  local pending_removal = self._pending_removal[event_name]

  if pending_removal then
    pending_removal[#pending_removal + 1] = callback
  else
    self._pending_removal[event_name] = { callback }
  end
end

function EventEmitter:remove_on_any_listener(callback)
  local pending_removal = self._any_pending_removal

  if pending_removal then
    pending_removal[#pending_removal + 1] = callback
  else
    self._any_pending_removal = { callback }
  end
end

function EventEmitter:async_iter(name)
  local promise_queue = {}
  local latest_resolve

  promise_queue[#promise_queue + 1] = Async.create_promise(function(resolve)
    latest_resolve = resolve
  end)

  self:on(name, function(...)
    local last_resolve = latest_resolve

    promise_queue[#promise_queue + 1] = Async.create_promise(function(resolve)
      latest_resolve = resolve
    end)

    last_resolve(...)
  end)

  self._destroy_listeners[#self._destroy_listeners + 1] = function()
    latest_resolve(nil)
  end

  return function()
    local promise = table.remove(promise_queue, 1)

    if promise then
      return promise
    else
      error("read past end, are you awaiting?")
    end
  end
end

function EventEmitter:async_iter_all()
  local promise_queue = {}
  local latest_resolve

  promise_queue[#promise_queue + 1] = Async.create_promise(function(resolve)
    latest_resolve = resolve
  end)

  self:on_any(function(...)
    local last_resolve = latest_resolve

    promise_queue[#promise_queue + 1] = Async.create_promise(function(resolve)
      latest_resolve = resolve
    end)

    last_resolve(...)
  end)

  self._destroy_listeners[#self._destroy_listeners + 1] = function()
    latest_resolve(nil)
  end

  return function()
    local promise = table.remove(promise_queue, 1)

    if promise then
      return promise
    else
      error("read past end, are you awaiting?")
    end
  end
end

function EventEmitter:destroy()
  for _, listener in ipairs(self._destroy_listeners) do
    listener()
  end

  self._destroy_listeners = nil
end

Net = EventEmitter.new()
Net.EventEmitter = EventEmitter
