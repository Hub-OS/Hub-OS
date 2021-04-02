function roster_init(info) 
    info:SetName("Dangerous Duo")
    info:SetDescription("... Duo is back too? The nightmare never ends!")
    info:SetSpeed(1)
    info:SetAttack(100)
    info:SetHP(3000)
end

function load_scripts()
    -- Tries to fetch the scripted character from the engine by namespace
    -- Note: most scripts won't have access to the files outside of the mod
    --       so it is expected that the fetch might fail and we do not spawn the enemy
    --       TODO: mark this file for loading later until all scripts are read to resolve this issue?

    -- Creates a new namespace to the script and contents
    -- Note: if conflicts will throw
    Engine.DefinesCharacter("Example.Duo", _modpath.."characters/duo")
end

function build(mob) 
    local texPath = _modpath.."background.png"
    local animPath = _modpath.."background.animation"
    mob:SetBackground(textPath, animPath, -2.4, 0.0)

    mob:StreamMusic(_modpath.."music.ogg")

    for local i=1,3 do
        mob:Field():TileAt(5, i):SetState(TileState.Hidden)
        mob:Field():TileAt(6, i):SetState(TileState.Hidden)
    end

    local duo_spawner = mob:CreateSpawner("Example.Duo")
    -- duo_spawner:SpawnAt(5,2,Duo.custom_intro)
    duo_spawner:SpawnAt(5, 2)

    -- NOTE: comment out for 2 Duos!
    duo_spwaner:SpawnAt(6, 3)
end