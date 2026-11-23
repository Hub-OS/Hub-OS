local CommandProcessing = require("scripts/libs/command_processing")
local json = require("scripts/libs/json")

---@type table<string, { nickname: string, end_time?: number, reason?: string }>
local banned_players = {}
---@type table<string, { nickname?: string, reason?: string }>
local banned_ips = {}
local save_ban_list
local BAN_FILE_NAME = "banned-players.json"

local property_resolvers = {
  id = function(actor_id)
    return actor_id
  end,
  name = function(actor_id)
    return json.encode(Net.get_actor_name(actor_id))
  end,
  type = function(actor_id)
    if Net.is_player(actor_id) then
      return "player"
    else
      return "bot"
    end
  end,
  area = function(actor_id)
    return json.encode(Net.get_actor_area(actor_id))
  end,
  avatar = function(actor_id)
    if Net.is_player(actor_id) then
      return json.encode(Net.get_player_avatar_name(actor_id))
    end
  end,
  x = function(actor_id)
    local x = Net.get_actor_position_multi(actor_id)
    return math.floor(x)
  end,
  y = function(actor_id)
    local _, y = Net.get_actor_position_multi(actor_id)
    return math.floor(y)
  end,
  z = function(actor_id)
    local _, _, z = Net.get_actor_position_multi(actor_id)
    return math.floor(z)
  end,
  ["x."] = function(actor_id)
    local x = Net.get_actor_position_multi(actor_id)
    return x
  end,
  ["y."] = function(actor_id)
    local _, y = Net.get_actor_position_multi(actor_id)
    return y
  end,
  ["z."] = function(actor_id)
    local _, _, z = Net.get_actor_position_multi(actor_id)
    return z
  end,
}

