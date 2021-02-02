local handle = Engine.ResourceHandle.new()
local path = "resources/mods/players/eraseman/"

function load_info(info) 
    info:SetSpecialDescription("Scripted net navi!")
    info:SetSpeed(2.0)
    info:SetAttack(2)
    info:SetChargedAttack(20)
    info:SetOverworldAnimationPath(path.."erase_OW.animation")
    info:SetIconTexture(handle.Textures:LoadTextureFromFile(path.."eraseman_face.png"))
    info:SetPreviewTexture(handle.Textures:LoadTextureFromFile(path.."preview.png"))
    info:SetOverworldTexture(handle.Textures:LoadTextureFromFile(path.."erase_OW.png"))
end

function load_player(player)
    player:SetName("Eraseman")
    player:SetHealth(1100)
    player:GetAnimation():SetPath(path.."eraseman.animation")
    player:SetTexture(handle.Textures:LoadTextureFromFile(path.."navi_eraseman_atlas.png"), true)
end