function load_scripts()
  -- Note: RequiresCharacter will throw if unable to find or load
  Engine.RequiresCharacter("Example.Duo")
  Engine.RequiresCharacter("BuiltIns.Mettaur")
end

function roster_init(info) 
  info:SetName("Kool Konst")
  info:SetDescription("crazy konst makes a kool enemy mod")
  info:SetSpeed(999)
  info:SetAttack(999)
  info:SetHP(9999)
end

function build(mob) 
  local duo_spawner = mob:CreateSpawner("Example.Duo")
  duo_spawner:SpawnAt(5, 2)
  
  local met_spawner = mob:CreateSpawner("BuiltIns.Mettaur")
  met_spawner:SpawnAt(4, 1)
  met_spawner:SpawnAt(4, 3)
end