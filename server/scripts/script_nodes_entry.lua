local ScriptNodes = require("scripts/libs/script_nodes")

local scripts = ScriptNodes:new()

for _, area_id in ipairs(Net.list_areas()) do
  scripts:load(area_id)
end

scripts:on_encounter_result(function(result)
  local player_id = result.player_id

  local health = math.min(Net.get_player_max_health(player_id), result.health)
  Net.set_player_health(player_id, health)
  Net.set_player_emotion(player_id, result.emotion)

  -- update save data
end)

scripts:on_inventory_update(function(player_id, item_id)
  -- update save data
end)

-- create custom script nodes
-- scripts:implement_script_node("custom node", function(context, object)
--   scripts:execute_next_node(context, context.area_id, object)
-- end)
