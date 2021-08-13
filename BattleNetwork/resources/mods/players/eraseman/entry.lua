function LoadTexture(path)
    return Engine.ResourceHandle.new().Textures:LoadFile(path)
end

function roster_init(info) 
    print("modpath: ".._modpath)
    info:DeclarePackage("com.example.player.Eraseman")
    info:SetSpecialDescription("Scripted net navi!")
    info:SetSpeed(2.0)
    info:SetAttack(2)
    info:SetChargedAttack(20)
    info:SetIconTexture(LoadTexture(_modpath.."eraseman_face.png"))
    info:SetPreviewTexture(LoadTexture(_modpath.."preview.png"))
    info:SetOverworldAnimationPath(_modpath.."erase_OW.animation")
    info:SetOverworldTexturePath(_modpath.."erase_OW.png")
    info:SetMugshotTexturePath(_modpath.."mug.png")
    info:SetMugshotAnimationPath(_modpath.."mug.animation")
end

function battle_init(player)
    player:SetName("Eraseman")
    player:SetHealth(1100)
    player:SetElement(Element.Cursor)
    player:SetHeight(100.0)
    print("here 1")
    player:SetAnimation(_modpath.."eraseman.animation")
    print("here 2")
    player:SetTexture(LoadTexture(_modpath.."navi_eraseman_atlas.png"), true)
    print("here 3")
    player:SetFullyChargeColor(Color.new(255,0,0,255))
    print("here 4")
end

function execute_special_attack(player)
    print("execute special")
    return nil
end

function execute_buster_attack(player)
    print("buster attack")
    return Battle.Buster.new(player, false, 10)
end

function execute_charged_attack(player)
    print("charged attack")
    return special_card_action(player)
end

function spawn_attack(user, tile)
    local spell = Battle.Spell.new(user:GetTeam())

    spell:SetHitProps(MakeHitProps(
        50, 
        Hit.Impact | Hit.Drag | Hit.Flinch, 
        Element.Cursor, 
        user:GetID(), 
        Drag(Direction.Right, 1)
        )
    )

    user:GetField():Spawn(spell, tile:X(), tileY())

    return spell
end

function special_card_action(user) 
    local action = Battle.CardAction(user, "PLAYER_SPECIAL")
    action:SetLockout(LockType.Animation)

    action.executeFunc = function(self, actor)
        local do_attack = function()
            local t1 = user:GetTile(Direction.Right, 1)
            local t2 = user:GetTile(Direction.Right, 2)

            spawn_attack(user, t1:X(), t1:Y())
            spawn_attack(user, t2:X(), t2:Y())
            spawn_attack(user, t2:X(), t2:Y()-1)
            spawn_attack(user, t2:X(), t2:Y()+1)
        end

        self:AddAnimAction(3, do_attack)
    end

    return action
end