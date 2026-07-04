local CommandProcessing = {}

---@param string string
function CommandProcessing.trim(string)
  return string:match("^%s*(.-)%s*$") or ""
end

---@param string string
---@param init_index number?
---@return string? word
---@return number? start_index
---@return number? end_index
function CommandProcessing.read_word(string, init_index)
  init_index = init_index or 1

  if init_index > #string then
    return nil
  end

  local _, space_end = string:find("^%s*", init_index)

  if not space_end then
    return nil
  end

  local start_index, end_index = string:find("^[^%s]+", space_end + 1)

  if not start_index or not end_index then
    return nil
  end

  return string:sub(start_index, end_index), start_index, end_index
end

---Returns a function that can be called repeatedly to iterate over
---
---```lua
---local word_iter = CommandProcessing.words(" a b  cd  \n ")
---
---print(word_iter) -- "a", 2, 2
---print(word_iter) -- "b", 4, 4
---print(word_iter) -- "cd", 7, 8
---print(word_iter) -- nil
---```
---@param string string
---@param init_index number?
function CommandProcessing.words(string, init_index)
  init_index = init_index or 1

  ---@return string? word
  ---@return number? start_index
  ---@return number? end_index
  return function()
    if init_index > #string then
      return nil
    end

    local _, space_end = string:find("^%s*", init_index)

    if not space_end then
      init_index = #string + 1
      return nil
    end

    local start_index, end_index = string:find("^[^%s]+", space_end + 1)

    if not start_index or not end_index then
      init_index = #string + 1
      return nil
    end

    init_index = end_index + 1

    return string:sub(start_index, end_index), start_index, end_index
  end
end

---@param string string
---@param init_index number?
---@return string? unquoted
---@return number? start_index
---@return number? end_index
function CommandProcessing.read_optionally_quoted(string, init_index)
  local text, start_index, end_index = CommandProcessing.read_quoted(string, init_index)

  if not text then
    text, start_index, end_index = CommandProcessing.read_word(string, init_index)
  end

  return text, start_index, end_index
end

---Converts \n to a new line, and \\ as a regular backslash
---@param string string
function CommandProcessing.unescape(string)
  return string:gsub("\\(.)", function(capture)
    if capture == "n" then
      return "\n"
    end

    return capture
  end)
end

---Returns nil if the next non space character is not a quote, or no matching quote was found
---@param string string
---@param init_index number?
---@return string? word
---@return number? start_index
---@return number? end_index
function CommandProcessing.read_quoted(string, init_index)
  init_index = init_index or 1

  local _, start_index = string:find("^%s*[\"']", init_index)

  if not start_index then
    return
  end

  -- build a pattern to match against the first quote
  local end_pattern = '^.-(\\*)' .. string:sub(start_index, start_index)

  local next_index = start_index + 1

  while true do
    local _, quote_index, backslashes = string:find(end_pattern, next_index)

    if not quote_index then
      -- failed to find a matching quote
      return
    end

    next_index = quote_index + 1

    -- continue looping if the quote is escaped
    if #backslashes % 2 == 0 then
      break
    end
  end

  -- read and unescape the inner string
  local end_index = next_index - 1
  local inner_string = string:sub(start_index + 1, end_index - 1)
  local replaced_string = CommandProcessing.unescape(inner_string)

  return replaced_string, start_index, end_index
end

local function process_player_name(name)
  for _, area_id in ipairs(Net.list_areas()) do
    for _, player_id in ipairs(Net.list_players(area_id)) do
      if Net.get_actor_name(player_id) == name then
        return player_id
      end
    end
  end
end

---@param string string
---@param init_index number
---@return table<string, string>? args
---@return table<string, boolean>? inverted_args
---@return number? end_index
local function process_selector_arguments(string, init_index)
  local args = {}
  local inverted_args = {}
  local next_index = init_index

  -- see if args exist
  if string:byte(next_index) ~= ("["):byte(1) then
    return args, inverted_args, next_index - 1
  end

  next_index = next_index + 1

  while true do
    -- read name
    local outer, name = string:match("^(%s*(%a+)%s*=)", next_index)

    if not outer or not name then
      -- no key, see if we're done
      local _, bracket_index = string:find("^%s*]", next_index)

      if not bracket_index then
        return
      end

      next_index = bracket_index + 1
      break
    end

    next_index = next_index + #outer

    -- check for !
    local _, invert_end_index = string:find("^%s*!", next_index)

    if invert_end_index then
      inverted_args[name] = true
      next_index = invert_end_index + 1
    end

    -- read value
    local value, _, value_end_index = CommandProcessing.read_quoted(string, next_index)

    if not value or not value_end_index then
      outer, value = string:match("^(%s*([^,%s%]]+)%s*)", next_index)

      if not outer or not value then
        -- expecting value
        return
      end

      next_index = next_index + #outer
    else
      next_index = value_end_index + 1
    end

    args[name] = value

    -- find the end of this key value pair
    local outer, punctuation = string:match("^(%s*(.))", next_index)

    if not outer then
      return
    end

    next_index = next_index + #outer

    if punctuation == "]" then
      -- nothing more to process
      break
    end

    if punctuation ~= "," then
      -- invalid
      return
    end
  end

  return args, inverted_args, next_index - 1
end

function xor(a, b)
  return not (not a == not b)
end

