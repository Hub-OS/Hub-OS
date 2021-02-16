nonce = function() end 

function LoadTexture(path)
    return Engine.ResourceHandle.new().Textures:LoadFile(path)
end

function StreamMusic(path, loop)
    return Engine.ResourceHandle.new().Audio:Stream(path, loop)
end

local MINE_SLIDE_FRAMES = 30 -- frames
local IDLE_TIME = 1 -- seconds inbetween moves 
local texture = nil
local shake = false
local waitTime = nil -- timer used in MoveState
local hand = nil
local middle = nil
local miscAnim = nil
local anim = nil
local dir = Direction.Up
local laserComplete = true
local laserOpening = false
local handTimer = 0
local aiStateIndex = 1 
local aiState = {} -- setup in the battle_init function
local idleHandPos = {x=0, y=15.0}
local idleDuoPos = {x=-15.0, y=0}

function UpwardFist(duo) 
    local fist = Battle.Spell.new(Team.Blue)
    fist:SetTexture(texture, true)
    fist:HighlightTile(Highlight.Flash)
    fist:SetSlideFrames(4)

    fist:SetHitProps(HitProps(
        50, 
        Hit.Impact | Hit.Recoil | Hit.Flinch, 
        Element.None, 
        duo, 
        Direction.None)
    )

    local fistAnim = fist:GetAnimation()
    fist.startSwinging = false

    fistAnim:CopyFrom(anim)
    fistAnim:SetState("HAND_SWIPE_BOTTOM_UP", Playback.Once, nonce)

    local closure = function() 
        local fistRef = fist
        return function()
            fistRef:HighlightTile(Highlight.Solid)
            fistRef.startSwinging = true
        end 
    end 

    fistAnim:OnFrame(2, closure(), true)

    fist.updateFunc = function(self, dt) 
        self:Tile():AttackEntities(self)

        if self:IsSliding() == false then 
            if self:Tile():IsEdge() then 
                self:Delete()
            end 

            if self.startSwinging == true then
                self:SlideToTile(true)
                self:Move(Direction.Up)
            end
        end
    end

    fist.attackFunc = function(self, other) 
        -- nothing
    end

    fist.deleteFunc = function(self) 
    end

    fist.canMoveToFunc = function(tile)
        return true
    end

    return fist
end 

function DownwardFist(duo) 
    local fist = Battle.Spell.new(Team.Blue)
    fist:SetTexture(texture, true)
    fist:SetSlideFrames(4)

    fist:SetHitProps(HitProps(
        50, 
        Hit.Impact | Hit.Recoil | Hit.Flinch, 
        Element.None, 
        duo, 
        Direction.None)
    )

    local fistAnim = fist:GetAnimation()
    fist.startSwinging = false

    fistAnim:CopyFrom(anim)
    fistAnim:SetState("HAND_SWIPE_TOP_DOWN", Playback.Once, nonce)

    local closure = function() 
        local fistRef = fist
        return function()
            fistRef.startSwinging = true
            fistRef:HighlightTile(Highlight.Solid)
        end 
    end 

    fistAnim:OnFrame(2, closure(), true)

    fist.updateFunc = function(self, dt) 
        self:Tile():AttackEntities(self)

        if self:IsSliding() == false then 
            if self:Tile():Y() == 4 then 
                self:Delete()
            end 

            if self.startSwinging == true then
                self:SlideToTile(true)
                self:Move(Direction.Down)
            end
        end
    end

    fist.attackFunc = function(self, other) 
        -- nothing
    end

    fist.deleteFunc = function(self) 
    end

    fist.canMoveToFunc = function(tile)
        return true
    end

    return fist
end 

