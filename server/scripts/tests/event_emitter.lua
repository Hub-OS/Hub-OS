-- functions tested:
-- Net.EventEmitter.new
-- :on
-- :once
-- :on_any
-- :on_any_once
-- :remove_listener
-- :remove_on_any_listener
-- :destroy


local emitter = Net.EventEmitter.new()

local a_iter = emitter:async_iter("a") -- also tests :on
local iter_all = emitter:async_iter_all() -- also tests :any
local iter_err = emitter:async_iter_all()

-- setup
local emitted_events = {
  {"b", 2, '?'},
  {"a", 1, '?'},
  {"a", 3, '?'},
  {"c", 5, '?'},
  {"a", 4, '?'},
  {"d", 6, '?'},
  {"a", 7, '?'},
  {"e", 8, '?'},
}

local a_events = { }

for _, params in ipairs(emitted_events) do
  if params[1] == "a" then
    a_events[#a_events+1] = params
  end
end

-- the test
-- once also tests :remove_listener
emitter:once("a", function(num, word)
  local params = a_events[1]
  local matches = num == params[2] and word == params[3]

  if not matches then
    error("event data does not match")
  end
end)

-- on_any_once also tests :remove_on_any_listener
emitter:on_any_once(function(name, num, word)
  local params = emitted_events[1]
  local matches = name == params[1] and num == params[2] and word == params[3]

  if not matches then
    error("event data does not match")
  end
end)

Async.promisify(coroutine.create(function()
  -- testing :async_iter
  local i = 1

  for num, word in Async.await(a_iter) do
    local params = a_events[i]
    local matches = num == params[2] and word == params[3]

    if not matches then
      error("event data does not match")
    end

    i = i + 1
  end

  -- testing :async_iter_all
  i = 1

  for name, num, word in Async.await(iter_all) do
    local params = emitted_events[i]
    local matches = name == params[1] and num == params[2] and word == params[3]

    if not matches then
      error("event data does not match")
    end

    i = i + 1
  end

  -- testing helpful error
  local success, err = pcall(function()
    for _ in iter_err do
    end
  end)

  if not err then
    error("no error created for missing await")
  end
end))

for _, params in ipairs(emitted_events) do
  emitter:emit(table.unpack(params))
end

emitter:destroy()
