local character_id = "BattleNetwork3.Canodumb.Enemy"

function encounter_init(encounter)
    encounter
        :create_spawner(character_id, Rank.V1)
        :spawn_at(4, 2)

    encounter
        :create_spawner(character_id, Rank.V2)
        :spawn_at(6, 1)

    encounter
        :create_spawner(character_id, Rank.V3)
        :spawn_at(6, 3)
end
