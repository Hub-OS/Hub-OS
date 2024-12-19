local canodumb_id = "BattleNetwork3.Canodumb.Enemy"
local mettaur_id = "BattleNetwork6.Mettaur.Enemy"

local layouts = {
  function(encounter)
    local mettaur_v1 = encounter:create_spawner(mettaur_id, Rank.V1)
    local canodumb_v2 = encounter:create_spawner(canodumb_id, Rank.V2)

    mettaur_v1:spawn_at(4, 1)
    canodumb_v2:spawn_at(5, 2)
    mettaur_v1:spawn_at(6, 3)
  end,

  function(encounter)
    local mettaur_v1 = encounter:create_spawner(mettaur_id, Rank.V1)
    local mettaur_v3 = encounter:create_spawner(mettaur_id, Rank.V3)
    local canodumb_v3 = encounter:create_spawner(canodumb_id, Rank.V3)

    mettaur_v1:spawn_at(4, 2)
    mettaur_v3:spawn_at(6, 2)
    canodumb_v3:spawn_at(5, 3)
  end,

  function(encounter)
    local mettaur_v2 = encounter:create_spawner(mettaur_id, Rank.V2)
    local canodumb_v1 = encounter:create_spawner(canodumb_id, Rank.V1)
    local canodumb_v3 = encounter:create_spawner(canodumb_id, Rank.V3)

    canodumb_v3:spawn_at(4, 2)
    mettaur_v2:spawn_at(6, 1)
    canodumb_v1:spawn_at(6, 3)
  end,
}

function encounter_init(encounter)
  local layout = layouts[math.random(3)]
  layout(encounter)
end
