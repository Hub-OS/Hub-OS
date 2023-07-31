local Encounters = require("scripts/libs/encounters")

-- Setup the default area's encounters
local encounters_table = {}
encounters_table["default"] = {
  -- min_travel = 8, -- 8 tiles are free to walk after each encounter
  -- chance = .05,   -- 5% chance tested at every tile's worth of movement
  chance = function(distance)
    -- curve 5: https://pastebin.com/vXbWiKAc
    local index = math.floor(distance / Encounters.CHECK_DISTANCE)
    local chance_curve = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 8, 10, 12 }

    local chance = chance_curve[index] or chance_curve[#chance_curve]
    return chance / 32
  end,
  preload = true,
  encounters = {
    {
      asset_path = "/server/assets/encounters/canodumb.zip",
      weight = 0.1
    }
  }
}

Encounters.setup(encounters_table)

Net:on("player_join", function(event)
  Encounters.track_player(event.player_id)
end)

Net:on("player_disconnect", function(event)
  -- Drop will forget this player entry record
  -- Also useful to stop tracking players for things like cutscenes or
  -- repel items
  Encounters.drop_player(event.player_id)
end)

Net:on("player_move", function(event)
  Encounters.handle_player_move(event.player_id, event.x, event.y, event.z)
end)
