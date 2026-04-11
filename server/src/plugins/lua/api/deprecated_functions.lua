local function backwards_compat(table, old_name, new_name)
  table[old_name] = function(...)
    warn(old_name .. "() is deprecated, use " .. new_name .. "() instead.")
    return table[new_name](...)
  end
end

backwards_compat(Net, "get_song", "get_music")
backwards_compat(Net, "set_song", "set_music")
backwards_compat(Net, "get_width", "get_layer_width")
backwards_compat(Net, "get_height", "get_layer_height")
backwards_compat(Net, "close_bbs", "close_board")