function Mine(duo) 
    local mine = Battle.Obstacle.new(Team.Blue)
    mine:SetHealth(20)
    mine:SetSlideFrames(MINE_SLIDE_FRAMES)

    mine:SetHitProps(HitProps(
        20, 
        Hit.Impact | Hit.Recoil | Hit.Flinch, 
        Element.None, 
        duo, 
        Direction.None)
    )

    mine.dir = Direction.Up
    mine.verticalDir = Direction.Up
    mine.moveOneColumn = false
    mine.timer = 0
    mine.waitTimer = 1 -- second
    mine.node = Engine.SpriteNode.new()
    mine.node:SetTexture(texture, true)
    mine.node:SetPosition(0,0)
    mine:AddNode(mine.node)

    miscAnim:SetState("MINE", Playback.Once, nonce)
    miscAnim:Refresh(mine.node:Sprite())

    mine.updateFunc = function(self, dt) 
        local tile = self:Tile()

        -- every frame try to attack shared tile
        tile:AttackEntities(self)

        local tileW = tile:Width()/2.0 -- downscale 2x
        local offset = math.min(1.0, ((self.timer*60.0)/((MINE_SLIDE_FRAMES*2.0)+1.0)))
        local x = (-offset*tileW)+(tileW/2.0)

        self.node:SetPosition(x, 0.0)

        self.waitTimer = self.waitTimer - dt
        if(self.waitTimer > 0) then 
            return 
        end

        self.timer = self.timer + dt

        if self:IsSliding() == false then
            local nextTile = nil
            
            if self.dir == Direction.Up then
                nextTile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()-1)
            elseif self.dir == Direction.Down then 
                nextTile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()+1)
            end

            if nextTile == nil then 
                self:Delete()
                return
            end

            if nextTile:IsEdge() and self.moveOneColumn == false then
                    -- we must be moving up/down
                    self.moveOneColumn = true
                    self.timer = 0
                    self.node:SetPosition(tileW/2.0, 0.0)

                    if not self:Move(Direction.Left) then 
                        self:Delete()
                        return
                    end

                    self:AdoptNextTile()
                    self:FinishMove()
                    return
            elseif self.moveOneColumn == true then
                if self.verticalDir == Direction.Up then
                    self.verticalDir = Direction.Down
                    self.dir = Direction.Down 
                else 
                    self.dir = Direction.Up
                    self.verticalDir = Direction.Up 
                end

                self.moveOneColumn = false
            end

            -- otherwise keep sliding
            self:SlideToTile(true)
            self:Move(self.dir)
        end 
    end

    mine.attackFunc = function(self, other) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():SpawnFX(explosion, self:Tile():X(), self:Tile():Y())
        self:Delete()
    end

    mine.deleteFunc = function(self) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():SpawnFX(explosion, self:Tile():X(), self:Tile():Y())
    end

    mine.canMoveToFunc = function(tile)
        -- mines fly over any tiles
        return true
    end

    return mine
end

function LaserBeam(duo)
    laserComplete = false
    local laser = Battle.Spell.new(Team.Blue) 

    laser:SetTexture(texture, true)
    laser:SetLayer(-2)

    laser:SetHitProps(HitProps(
        100, 
        Hit.Impact | Hit.Recoil | Hit.Flinch, 
        Element.None, 
        duo, 
        Direction.None)
    )

    local origin = duo:GetAnimation():Point("origin")
    local point  = duo:GetAnimation():Point("NO_SHOOT")
    laser:SetPosition(point.x - origin.x + 20, point.y - origin.y)

    local laserAnim = laser:GetAnimation()
    laserAnim:CopyFrom(anim)

    local onComplete = function() 
        local laserRef = laser
        local animRef = laserAnim
        return function()
            laserRef.laserCount = 0
            animRef:SetState("LASER_BEAM", Playback.Loop, function() 
                laserRef.laserCount = laserRef.laserCount + 1

                if laserRef.laserCount > 40 then
                    animRef:SetState("LASER_BEAM_OPEN", Playback.Reverse, function()
                        laserComplete = true
                        laserRef:Delete()
                    end)
                end
            end)
        end 
    end 

    laserAnim:SetState("LASER_BEAM_OPEN", Playback.Once, onComplete())

    laser.updateFunc = function(self, dt) 
        self:Tile():AttackEntities(self)
    end

    laser.attackFunc = function(self, other) 
        -- nothing
    end

    laser.deleteFunc = function(self) 
    end

    laser.canMoveToFunc = function(tile)
        return false
    end

    return laser
