function roster_init(info) 
  info:SetName("Kool Konst")
  info:SetDescription("crazy konst makes a kool enemy mod")
  info:SetSpeed(999)
  info:SetAttack(999)
  info:SetHP(9999)
end

function load_scripts()
  -- Note: RequiresCharacter will throw if unable to find or load
  Engine.RequiresCharacter("Example.Duo")
end

function build(mob) 
  local duo_spawner = mob:CreateSpawner("Example.Duo")
  duo_spawner:SpawnAt(5, 2)
  duo_spwaner:SpawnAt(6, 3)
end