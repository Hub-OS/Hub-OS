function package_requires_scripts()
  -- Note: `requires_character` will throw if unable to find or load
  Engine.requires_character("com.example.char.Duo")
  Engine.requires_character("com.builtins.char.mettaur")
  
  -- Note: uncommenting out will test the engine's ability to reject bad scripts
  -- Engine.requires_character("does.not.exist") 
end

function package_init(package) 
  package:declare_package_id("com.example.mob.test")
  package:set_name("Test Mob")
  package:set_description("this mob uses a scripted character define elsewhere!")
  package:set_speed(999)
  package:set_attack(999)
  package:set_health(9999)
  package:set_preview_texture_path(_modpath.."preview.png")
end

function package_build(mob) 
  local duo_spawner = mob:create_spawner("com.example.char.Duo")
  duo_spawner:spawn_at(5, 2)
  
  local met_spawner = mob:create_spawner("com.builtins.char.mettaur")
  met_spawner:spawn_at(4, 1)
  met_spawner:spawn_at(4, 3)
end