function LoadTexture(path)
    return Engine.ResourceHandle.new().Textures:LoadFile(path)
end

function StreamMusic(path, loop)
    return Engine.ResourceHandle.new().Audio:Stream(path, loop)
end

nonce = function() end 

local hand = nil
local handAnim = nil
local dir = Direction.Up

function battle_init(self) 
    print("modpath: ".._modpath)
    local texture = LoadTexture(_modpath.."duo_compressed.png")

    self:SetName("Duo")
    self:SetHealth(3000)
    self:SetTexture(texture, true)
    self:SetHeight(60)
    self:SetSlideFrames(180)
    self:ShareTile(true)

    local anim = self:GetAnimation()
    anim:SetPath(_modpath.."duo_compressed.animation")
    anim:Load()
    anim:SetState("BODY", Playback.Once, nonce)

    hand = Engine.SpriteNode.new()
    hand:SetTexture(texture, true)
    hand:SetPosition(10.0, 10.0)
    hand:SetLayer(-1) -- put it in front at all times
    self:AddNode(hand)

    handAnim = Engine.Animation.new(anim:Copy())
    handAnim:SetState("HAND_IDLE")
    handAnim:Refresh(hand:Sprite())

    print("done")
end

function num_of_explosions()
    return 12
end

function on_update(self, dt)
    -- print("on_update tick")

    if self:IsSliding() == false then
        local tile = nil
        
        if dir == Direction.Up then 
            tile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()-1)

            if can_move_to(tile) == false then 
                dir = Direction.Down
            end
        elseif dir == Direction.Down then
            tile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()+1)

            if can_move_to(tile) == false then
                dir = Direction.Up
            end
        end

        self:SlideToTile(true)
        self:Move(dir)
    end 
end

function on_battle_start(self)
   print("on_battle_start called")
end

function on_battle_end(self)
    print("on_battle_end called")
end

function on_spawn(self, tile) 
    StreamMusic(_modpath.."music.ogg", true)
    print("on_spawn called")
end

function can_move_to(tile) 
    if tile:IsEdge() then
        return false
    end

    return true
end