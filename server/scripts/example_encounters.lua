local Encounters = require("scripts/libs/encounters")

-- Setup the default area's encounters
local encounters_table = {}
encounters_table["default"] = {
  min_travel = 2, -- 2 tiles are free to walk after each encounter
  chance = .05, -- 5% chance for each movement event after the minimum travel to cause an encounter
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
