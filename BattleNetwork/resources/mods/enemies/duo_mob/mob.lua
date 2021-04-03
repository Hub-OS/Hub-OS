function load_scripts()
    -- Tries to fetch the scripted character from the engine by namespace
    -- Note: most scripts won't have access to the files outside of the mod
    --       so it is expected that the fetch might fail and we do not spawn the enemy
    --       TODO: mark this file for loading later until all scripts are read to resolve this issue?

    -- Creates a new namespace to the script and contents
    -- Note: if conflicts will throw
    print("Before defined character!")
    Engine.DefineCharacter("Example.Duo", _modpath.."characters/duo")
    print("Defined character!")
end

function roster_init(info) 
    info:SetName("Dangerous Duo")
    info:SetDescription("Duo is back too? The nightmare never ends!")
    info:SetSpeed(1)
    info:SetAttack(100)
    info:SetHP(3000)
end

function build(mob) 
    print("_modpath is: ".._modpath)
    local texPath = _modpath.."background.png"
    local animPath = _modpath.."background.animation"
    mob:SetBackground(texPath, animPath, -1.4, 0.0)

    print("Background set")

    mob:StreamMusic(_modpath.."music.ogg")

    print("Music set!")

    for i=1,3 do
        mob:Field():TileAt(5, i):SetState(TileState.Hidden)
        mob:Field():TileAt(6, i):SetState(TileState.Hidden)
    end

    print("field set!")

    local duo_spawner = mob:CreateSpawner("Example.Duo")

    print("spawner created!")

    -- duo_spawner:SpawnAt(5,2,Duo.custom_intro)
    duo_spawner:SpawnAt(5, 2)

    print("Duo spawned")
end