end

function Missile(duo)
    local missile = Battle.Obstacle.new(Team.Blue)
    missile:SetHealth(20)
    missile:SetTexture(texture, true)
    missile:SetSlideFrames(20)
    missile:SetHeight(20.0)
    missile:SetLayer(-2)
    missile:ShowShadow(false)
    missile.waitTimer = 1 -- second

    missile:SetHitProps(HitProps(
        30, 
        Hit.Impact | Hit.Recoil | Hit.Flinch, 
        Element.None, 
        duo, 
        Direction.None)
    )

    local missileAnim = missile:GetAnimation()
    missileAnim:CopyFrom(anim)
    missileAnim:SetState("MISSILE", Playback.Loop, nonce)

    missile.updateFunc = function(self, dt) 
        self.waitTimer = self.waitTimer - dt
        if(self.waitTimer > 0) then 
            return 
        end

        if self:IsSliding() == false then
            if self:Tile():IsEdge() then 
                self:Delete()
            end

            -- otherwise keep sliding
            self:SlideToTile(true)
            self:Move(Direction.Left)
        end 

        if self:Tile():IsHole() == false then 
            missile:ShowShadow(true)
        end

        -- every frame try to attack shared tile
        self:Tile():AttackEntities(self)
    end

    missile.attackFunc = function(self, other) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():SpawnFX(explosion, self:Tile():X(), self:Tile():Y())
        self:Delete()
    end

    missile.deleteFunc = function(self) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():SpawnFX(explosion, self:Tile():X(), self:Tile():Y())
    end

    missile.canMoveToFunc = function(tile)
        -- missiles fly over any tiles
        return true
    end

    return missile
end

function NextState()
    -- increment our AI state counter
    aiStateIndex = aiStateIndex + 1
    if aiStateIndex > #aiState then 
        aiStateIndex = 1
    end
end

function RestoreHand(self, dt) 
    hand:Show()

    pos = hand:GetPosition()

    -- slide in
    hand:SetPosition(pos.x - (dt*5*60), pos.y + (dt*2*60))

    pos = self:GetPosition()

    self:SetPosition(pos.x - (dt*40), pos.y)

    if hand:GetPosition().x <= 0 then
        self:SetPosition(idleDuoPos.x, idleDuoPos.y)
        hand:SetPosition(idleHandPos.x, idleHandPos.y)
        NextState()
    end
end

function MoveHandAway(self, dt) 
    local pos = hand:GetPosition()

    -- slide out and up 5px a frame
    hand:SetPosition(pos.x + (dt*5*60), pos.y - (dt*2*60))

    pos = self:GetPosition()

    self:SetPosition(pos.x + (dt*40), pos.y)

    if hand:GetPosition().x > 92 then
        hand:Hide()
        NextState()
    end
end

function ShootLaserState(self, dt)
    if miscAnim:State() ~= "CHEST_GUN" then
        laserOpening = true

        function onComplete() 
            local duoRef = self 
            return function() 
                -- laserComplete gets toggled to false when a laser beam is created
                duoRef:Field():Spawn(LaserBeam(duoRef), duoRef:Tile():X()-1, duoRef:Tile():Y())
                NextState()
            end 
        end

        miscAnim:SetState("CHEST_GUN")
        miscAnim:SetPlayback(Playback.Once)
        miscAnim:OnComplete(onComplete())
    end

    miscAnim:Update(dt, middle:Sprite(), 1.0)
end