---@param actor_ids Net.ActorId[]
---@param args table<string, string>
---@param inverted_args table<string, boolean>
local function selector_filter(actor_ids, args, inverted_args)
  --id filter, we only check this one if it's inverted
  --as we already have a fast path for the direct id filter
  ---@type number|string?
  local target_id = args.id
  local inverted = inverted_args.id

  if target_id and inverted then
    target_id = tonumber(target_id)

    for i = #actor_ids, 1, -1 do
      local actor_id = actor_ids[i]

      if actor_id == target_id then
        -- swap remove
        actor_ids[i] = actor_ids[#actor_ids]
        actor_ids[#actor_ids] = nil
      end
    end
  end

  local target_name = args.name
  inverted = inverted_args.name

  if target_name then
    for i = #actor_ids, 1, -1 do
      local actor_id = actor_ids[i]

      if xor(Net.get_actor_name(actor_id) ~= target_name, inverted) then
        -- swap remove
        actor_ids[i] = actor_ids[#actor_ids]
        actor_ids[#actor_ids] = nil
      end
    end
  end
end

---Inspired by minecraft's target selectors
---
---```
---`@a` all players
---`@e` all actors (bots + players)
---
---`@e[type=bot, name=ampstr]` = all bots named ampstr
---`@a[area=default]` = all players in the "default" area
---`@a[area=!default]` = all players not in the "default" area
---`@a[area="quoted area"]` = all players in "quoted area"
---```
---@param string string
---@param init_index number?
---@return Net.ActorId[]?
---@return string? selector
---@return number? end_index
function CommandProcessing.process_selector(string, init_index)
  local word_iter = CommandProcessing.words(string, init_index)
  local first_word, start_index, end_index = word_iter()

  if not first_word or not start_index or not end_index then
    return
  end

  -- initial selector test
  if not string:find("@%a%f[%[\0 ]", start_index) then
    -- process as a player name
    local player_id = process_player_name(first_word)

    if player_id then
      return { player_id }, first_word, end_index
    end

    return {}, first_word, end_index
  end

  local selector_variable = first_word:sub(1, 2)

  local args_start = start_index + 2
  local selector_args, inverted_args, args_end = process_selector_arguments(string, args_start)

  -- make sure the args are closed
  if not selector_args or not inverted_args or (string:byte(args_end + 1) ~= (" "):byte(1) and string:byte(args_end + 1) ~= nil) then
    -- not a selector
    return
  end

  local selector_string = string:sub(start_index, args_end)

  -- resolve supported actors
  local accept_players = true
  local accept_bots = false

  if selector_variable == "@a" then
  elseif selector_variable == "@e" then
    accept_bots = true
  else
    -- invalid selector
    return
  end

  if selector_args.type == "player" then
    if inverted_args.type then
      accept_players = false
    else
      accept_bots = false
    end
  end

  if selector_args.type == "bot" then
    if inverted_args.type then
      accept_bots = false
    else
      accept_players = false
    end
  end

  -- see if we're testing only a single actor to avoid testing everyone
  if selector_args.id and not inverted_args.id then
    -- separate branch to avoid looping over every actor
    local actor_id = tonumber(selector_args.id) --[[@as Net.ActorId]]

    local accepts_this_actor =
        actor_id % 1 == 0 and (
          (accept_players and Net.is_player(actor_id))
          or (accept_bots and Net.is_actor(actor_id)))

    if not accepts_this_actor then
      return {}, selector_string, args_end
    end

    -- test area
    local target_area = selector_args.area

    if target_area and xor(Net.get_actor_area(actor_id) ~= target_area, inverted_args.area) then
      return {}, selector_string, args_end
    end

    local actor_ids = { actor_id }
    selector_filter(actor_ids, selector_args, inverted_args)

    return actor_ids, selector_string, args_end
  end

  -- resolve areas
  local areas

  if not selector_args.area then
    areas = Net.list_areas()
  elseif inverted_args.area then
    areas = Net.list_areas()

    for i = 1, #areas do
      if areas[i] == selector_args.area then
        -- swap remove
        areas[i] = areas[#areas]
        areas[#areas] = nil
        break
      end
    end
  elseif Net.is_area(selector_args.area) then
    areas = { selector_args.area }
  else
    areas = {}
  end

  -- pull actors in relevant areas
  local actor_ids = {}

  for _, area_id in ipairs(areas) do
    if accept_players then
      local player_ids = Net.list_players(area_id)
      table.move(player_ids, 1, #player_ids, #actor_ids + 1, actor_ids)
    end

    if accept_bots then
      local bot_ids = Net.list_bots(area_id)
      table.move(bot_ids, 1, #bot_ids, #actor_ids + 1, actor_ids)
    end
  end

  -- filtering
  selector_filter(actor_ids, selector_args, inverted_args)

  return actor_ids, selector_string, args_end
end

---@class CommandProcessing.Command
---@field usage string[]? a list of argument lists to build the usage description, `{ "<arg>" }` is rewritten as `command <arg>`
---@field description string
---@field callback fun(event: { player_id: Net.ActorId?, command: string } )

local shared_command_map

---@param command_map table<string, CommandProcessing.Command>
function CommandProcessing.register_commands(command_map)
  if not shared_command_map then
    -- initialize event listener
    shared_command_map = {}

    Net:on("command", function(event)
      local command_string = event.command --[[@as string]]

      local command_name = CommandProcessing.read_word(command_string)
      local callback = shared_command_map[command_name]

      if callback then
        local _, err = pcall(function()
          callback(event)
        end)

        if err then
          Net.error_to(event.player_id, "Failed to execute command, check server console")

          error(err)
        end
      end
    end)
  end

  for name, info in pairs(command_map) do
    local description = ""

    if info.usage then
      for _, args in ipairs(info.usage) do
        description = description .. name .. " " .. args .. "\n"
      end
    else
      description = description .. name
    end

    description = description .. "\n" .. info.description

    Net.register_command(name, { description = description })
    shared_command_map[name] = info.callback
  end
end

return CommandProcessing
