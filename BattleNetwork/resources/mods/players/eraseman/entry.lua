function LoadTexture(path)
    return Engine.ResourceHandle.new().Textures:LoadFile(path)
end

function roster_init(info) 
    print("modpath: ".._modpath)
    info:SetSpecialDescription("Scripted net navi!")
    info:SetSpeed(2.0)
    info:SetAttack(2)
    info:SetChargedAttack(20)
    info:SetOverworldAnimationPath(_modpath.."erase_OW.animation")
    info:SetIconTexture(LoadTexture(_modpath.."eraseman_face.png"))
    info:SetPreviewTexture(LoadTexture(_modpath.."preview.png"))
    info:SetOverworldTexture(LoadTexture(_modpath.."erase_OW.png"))
end

function battle_init(player)
    player:SetName("Eraseman")
    player:SetHealth(1100)
    player:SetElement(Battle.Element.Cursor)
    player:SetHeight(100.0)
    player:GetAnimation():SetPath(_modpath.."eraseman.animation")
    player:SetTexture(LoadTexture(_modpath.."navi_eraseman_atlas.png"), true)
    player:SetFullyChargeColor(Color.new(255,0,0,255))
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
    return Battle.Buster.new(player, true, 100)
end