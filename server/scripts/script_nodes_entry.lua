local ScriptNodes = require("scripts/libs/script_nodes")

local scripts = ScriptNodes:new()

-- define consumable items
Net.register_item("MiniEnrg", {
  name = "MiniEnrg",
  description = "Recovers 50HP!",
  consumable = true
})

Net.register_item("FullEnrg", {
  name = "FullEnrg",
  description = "Recovers max HP!",
  consumable = true
})

Net.register_item("SneakRun", {
  name = "SneakRun",
  description = "No weak viruses for a while.",
  consumable = true
})

-- load areas after defining items (allows shops to cache item names)
for _, area_id in ipairs(Net.list_areas()) do
  scripts:load(area_id)
end

-- define universal handlers (avoid to use custom handling for each area)
local item_use_map = {
  MiniEnrg = function(event)
    local health = Net.get_player_health(event.player_id)
    local max_health = Net.get_player_max_health(event.player_id)
    Net.set_player_health(event.player_id, math.min(health + 50, max_health))
  end,
  FullEnrg = function(event)
    local max_health = Net.get_player_max_health(event.player_id)
    Net.set_player_health(event.player_id, max_health)
  end,
  -- SneakRun is handled per map.
}

Net:on("item_use", function(event)
  local callback = item_use_map[event.item_id]

  if callback then
    Net.give_player_item(event.player_id, event.item_id, -1)
    callback(event)
  end
end)

-- handle battle results (never called if `Forget Results` is set to true on `Encounter` nodes)
scripts:on_encounter_result(function(result)
  local player_id = result.player_id

  if result.health <= 0 then
    -- handle deletion
    Net.set_player_health(player_id, 1)
    return
  end

  local health = math.min(Net.get_player_max_health(player_id), result.health)
  Net.set_player_health(player_id, health)
  Net.set_player_emotion(player_id, result.emotion)
end)

scripts:on_inventory_update(function(player_id, item_id)
  -- update save data
end)

scripts:on_money_update(function(player_id)
  -- update save data
end)

-- create custom script nodes
-- scripts:implement_node("custom", function(context, object)
--   scripts:execute_next_node(context, context.area_id, object)
-- end)

-- listen to areas added by :load(), including dynamically instanced areas
-- scripts:on_load(function(area_id)
-- end)
