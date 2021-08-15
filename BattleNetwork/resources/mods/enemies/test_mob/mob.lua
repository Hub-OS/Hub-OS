function load_scripts()
  -- Note: `requires_character` will throw if unable to find or load
  Engine.requires_character("Example.Duo")
  Engine.requires_character("BuiltIns.Mettaur")
  Engine.requires_character("BuiltIns.Canodumb")
end

function roster_init(info) 
  info:set_name("Test Mob")
  info:set_description("this mob uses a scripted character define elsewhere!")
  info:set_speed(999)
  info:set_attack(999)
  info:set_health(9999)
  info:set_preview_texture_path(_modpath.."preview.png")
end

function build(mob) 
  local duo_spawner = mob:create_spawner("Example.Duo")
  duo_spawner:spawn_at(5, 2)
  
  local met_spawner = mob:create_spawner("BuiltIns.Canodumb")
  met_spawner:spawn_at(4, 1)
  met_spawner:spawn_at(4, 3)
end