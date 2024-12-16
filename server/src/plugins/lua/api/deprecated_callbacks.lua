---@diagnostic disable: undefined-global

local function backwards_compat(event_name, old_name, callback)
  if _G[old_name] then
    warn(old_name .. "() is deprecated, use Net:on(\"" .. event_name .. "\", function(event) end)")

    Net:on(event_name, callback)
  end
end

backwards_compat("tick", "tick", function(event)
  tick(event.delta_time)
end)

backwards_compat("authorization", "handle_authorization", function(event)
  handle_authorization(event.identity, event.host, event.port, event.data)
end)

backwards_compat("player_request", "handle_player_request", function(event)
  handle_player_request(event.player_id, event.data)
end)

backwards_compat("player_connect", "handle_player_connect", function(event)
  handle_player_connect(event.player_id)
end)

backwards_compat("player_join", "handle_player_join", function(event)
  handle_player_join(event.player_id)
end)

backwards_compat("player_area_transfer", "handle_player_transfer", function(event)
  handle_player_transfer(event.player_id)
end)

backwards_compat("player_disconnect", "handle_player_disconnect", function(event)
  handle_player_disconnect(event.player_id)
end)

backwards_compat("player_move", "handle_player_move", function(event)
  handle_player_move(event.player_id, event.x, event.y, event.z)
end)

backwards_compat("player_avatar_change", "handle_player_avatar_change", function(event)
  if handle_player_avatar_change(event.player_id, event) then
    event.prevent_default()
  end
end)

backwards_compat("player_emote", "handle_player_emote", function(event)
  if handle_player_emote(event.player_id, event.emote) then
    event.prevent_default()
  end
end)

backwards_compat("custom_warp", "handle_custom_warp", function(event)
  handle_custom_warp(event.player_id, event.object_id)
end)

backwards_compat("object_interaction", "handle_object_interaction", function(event)
  handle_object_interaction(event.player_id, event.object_id, event.button)
end)

backwards_compat("actor_interaction", "handle_actor_interaction", function(event)
  handle_actor_interaction(event.player_id, event.actor_id, event.button)
end)

backwards_compat("tile_interaction", "handle_tile_interaction", function(event)
  handle_tile_interaction(event.player_id, event.x, event.y, event.z, event.button)
end)

backwards_compat("textbox_response", "handle_textbox_response", function(event)
  handle_textbox_response(event.player_id, event.response)
end)


if handle_board_open then
  warn("handle_board_open() is deprecated")

  Net:on("board_open", function(event)
    handle_board_open(event.player_id)
  end)
end

backwards_compat("board_close", "handle_board_close", function(event)
  handle_board_close(event.player_id)
end)

backwards_compat("post_request", "handle_post_request", function(event)
  handle_post_request(event.player_id)
end)

backwards_compat("post_selection", "handle_post_selection", function(event)
  handle_post_selection(event.player_id, event.post_id)
end)

backwards_compat("shop_close", "handle_shop_close", function(event)
  handle_shop_close(event.player_id)
end)

backwards_compat("shop_purchase", "handle_shop_purchase", function(event)
  handle_shop_purchase(event.player_id, event.item_name)
end)

backwards_compat("battle_results", "handle_battle_results", function(event)
  handle_battle_results(event.player_id, event)
end)

backwards_compat("server_message", "handle_server_message", function(event)
  handle_server_message(event.host, event.port, event.data)
end)