function ShootMissileState(self, dt)
    self:Field():Spawn(Missile(self), self:Tile():X()-1, self:Tile():Y())
    NextState()
end 

function ShootMineState(self, dt) 
    self:Field():Spawn(Mine(self), self:Tile():X()-1, self:Tile():Y())
    NextState()
end 

function DownwardFistState(self, dt)
    self:Field():Spawn(DownwardFist(self), 2, 0)

    NextState()
end

function UpwardFistState(self, dt)
    -- back-left corner
    self:Field():Spawn(UpwardFist(self), 1, 3)

    NextState()
end

function LaserState(self, dt)
    NextState()
end

function FaceAttackState(self, dt) 
    NextState()
end

function SetWaitTimer(seconds) 
    return function(self, dt) 
        waitTime = seconds
        NextState()
    end 
end 

function WaitState(self, dt)
    -- also wait for laser to complete
    if laserComplete == false then 
        if laserOpening == true then 
            miscAnim:SetPlayback(Playback.Reverse)
            miscAnim:OnComplete(function()
                    miscAnim:SetState("NO_SHOOT")
                    miscAnim:SetPlayback(Playback.Once)
                    miscAnim:Refresh(middle:Sprite())
                end)
                
            laserOpening = false
        end

        miscAnim:Update(dt, middle:Sprite(), 1.0) 
        return 
    end 

    waitTime = waitTime - dt 

    if waitTime <= 0 then
        NextState()
    end
end

function MoveState(self, dt) 
    if self:IsSliding() == false then
        -- we arrive, shake the ground
        
        if shake then
            self:ShakeCamera(3.0, 1.0)
            shake = false
            middle:Hide() -- reveal red center
        end
        
        local tile = nil
        
        waitTime = waitTime - dt
        -- pause before moving again
        if waitTime <= 0 then
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

            shake = true -- can shake again
            middle:Show() -- coverup red center
            self:SlideToTile(true)
            self:Move(dir)
            NextState()
        end
    end 
end

function battle_init(self) 
    aiStateIndex = 1
    aiState = { 
        SetWaitTimer(IDLE_TIME),
        MoveState,
        ShootMissileState,
        SetWaitTimer(IDLE_TIME),
        MoveState,
        ShootMineState,
        SetWaitTimer(IDLE_TIME),
        MoveState,
        ShootMissileState,
        SetWaitTimer(IDLE_TIME),
        MoveState,
        SetWaitTimer(IDLE_TIME),
        WaitState,
        MoveHandAway,
        SetWaitTimer(IDLE_TIME*0.5),
        WaitState,
        UpwardFistState,
        SetWaitTimer(IDLE_TIME),
        WaitState,
        DownwardFistState,
        SetWaitTimer(IDLE_TIME),
        WaitState,
        RestoreHand,
        SetWaitTimer(IDLE_TIME*0.5),
        WaitState,
        ShootLaserState,
        SetWaitTimer(IDLE_TIME),
        WaitState,
    }

    texture = LoadTexture(_modpath.."duo_compressed.png")

    print("modpath: ".._modpath)
    self:SetName("Duo")
    self:SetHealth(3000)
    self:SetTexture(texture, true)
    self:SetHeight(60)
    self:SetSlideFrames(80)
    self:ShareTile(true)
    self:SetPosition(idleDuoPos.x, idleDuoPos.y)

    anim = self:GetAnimation()
    anim:SetPath(_modpath.."duo_compressed.animation")
    anim:Load()
    anim:SetState("BODY", Playback.Once, nonce)

    hand = Engine.SpriteNode.new()
    hand:SetTexture(texture, true)
    hand:SetPosition(idleHandPos.x, idleHandPos.y)
    hand:SetLayer(-2) -- put it in front at all times
    hand:EnableParentShader(true)
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
    --handTimer = handTimer + dt
    --local x = math.sin(handTimer*60)*5
    --hand:setPosition(x, x)

    aiState[aiStateIndex](self, dt)
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