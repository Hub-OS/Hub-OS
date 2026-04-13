local function backwards_compat(table, old_name, new_name)
  table[old_name] = function(...)
    warn(old_name .. "() is deprecated, use " .. new_name .. "() instead.")

    -- we'll only warn once
    table[old_name] = function(...)
      return table[new_name](...)
    end

    return table[new_name](...)
  end
end

backwards_compat(Net, "get_song", "get_music")
backwards_compat(Net, "set_song", "set_music")
backwards_compat(Net, "get_width", "get_layer_width")
backwards_compat(Net, "get_height", "get_layer_height")
backwards_compat(Net, "close_bbs", "close_board")

backwards_compat(Net, "get_player_area", "get_actor_area")
backwards_compat(Net, "get_bot_area", "get_actor_area")
backwards_compat(Net, "get_player_name", "get_actor_name")
backwards_compat(Net, "get_bot_name", "get_actor_name")
backwards_compat(Net, "set_player_name", "set_actor_name")
backwards_compat(Net, "set_bot_name", "set_actor_name")
backwards_compat(Net, "get_player_direction", "get_actor_direction")
backwards_compat(Net, "get_bot_direction", "get_actor_direction")
backwards_compat(Net, "get_player_position", "get_actor_position")
backwards_compat(Net, "get_bot_position", "get_actor_position")
backwards_compat(Net, "get_player_position_multi", "get_actor_position_multi")
backwards_compat(Net, "get_bot_position_multi", "get_actor_position_multi")
backwards_compat(Net, "animate_player", "animate_actor")
backwards_compat(Net, "animate_bot", "animate_actor")
backwards_compat(Net, "animate_player_properties", "animate_actor_properties")
backwards_compat(Net, "animate_bot_properties", "animate_actor_properties")
backwards_compat(Net, "get_player_avatar", "get_actor_avatar")
backwards_compat(Net, "get_bot_avatar", "get_actor_avatar")
backwards_compat(Net, "set_player_avatar", "set_actor_avatar")
backwards_compat(Net, "set_bot_avatar", "set_actor_avatar")
backwards_compat(Net, "set_player_emote", "set_actor_emote")
backwards_compat(Net, "set_bot_emote", "set_actor_emote")
backwards_compat(Net, "exclusive_player_emote", "exclusive_actor_emote_for_player")
backwards_compat(Net, "set_player_map_color", "set_actor_map_color")
backwards_compat(Net, "set_bot_map_color", "set_actor_map_color")
