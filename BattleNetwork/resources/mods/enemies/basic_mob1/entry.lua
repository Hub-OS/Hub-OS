function package_requires_scripts()
  -- Note: `requires_character` will throw if unable to find or load
  Engine.requires_character("com.builtins.char.mettaur")
end

function package_init(package) 
  package:declare_package_id("com.example.mob.basic_mob1")
  package:set_name("Basic mob")
end

function package_build(mob) 
  local met_spawner = mob:create_spawner("com.builtins.char.mettaur")

  math.randomseed(Engine.get_rand_seed())

  local rand = math.random(0, 1)

  if rand == 0 then 
    met_spawner:spawn_at(4, 1)
    met_spawner:spawn_at(5, 2)
    met_spawner:spawn_at(6, 3)
  else 
    met_spawner:spawn_at(4, 1)
    met_spawner:spawn_at(4, 3)
  end
end