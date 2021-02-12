nonce = function() end 

function LoadTexture(path)
    return Engine.ResourceHandle.new().Textures:LoadFile(path)
end

function StreamMusic(path, loop)
    return Engine.ResourceHandle.new().Audio:Stream(path, loop)
end

local texture = nil
local shake = false
local waitTime = 1
local hand = nil
local middle = nil
local miscAnim = nil
local anim = nil
local dir = Direction.Up

function Missile()
    local missile = Battle.Spell.new(Team.Blue)
    missile:SetTexture(texture, true)
    missile:SetSlideFrames(80)
    missile:SetHeight(20.0)
    missile:ShowShadow(true)

    local missileAnim = missile:GetAnimation()
    missileAnim:CopyFrom(anim)
    missileAnim:SetState("MISSILE", Playback.Loop, nonce)

    missile.updateFunc = function(self, dt) 

        if self:IsSliding() == false then
            -- keep sliding
            self:SlideToTile(true)
            self:Move(Direction.Left)
        end 

        -- every frame try to attack shared tile
        self:Tile():AttackEntities(self)
    end

    missile.attackFunc = function(self, other) 
        self:Delete()
    end

    missile.deleteFunc = function(self) 
        -- todo: spawn explosion
    end

    missile.canMoveToFunc = function(tile)
        -- missiles fly over any tiles
        return true
    end

    return missile
end

function battle_init(self) 
    texture = LoadTexture(_modpath.."duo_compressed.png")

    print("modpath: ".._modpath)
    self:SetName("Duo")
    self:SetHealth(300)
    self:SetTexture(texture, true)
    self:SetHeight(60)
    self:SetSlideFrames(120)
    self:ShareTile(true)

    anim = self:GetAnimation()
    anim:SetPath(_modpath.."duo_compressed.animation")
    anim:Load()
    anim:SetState("BODY", Playback.Once, nonce)

    hand = Engine.SpriteNode.new()
    hand:SetTexture(texture, true)
    hand:SetPosition(0, 15.0)
    hand:SetLayer(-2) -- put it in front at all times
    self:AddNode(hand)

    miscAnim = Engine.Animation.new(anim:Copy())
    miscAnim:SetState("HAND_IDLE")
    miscAnim:Refresh(hand:Sprite())

    middle = Engine.SpriteNode.new()
    middle:SetTexture(texture, true)
    middle:SetLayer(-1)

    miscAnim:SetState("NO_SHOOT")
    miscAnim:Refresh(middle:Sprite())

    local origin = anim:Point("origin")
    local point  = anim:Point("NO_SHOOT")
    middle:SetPosition(point.x - origin.x, point.y - origin.y)
    self:AddNode(middle)

    print("done")
end

function num_of_explosions()
    return 12
end

function on_update(self, dt)
    if self:IsSliding() == false then
        -- we arrive, shake the ground
        
        if shake then
            self:ShakeCamera(3.0, 1.0)
            shake = false
        end
        
        waitTime = waitTime - dt
        
        -- pause before moving again
        if waitTime <= 0 then
            -- spawn a missile 
            self:Field():SpawnSpell(Missile(), self:Tile():X(), self:Tile():Y())
            print("field spawn")

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

            waitTime = 1
            shake = true -- can shake again
            self:SlideToTile(true)
            self:Move(dir)
        end
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