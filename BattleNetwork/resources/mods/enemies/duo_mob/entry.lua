function package_requires_scripts()
    -- Tries to fetch the scripted character from the engine by namespace
    -- Note: most scripts won't have access to the files outside of the mod
    --       so it is expected that the fetch might fail and we do not spawn the enemy
    --       TODO: mark this file for loading later until all scripts are read to resolve this issue?

    -- Creates a new namespace to the script and contents
    -- Note: if conflicts will throw
    Engine.define_character("com.example.char.Duo", _modpath.."characters/duo")
end

function package_init(package)
    package:declare_package_id("com.example.mob.duobossfight")
    package:set_name("Dangerous Duo")
    package:set_description("Duo is back too? The nightmare never ends!")
    package:set_speed(1)
    package:set_attack(100)
    package:set_health(3000)
    package:set_preview_texture_path(_modpath.."preview.png")
end

function package_build(mob) 
    local texPath = _modpath.."background.png"
    local animPath = _modpath.."background.animation"
    mob:set_background(texPath, animPath, -1.4, 0.0)
    mob:stream_music(_modpath.."music.ogg", 6579, 76000)

    for i=1,3 do
        mob:get_field():tile_at(5, i):set_state(TileState.Hidden)
        mob:get_field():tile_at(6, i):set_state(TileState.Hidden)
    end

    local duo_spawner = mob:create_spawner("com.example.char.Duo", Rank.V1)
    -- duo_ex_spawner = mob:create_spawner("com.example.char.Duo", Rank.EX)
    -- duo_spawner:spawn_at(5,2,Duo.custom_intro)
    duo_spawner:spawn_at(5, 2)
end