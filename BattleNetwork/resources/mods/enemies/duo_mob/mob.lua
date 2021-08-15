function load_scripts()
    -- Tries to fetch the scripted character from the engine by namespace
    -- Note: most scripts won't have access to the files outside of the mod
    --       so it is expected that the fetch might fail and we do not spawn the enemy
    --       TODO: mark this file for loading later until all scripts are read to resolve this issue?

    -- Creates a new namespace to the script and contents
    -- Note: if conflicts will throw
    Engine.define_character("Example.Duo", _modpath.."characters/duo")
end

function roster_init(info) 
    info:set_name("Dangerous Duo")
    info:set_description("Duo is back too? The nightmare never ends!")
    info:set_speed(1)
    info:set_attack(100)
    info:set_health(3000)
    info:set_preview_texture_path(_modpath.."preview.png")
end

function build(mob) 
    print("_modpath is: ".._modpath)
    local texPath = _modpath.."background.png"
    local animPath = _modpath.."background.animation"
    mob:set_background(texPath, animPath, -1.4, 0.0)
    mob:stream_music(_modpath.."music.ogg", 6579, 76000)

    for i=1,3 do
        mob:get_field():tile_at(5, i):set_state(TileState.Hidden)
        mob:get_field():tile_at(6, i):set_state(TileState.Hidden)
    end

    local duo_spawner = mob:create_spawner("Example.Duo")
    -- duo_spawner:spawn_at(5,2,Duo.custom_intro)
    duo_spawner:spawn_at(5, 2)
end