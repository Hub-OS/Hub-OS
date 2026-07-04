local CommandProcessing = require("scripts/libs/command_processing")

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
          Net.print_to(event.player_id, player_id .. " " .. Net.get_actor_name(player_id))

          player_count = player_count + 1
        end
      end

      Net.print_to(event.player_id, "Found " .. player_count .. " players")
    end
  },
  ["list-actors"] = {
    usage = { "<actor>" },
    description = "Lists all matching actors",
    callback = function(event)
      local _, _, command_end_index = CommandProcessing.read_word(event.command)

      local actor_list = CommandProcessing.process_selector(event.command, command_end_index + 1)

      if not actor_list then
        Net.warn_to(event.player_id, "Missing or invalid selector")
        return
      end

      for _, actor_id in ipairs(actor_list) do
        local message = actor_id

        if Net.is_player(actor_id) then
          message = message .. " (player) "
        else
          message = message .. " (bot) "
        end
        message = message .. Net.get_actor_name(actor_id) .. " "

        Net.print_to(event.player_id, message)
      end

      Net.print_to(event.player_id, "Found " .. #actor_list .. " actors")
    end
  },
  ["locate"] = {
    usage = { "<actor>" },
    description = "Lists all matching actors with location data",
    callback = function(event)
      local _, _, command_end_index = CommandProcessing.read_word(event.command)

      local actor_list = CommandProcessing.process_selector(event.command, command_end_index + 1)

      if not actor_list then
        Net.warn_to(event.player_id, "Missing or invalid selector")
        return
      end

      for _, actor_id in ipairs(actor_list) do
        local message = actor_id

        if Net.is_player(actor_id) then
          message = message .. " (player) "
        else
          message = message .. " (bot) "
        end

        local name = Net.get_actor_name(actor_id)
        local area_id = Net.get_actor_area(actor_id)
        local x, y, z = Net.get_actor_position_multi(actor_id)
        message = message .. name .. " " .. area_id .. " (" .. x .. ", " .. y .. ", " .. z .. ")"

        Net.print_to(event.player_id, message)
      end

      Net.print_to(event.player_id, "Found " .. #actor_list .. " actors")
    end
  },
  ["teleport"] = {
    usage = { "<actor> <area> [<x> <y> <z>]", "<actor> <dest actor>" },
    description = "Teleports actors to the specified location",
    callback = function(event)
      local _, _, command_end_index = CommandProcessing.read_word(event.command)

      local actor_list, _, actor_end_index = CommandProcessing.process_selector(event.command, command_end_index + 1)

      if not actor_list then
        Net.warn_to(event.player_id, "Missing or invalid selector")
        return
      end

      local word_iter = CommandProcessing.words(event.command, actor_end_index + 1)
      local area_id = word_iter()
      local xs, ys, zs = word_iter(), word_iter(), word_iter()
      local x, y, z

      local dest_list = CommandProcessing.process_selector(event.command, actor_end_index + 1)

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
        if Net.get_actor_area(actor_id) == area_id then
          --   Net.warp_actor(actor_id, x, y, z)
          -- else
          Net.transfer_actor(actor_id, area_id, true, x, y, z)
        end
      end

      Net.print_to(event.player_id, "Teleported " .. #actor_list .. " actors")
    end
  },
  ["kick"] = {
    usage = { "<player> [<reason>]" },
    description = "Kicks players from the server",
    callback = function(event)
      local _, _, command_end_index = CommandProcessing.read_word(event.command)

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, command_end_index + 1)

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

          total_kicked = total_kicked + 1
        end
      end

      if total_kicked ~= 1 then
        Net.print_to(event.player_id, "Kicked " .. total_kicked .. " players")
      end
    end
  },
  message = {
    usage = { "<player> <message>" },
    description = "Message players through a textbox",
    callback = function(event)
      local _, _, command_end_index = CommandProcessing.read_word(event.command)

      local player_list, _, sel_end = CommandProcessing.process_selector(event.command, command_end_index + 1)

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

      Net.print_to(event.player_id, "Messaged \"" .. total_messaged .. '" players')
    end
  },
  stop = {
    description = "Stops the server",
    callback = function(event)
      Net.shutdown()
    end
  }
})