CommandProcessing.register_commands({
  help = {
    usage = { "[<command>]" },
    description = "Lists all commands",
    callback = function(event)
      local word_iter = CommandProcessing.words(event.command)
      word_iter()
      local command_name = word_iter()

      if not command_name then
        local commands = Net.list_commands()

        table.sort(commands)

        for _, command in ipairs(commands) do
          Net.print_to(event.player_id, command)
        end

        return
      end

      local description = Net.get_command_description(command_name)

      if description then
        Net.print_to(event.player_id, description)
      else
        Net.warn_to(event.player_id, "Command \"" .. command_name .. "\" not found")
      end
    end
  },
  ["list-areas"] = {
    description = "Lists area ids for all loaded areas",
    callback = function(event)
      for _, area_id in ipairs(Net.list_areas()) do
        Net.print_to(event.player_id, area_id)
      end
    end
  },
  ["list-players"] = {
    usage = { "[<area_id>]" },
    description = "Lists all players connected to the server",
    callback = function(event)
      local word_iter = CommandProcessing.words(event.command)
      word_iter()

      local specified_area_id = word_iter()
      local area_ids

      if specified_area_id then
        if not Net.is_area(specified_area_id) then
          Net.warn_to(event.player_id, "No area with id \"" .. specified_area_id:gsub('"', '\"') .. '"')
          return
        end

        area_ids = { specified_area_id }
      else
        area_ids = Net.list_areas()
      end

      local player_count = 0

      for _, area_id in ipairs(area_ids) do
        for _, player_id in ipairs(Net.list_players(area_id)) do
          Net.print_to(event.player_id, Net.get_actor_name(player_id))

          player_count = player_count + 1
        end
      end

      Net.print_to(event.player_id, "Found " .. player_count .. " players")
    end
  },
  ["list-actors"] = {
    usage = { "<actor> <id|name|type|area|avatar|x|y|z|x.|y.|z.>*" },
    description = "Lists all matching actors",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local actor_list, _, last_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not actor_list then
        Net.warn_to(event.player_id, "Missing or invalid selector")
        return
      end

      local resolver_list = {}

      while true do
        local prop, _, prop_end = CommandProcessing.read_word(event.command, last_end + 1)

        if not prop then
          break
        end

        last_end = prop_end

        local resolver = property_resolvers[prop]

        if not resolver then
          Net.warn_to(event.player_id, "Invalid property: " .. prop)
          return
        end

        resolver_list[#resolver_list + 1] = resolver
      end

      if #resolver_list == 0 then
        Net.warn_to(event.player_id, "Missing properties")
        return
      end

      for _, actor_id in ipairs(actor_list) do
        local message = ""

        for j = 1, #resolver_list do
          message = message .. tostring(resolver_list[j](actor_id)) .. " "
        end

        Net.print_to(event.player_id, message)
      end

      Net.print_to(event.player_id, "Found " .. #actor_list .. " actors")
    end
  },
  ["teleport"] = {
    usage = { "<actor> <area> [<x> <y> <z>]", "<actor> <dest actor>" },
    description = "Teleports actors to the specified location",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local actor_list, _, actor_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not actor_list then
        Net.warn_to(event.player_id, "Missing or invalid selector")
        return
      end

      local word_iter = CommandProcessing.words(event.command, actor_end + 1)
      local area_id = word_iter()
      local xs, ys, zs = word_iter(), word_iter(), word_iter()
      local x, y, z

      local dest_list = CommandProcessing.process_selector(event.command, actor_end + 1)

      if dest_list and not xs then
        local dest_actor_id = dest_list[1]
        if not dest_actor_id then
          Net.warn_to(event.player_id, "Destination selector failed to match any actors")
          return
        end

        area_id = Net.get_actor_area(dest_actor_id)
        x, y, z = Net.get_actor_position_multi(dest_actor_id)
      else
        if not area_id then
          Net.warn_to(event.player_id, "Missing destination")
          return
        end

        if not Net.is_area(area_id) then
          Net.warn_to(event.player_id, "No area matching " .. area_id .. " found")
          return
        end


        if xs then
          x, y, z = tonumber(xs), tonumber(ys), tonumber(zs)

          if not x or not y or not z then
            Net.warn_to(event.player_id, "Invalid coordinates")
            return
          end
        end
      end

      for _, actor_id in ipairs(actor_list) do
        Net.transfer_actor(actor_id, area_id, true, x, y, z)
      end

      Net.print_to(event.player_id, "Teleported " .. #actor_list .. " actors")
    end
  },
  ["emote"] = {
    usage = { "<from_actor> <to_actor> <emote>" },
    description = "Causes actors to perform an emote",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local from_list, _, from_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not from_list or not from_end then
        Net.warn_to(event.player_id, "Missing from selector")
        return
      end

      local to_list, _, to_end = CommandProcessing.process_selector(event.command, from_end + 1)

      if not to_list or not to_end then
        Net.warn_to(event.player_id, "Missing to selector")
        return
      end

      local emote = CommandProcessing.read_optionally_quoted(event.command, to_end + 1)

      if not emote then
        Net.warn_to(event.player_id, "Missing emote")
        return
      end

      Net.synchronize(function()
        local total_to = 0

        for _, to_id in ipairs(to_list) do
          if Net.is_player(to_id) then
            for _, from_id in ipairs(from_list) do
              Net.exclusive_actor_emote_for_player(to_id, from_id, emote)
            end

            total_to = total_to + 1
          end
        end

        Net.print_to(event.player_id, #from_list .. " entities emoted to " .. total_to .. " players")
      end)
    end
  },
  ["kick"] = {
    usage = { "<player> [<reason>]" },
    description = "Kicks players from the server",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not player_list then
        Net.warn_to(event.player_id, "Missing player selector")
        return
      end

      local reason = CommandProcessing.trim(event.command:sub(sel_end + 1))

      if #reason == 0 then
        reason = "No reason provided."
      end

      local total_kicked = 0

      for i = 1, #player_list do
        local player_id = player_list[i]

        if Net.is_player(player_id) then
          local name = Net.get_actor_name(player_id)

          Net.kick_player(player_id, reason, true)
          Net.print_to(event.player_id, "Kicked " .. name)

          if event.player_id then
            print("Kicked " .. name)
          end

          total_kicked = total_kicked + 1
        end
      end

      if total_kicked ~= 1 then
        Net.print_to(event.player_id, "Kicked " .. total_kicked .. " players")
      end
    end
  },
  ["perma-ban"] = {
    usage = { "<player> [<reason>]" },
    description = "Bans players from the server",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not player_list then
        Net.warn_to(event.player_id, "Missing player selector")
        return
      end

      ---@type string?
      local reason = CommandProcessing.trim(event.command:sub(sel_end + 1))
      local display_reason

      if #reason > 0 then
        display_reason = "Banned: " .. reason
      else
        display_reason = "Banned"
        reason = nil
      end

      local total_bans = 0

      for i = 1, #player_list do
        local player_id = player_list[i]

        if Net.is_player(player_id) then
          local name = Net.get_actor_name(player_id)

          banned_players[Net.get_player_secret(player_id)] = {
            nickname = Net.get_actor_name(player_id),
            reason = reason
          }

          Net.kick_player(player_id, display_reason, true)
          Net.print_to(event.player_id, "Banned " .. name)

          if event.player_id then
            print("Banned " .. name)
          end

          total_bans = total_bans + 1
        end
      end

      if total_bans ~= 1 then
        Net.print_to(event.player_id, "Banned " .. total_bans .. " players")
      end

      if total_bans > 0 then
        save_ban_list()
      end
    end
  },
  ["ban"] = {
    usage = { "<player> <duration> [<reason>]" },
    description = "Bans players from the server for a specific time period. Accepts a number and unit (y/d/h/m/s)",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not player_list then
        Net.warn_to(event.player_id, "Missing player selector")
        return
      end

      local duration_str, _, duration_end = CommandProcessing.read_word(event.command, sel_end + 1)

      if not duration_str then
        Net.warn_to(event.player_id, "Missing duration")
        return
      end

      local duration = CommandProcessing.process_duration(duration_str)

      if not duration then
        Net.warn_to(event.player_id, "Invalid duration")
        return
      end

      ---@type string?
      local reason = CommandProcessing.trim(event.command:sub(duration_end + 1))
      local display_reason
      local end_time = os.time() + duration

      if #reason > 0 then
        display_reason = "Banned for " .. duration_str .. ": " .. reason
      else
        display_reason = "Banned for " .. duration_str
        reason = nil
      end

      local total_bans = 0

      for i = 1, #player_list do
        local player_id = player_list[i]

        if Net.is_player(player_id) then
          local name = Net.get_actor_name(player_id)

          banned_players[Net.get_player_secret(player_id)] = {
            nickname = Net.get_actor_name(player_id),
            reason = reason,
            end_time = end_time
          }

          Net.kick_player(player_id, display_reason, true)
          Net.print_to(event.player_id, "Banned " .. name)

          if event.player_id then
            print("Banned " .. name)
          end

          total_bans = total_bans + 1
        end
      end

      if total_bans ~= 1 then
        Net.print_to(event.player_id, "Banned " .. total_bans .. " players")
      end

      if total_bans > 0 then
        save_ban_list()
      end
    end
  },
  ["ban-ip"] = {
    usage = { "<player> [<reason>]" },
    description = "Bans players by IP from the server",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not player_list then
        Net.warn_to(event.player_id, "Missing player selector")
        return
      end

      ---@type string?
      local reason = CommandProcessing.trim(event.command:sub(sel_end + 1))
      local display_reason

      if #reason > 0 then
        display_reason = "Banned: " .. reason
      else
        display_reason = "Banned"
        reason = nil
      end

      local total_banned = 0

      for i = 1, #player_list do
        local player_id = player_list[i]

        if Net.is_player(player_id) then
          local name = Net.get_actor_name(player_id)


          banned_ips[Net.get_player_ip(player_id)] = {
            nickname = Net.get_actor_name(player_id),
            reason = reason
          }

          Net.kick_player(player_id, display_reason, true)
          Net.print_to(event.player_id, "Banned " .. name)

          if event.player_id then
            print("Banned " .. name)
          end

          total_banned = total_banned + 1
        end
      end

      if total_banned ~= 1 then
        Net.print_to(event.player_id, "Banned " .. total_banned .. " players")
      end

      if total_banned > 0 then
        save_ban_list()
      end
    end
  },
  ["unban"] = {
    usage = { "<name>" },
    description = "Unbans a player by nickname from the server, may unban multiple players",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)
      local name = CommandProcessing.read_optionally_quoted(event.command, command_end + 1)

      if not name then
        Net.warn_to(event.player_id, "Missing name")
        return
      end

      local unbans = 0

      for key, details in pairs(banned_players) do
        if details.nickname == name then
          banned_players[key] = nil
          unbans = unbans + 1
        end
      end

      for key, details in pairs(banned_ips) do
        if details.nickname == name then
          banned_ips[key] = nil
          unbans = unbans + 1
        end
      end

      if unbans == 0 then
        Net.warn_to(event.player_id, name .. " was already not banned")
        return
      end

      local message = "Unbanned " .. unbans .. " players named " .. name
      Net.print_to(event.player_id, message)

      if event.player_id then
        print(message)
      end

      save_ban_list()
    end
  },
  ["unban-ip"] = {
    usage = { "<ip address>" },
    description = "Unbans a player's IP address from the server",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)
      local ip = CommandProcessing.read_word(event.command, command_end + 1)

      if not ip then
        Net.warn_to(event.player_id, "Missing IP address")
        return
      end

      local details = banned_ips[ip]

      if not details then
        Net.warn_to(event.player_id, ip .. " was already not banned")
        return
      end

      banned_ips[ip] = nil

      local message = "Unbanned " .. ip

      if details.nickname then
        message = message .. " (" .. details.nickname .. ")"
      end

      Net.print_to(event.player_id, message)

      if event.player_id then
        print(message)
      end
    end
  },
  ["ban-list"] = {
    description = "Lists banned IPs and players",
    callback = function(event)
      local total_banned = 0

      for key, details in pairs(banned_ips) do
        local message = tostring(key)

        if details.nickname then
          message = message .. " " .. json.encode(details.nickname)
        end

        if details.reason then
          message = message .. " " .. json.encode(details.reason)
        end

        Net.print_to(event.player_id, message)
        total_banned = total_banned + 1
      end

      local time = os.time()

      for _, details in pairs(banned_players) do
        local message = json.encode(details.nickname)

        if details.end_time then
          local duration = details.end_time - time
          message = message .. " " .. CommandProcessing.format_duration(duration)
        end

        if details.reason then
          message = message .. " " .. json.encode(details.reason)
        end

        Net.print_to(event.player_id, message)
        total_banned = total_banned + 1
      end

      Net.print_to(event.player_id, total_banned .. " bans total")
    end
  },
  message = {
    usage = { "<player> <message>" },
    description = "Message players through a textbox",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, command_end + 1)

      if not player_list then
        Net.warn_to(event.player_id, "Missing player selector")
        return
      end

      local message = CommandProcessing.unescape(CommandProcessing.trim(event.command:sub(sel_end + 1)))

      if message == "" then
        Net.warn_to(event.player_id, "Missing message")
        return
      end

      local total_messaged = 0

      for i = 1, #player_list do
        local player_id = player_list[i]

        if Net.is_player(player_id) then
          Net.message_player(player_id, message)
          total_messaged = total_messaged + 1
        end
      end

      Net.print_to(event.player_id, "Messaged " .. total_messaged .. " players")
    end
  },
  assets = {
    usage = {
      "list [<substring>]",
      "load <server_path> [<file_path>]",
      "unload <server_path>",
      "size <server_path>",
      "type <server_path>",
      "hash <server_path>",
    },
    description = "Loads and unloads assets",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local subcommand, _, subcommand_end = CommandProcessing.read_word(event.command, command_end + 1)

      if not subcommand then
        Net.warn_to(event.player_id, 'Missing subcommand')
      end

      -- special check for the list command, as it doesn't use <server_path>
      if subcommand == "list" then
        local list = Net.list_assets()
        local filter = CommandProcessing.read_optionally_quoted(event.command, subcommand_end + 1)
        local total_matches = 0

        for _, path in ipairs(list) do
          if not filter or path:find(filter, nil, true) then
            Net.print_to(event.player_id, path)
            total_matches = total_matches + 1
          end
        end

        if #list == 0 then
          Net.print_to(event.player_id, "No assets loaded")
        elseif filter and total_matches then
          Net.print_to(event.player_id, total_matches .. " assets matched")
        end

        return
      end

      -- read server path arg
      local server_path, _, server_path_end = CommandProcessing.read_optionally_quoted(event.command, subcommand_end + 1)

      if not server_path then
        Net.warn_to(event.player_id, 'Missing asset path')
        return
      end

      -- process subcommands
      if subcommand == "load" then
        local file_path = CommandProcessing.read_optionally_quoted(event.command, server_path_end + 1)

        if not file_path then
          local SERVER_PREFIX = "/server/"

          if server_path:sub(1, #SERVER_PREFIX) ~= SERVER_PREFIX then
            Net.warn_to(event.player_id, 'Invalid file path')
            return
          end

          file_path = server_path:sub(#SERVER_PREFIX + 1)
        end

        Net.print_to(event.player_id, 'Updating "' .. server_path .. '"...')

        Async.read_file(file_path).and_then(function(contents)
          Net.update_asset(server_path, contents)
          Net.print_to(event.player_id, 'Updated "' .. server_path .. '"')
        end)
      elseif subcommand == "unload" then
        Net.remove_asset(server_path)
        Net.print_to(event.player_id, 'Removed "' .. server_path .. '"')
      elseif subcommand == "size" then
        Net.print_to(event.player_id, Net.get_asset_size(server_path))
      elseif subcommand == "type" then
        Net.print_to(event.player_id, Net.get_asset_type(server_path))
      elseif subcommand == "hash" then
        Net.print_to(event.player_id, Net.get_asset_hash(server_path))
      else
        Net.warn_to(event.player_id, 'Invalid subcommand "' .. subcommand .. '"')
      end
    end
  },
  restrictions = {
    usage = {
      "apply <player> <server_path>",
      "clear <player>"
    },
    description = "Applies a restriction file to players",
    callback = function(event)
      local _, _, command_end = CommandProcessing.read_word(event.command)

      local subcommand, _, subcommand_end = CommandProcessing.read_word(event.command, command_end + 1)

      if not subcommand then
        Net.warn_to(event.player_id, 'Missing subcommand')
      end

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, subcommand_end + 1)

      if not player_list then
        Net.warn_to(event.player_id, "Missing player selector")
        return
      end

      if subcommand == "apply" then
        local server_path = CommandProcessing.read_optionally_quoted(event.command, sel_end + 1)

        if not server_path then
          Net.warn_to(event.player_id, 'Missing asset path')
          return
        end

        local total_players = 0

        for _, player_id in ipairs(player_list) do
          if Net.is_player(player_id) then
            Net.set_player_restrictions(player_id, server_path)
            total_players = total_players + 1
          end
        end

        Net.print_to(event.player_id, "Cleared restrictions for " .. total_players .. " players")
      elseif subcommand == "clear" then
        local total_players = 0

        for _, player_id in ipairs(player_list) do
          Net.set_player_restrictions(player_id)
          total_players = total_players + 1
        end

        Net.print_to(event.player_id, "Cleared restrictions for " .. total_players .. " players")
      else
        Net.warn_to(event.player_id, 'Invalid subcommand "' .. subcommand .. '"')
      end
    end
  },
  stop = {
    description = "Stops the server",
    callback = function(event)
      Net.shutdown()
    end
  }
})

function save_ban_list()
  Async.write_file(BAN_FILE_NAME, json.encode({
    banned_players = banned_players,
    banned_ips = banned_ips
  }))
end

Async.read_file(BAN_FILE_NAME).and_then(function(contents)
  if #contents == 0 then
    return
  end

  local data = json.decode(contents)

  -- copy new data into old file
  local total_writes = 0
  for key, value in pairs(banned_players) do
    data.banned_players[key] = value
    total_writes = total_writes + 1
  end

  for key, value in pairs(banned_ips) do
    data.banned_ips[key] = value
    total_writes = total_writes + 1
  end

  banned_players = data.banned_players
  banned_ips = data.banned_ips

  if total_writes > 0 then
    save_ban_list()
  end
end)

local function kick_banned_player(player_id, details)
  local reason = "Banned"

  if details.reason then
    reason = "Banned: " .. details.reason
  end

  Net.kick_player(player_id, reason, false)
end

Net:on("player_request", function(event)
  local ip_ban_details = banned_ips[Net.get_player_ip(event.player_id)]

  if ip_ban_details then
    kick_banned_player(event.player_id, ip_ban_details)
    return
  end

  local secret = Net.get_player_secret(event.player_id)
  local direct_ban_details = banned_players[secret]

  if direct_ban_details and direct_ban_details.end_time and direct_ban_details.end_time < os.time() then
    -- ban ended!!
    banned_players[secret] = nil
    save_ban_list()
    return
  end

  if direct_ban_details then
    kick_banned_player(event.player_id, direct_ban_details)
    return
  end
end)
