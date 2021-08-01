nonce = function() end 

function LoadTexture(path)
    return Engine.ResourceHandle.new().Textures:LoadFile(path)
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
local laserComplete = false
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

    fist:SetHitProps(MakeHitProps(
        50, 
        Hit.Impact | Hit.Flash | Hit.Flinch, 
        Element.None, 
        duo:GetID(), 
        Drag(Direction.None, 0)
        )
    )

    local fistAnim = fist:GetAnimation()
    fist.startSwinging = false

    fistAnim:CopyFrom(anim)
    fistAnim:SetState("HAND_SWIPE_BOTTOM_UP")
    fistAnim:SetPlayback(Playback.Once)

    local closure = function() 
        local fistRef = fist
        return function()
            fistRef:HighlightTile(Highlight.Solid)
            fistRef.startSwinging = true
        end 
    end 

    fistAnim:OnFrame(2, closure(), true)

    fist.updateFunc = function(self, dt) 
        self:CurrentTile():AttackEntities(self)

        if self:IsSliding() == false then 
            if self:CurrentTile():IsEdge() then 
                self:Delete()
            end 

            if self.startSwinging == true then
                local dest = self:GetTile(Direction.Up, 1)
                self:Slide(dest, frames(4), frames(0), ActionOrder.Voluntary, nonce)
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

    fist:SetHitProps(MakeHitProps(
        50, 
        Hit.Impact | Hit.Flash | Hit.Flinch, 
        Element.None, 
        duo:GetID(), 
        Drag(Direction.None, 0)
        )
    )

    local fistAnim = fist:GetAnimation()
    fist.startSwinging = false

    fistAnim:CopyFrom(anim)
    fistAnim:SetState("HAND_SWIPE_TOP_DOWN")
    fistAnim:SetPlayback(Playback.Once)

    local closure = function() 
        local fistRef = fist
        return function()
            fistRef.startSwinging = true
            fistRef:HighlightTile(Highlight.Solid)
        end 
    end 

    fistAnim:OnFrame(2, closure(), true)

    fist.updateFunc = function(self, dt) 
        self:CurrentTile():AttackEntities(self)

        if self:IsSliding() == false then 
            if self:CurrentTile():Y() == 4 then 
                self:Delete()
            end 

            if self.startSwinging == true then
                local dest = self:GetTile(Direction.Down, 1)
                self:Slide(dest, frames(4), frames(0), ActionOrder.Voluntary, nonce)
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
    mine:ShareTile(true)
    mine:SetHitProps(MakeHitProps(
        20, 
        Hit.Impact | Hit.Flash | Hit.Flinch, 
        Element.None, 
        duo:GetID(), 
        Drag(Direction.None, 0)
        )
    )

    mine.dir = Direction.Up
    mine.verticalDir = Direction.Down
    mine.moveOneColumn = false
    mine.timer = 0
    mine.waitTimer = 1 -- second
    mine.slideFrames = MINE_SLIDE_FRAMES
    mine.moveStart = true -- first time starts
    mine.node = Engine.SpriteNode.new()
    mine.node:SetTexture(texture, true)
    mine.node:SetPosition(0,0)
    mine:AddNode(mine.node)

    miscAnim:SetState("MINE")
    miscAnim:SetPlayback(Playback.Once)
    miscAnim:Refresh(mine.node:Sprite())

    mine.updateFunc = function(self, dt) 
        --print("updating mine")
        local tile = self:CurrentTile()

        -- every frame try to attack shared tile
        tile:AttackEntities(self)
        self:HighlightTile(Highlight.Solid)

        local tileW = tile:Width()/2.0 -- downscale 2x
        local tileH = tile:Height()/2.0 
        local offset = -self.timer*(1.0/(mine.slideFrames/60.0))*0.5
        -- local x = (-offset*tileW)+(tileW/2.0)
        -- local x = math.cos(offset*2.0*math.pi)*tileW/2.0
        local y = math.sin(offset*2.0*math.pi)*tileH/2.0
        self.node:SetPosition(0, y)

        if self.moveStart then
            self.waitTimer = self.waitTimer - dt
            
            if(self.waitTimer > 0) then 
                return 
            end
        
            self.timer = self.timer + dt
        end 

        if self:IsSliding() == false then

            if self.moveOneColumn == false then
                if self.verticalDir == Direction.Up then
                    self.dir = Direction.Down
                elseif self.verticalDir == Direction.Down then 
                    self.dir = Direction.Up
                end
            else 
                self.dir = Direction.Left
            end

            -- otherwise keep sliding
            local ref = self
            local dest = self:GetTile(self.dir, 1)
            local slide = self:Slide(dest, frames(MINE_SLIDE_FRAMES), frames(0), ActionOrder.Voluntary, 
                function() 
                    ref.moveStart = true

                    if ref.moveOneColumn == false then 
                        ref.verticalDir = ref.dir
                    end

                    ref.moveOneColumn = not ref.moveOneColumn

                end)

            if not slide then 
                self:Remove()
            end
        end 
    end

    mine.attackFunc = function(self, other) 
        self:Delete()
    end

    mine.deleteFunc = function(self) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():Spawn(explosion, self:CurrentTile():X(), self:CurrentTile():Y())
        self:Remove()
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

    laser:SetHitProps(MakeHitProps(
        100, 
        Hit.Impact | Hit.Flash | Hit.Flinch, 
        Element.None, 
        duo:GetID(), 
        Drag(Direction.None, 0 )
        )
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
            animRef:SetState("LASER_BEAM")
            animRef:SetPlayback(Playback.Loop)
            animRef:OnComplete(function() 
                laserRef.laserCount = laserRef.laserCount + 1

                if laserRef.laserCount > 40 then
                    animRef:SetState("LASER_BEAM_OPEN")
                    animRef:SetPlayback(Playback.Reverse)
                    animRef:OnComplete(function()
                        laserComplete = true
                        laserRef:Delete()
                    end)
                end
            end)
        end 
    end 

    laserAnim:SetState("LASER_BEAM_OPEN")
    laserAnim:SetPlayback(Playback.Once)
    laserAnim:OnComplete(onComplete())

    laser.updateFunc = function(self, dt) 
        self:CurrentTile():AttackEntities(self)
    end

    laser.attackFunc = function(self, other) 
        -- nothing
    end

    laser.deleteFunc = function(self) 
    end

    laser.canMoveToFunc = function(tile)
        return false
    end

    laser.onSpawnFunc = function(self, tile) 
       local count = 1
       local dest = self:GetTile(Direction.Left, 1)

       while dest ~= nil do
            count = count + 1
            local hitbox = Battle.Hitbox.new(self:Team())
            hitbox:SetHitProps(self:GetHitProps())
            self:Field():Spawn(hitbox, dest:X(), dest:Y())
            dest = self:GetTile(Direction.Left, count)
       end
    end

    return laser
end

function Missile(duo)
    local missile = Battle.Obstacle.new(Team.Blue)
    missile:SetHealth(20)
    missile:SetTexture(texture, true)
    missile:SetHeight(20.0)
    missile:SetLayer(-2)
    missile:ShowShadow(false)
    missile:ShareTile(true)
    missile.waitTimer = 1 -- second

    missile:SetHitProps(MakeHitProps(
        30, 
        Hit.Impact | Hit.Flash | Hit.Flinch, 
        Element.None, 
        duo:GetID(), 
        Drag(Direction.None, 0)
        )
    )

    local missileAnim = missile:GetAnimation()
    missileAnim:CopyFrom(anim)
    missileAnim:SetState("MISSILE")
    missileAnim:SetPlayback(Playback.Loop)

    missile.updateFunc = function(self, dt) 
        --print("updating missile")

        self.waitTimer = self.waitTimer - dt
        if(self.waitTimer > 0) then 
            return 
        end

        if self:IsSliding() == false then
            if self:CurrentTile():IsEdge() then 
                self:Remove()
            end

            -- otherwise keep sliding
            local dest = self:GetTile(Direction.Left, 1)
            self:Slide(dest, frames(20), frames(0), ActionOrder.Voluntary, nonce)
        end 

        if self:CurrentTile():IsHole() == false then 
            missile:ShowShadow(true)
        end

        -- every frame try to attack shared tile
        self:CurrentTile():AttackEntities(self)
    end

    missile.attackFunc = function(self, other) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():Spawn(explosion, self:CurrentTile():X(), self:CurrentTile():Y())
        self:Delete()
    end

    missile.deleteFunc = function(self) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():Spawn(explosion, self:CurrentTile():X(), self:CurrentTile():Y())
    end

    missile.canMoveToFunc = function(tile)
        -- missiles fly over any tile
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
        laserComplete = false
        function onComplete() 
            local duoRef = self 
            return function() 
                -- laserComplete gets toggled to false when a laser beam is created
                duoRef:Field():Spawn(LaserBeam(duoRef), duoRef:CurrentTile():X()-1, duoRef:CurrentTile():Y())
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
    middle:Hide() -- reveal red center
    self:Field():Spawn(Missile(self), self:CurrentTile():X()-1, self:CurrentTile():Y())
    NextState()
end 

function ShootMineState(self, dt) 
    middle:Hide() -- reveal red center
    self:Field():Spawn(Mine(self), self:CurrentTile():X()-1, self:CurrentTile():Y())
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

function WaitLaserState(self, dt)
    miscAnim:Update(dt, middle:Sprite(), 1.0) 

    -- also wait for laser to complete
    if laserComplete == true then 
        middle:Show()

        miscAnim:SetState("CHEST_GUN")
        miscAnim:SetPlayback(Playback.Reverse)
        miscAnim:OnComplete(function()
                miscAnim:SetState("NO_SHOOT")
                miscAnim:SetPlayback(Playback.Once)
                miscAnim:Refresh(middle:Sprite())
                miscAnim:OnComplete(function() 
                    NextState()
                end)
            end)
            
        laserOpening = false
        laserComplete = false
        return 
    end 
end

function WaitState(self, dt)
    if not self:IsJumping() then
        waitTime = waitTime - dt 
    end

    if waitTime <= 0 then
        NextState()
    end
end

function MoveState(self, dt) 
    if self:IsJumping() == false then      
        -- we arrive, shake the ground
        if shake then
            self:ShakeCamera(3.0, 1.0)
            shake = false
            NextState()
            return
        end

        local tile = nil
        
        waitTime = waitTime - dt

        -- pause before moving again
        if waitTime <= 0 then
            if dir == Direction.Up then 
                tile = self:Field():TileAt(self:CurrentTile():X(), self:CurrentTile():Y()-1)

                if can_move_to(tile) == false then 
                    dir = Direction.Down
                end
            elseif dir == Direction.Down then
                tile = self:Field():TileAt(self:CurrentTile():X(), self:CurrentTile():Y()+1)

                if can_move_to(tile) == false then
                    dir = Direction.Up
                end
            end

            middle:Show() -- coverup red center
            local dest = self:GetTile(dir, 1)
            self:Jump(dest, 40.0, frames(60), frames(60), ActionOrder.Voluntary, 
                function() 
                    print("jumping started")
                    shake = true 
                end)
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
        WaitLaserState,
        SetWaitTimer(IDLE_TIME),
        WaitState,
    }

    texture = LoadTexture(_modpath.."duo_compressed.png")

    print("modpath: ".._modpath)
    self:SetName("Duo")
    self:SetRank(Rank.EX)
    self:SetHealth(3000)
    self:SetTexture(texture, true)
    self:SetHeight(60)
    self:ShareTile(true)

    print("before set position")
    self:SetPosition(idleDuoPos.x, idleDuoPos.y)
    print("after set position")

    anim = self:GetAnimation()
    anim:Load(_modpath.."duo_compressed.animation")
    anim:SetState("BODY")
    anim:SetPlayback(Playback.Once)

    hand = Engine.SpriteNode.new()
    hand:SetTexture(texture, true)
    hand:SetPosition(idleHandPos.x, idleHandPos.y)
    hand:SetLayer(-2) -- put it in front at all times
    hand:EnableParentShader(true)
    self:AddNode(hand)

    miscAnim = Engine.Animation.new(anim)
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

    print("right before self.defense")

    self.defense = Battle.DefenseRule.new(0, DefenseOrder.Always)

    print("defense was created")

    self.defense.filterStatusesFunc = function(props) 
        print("inside filterStatusesFunc")
        if props.flags & Hit.Flinch == Hit.Flinch then
            props.flags = props.flags~Hit.Flinch
        end

        if props.flags & Hit.Flash == Hit.Flash then
            props.flags = props.flags~Hit.Flash
        end

        if props.flags & Hit.Drag == Hit.Drag then
            props.flags = props.flags~Hit.Drag
        end

        return props
    end

    self:AddDefenseRule(self.defense)

    print("done")
end

function num_of_explosions()
    return 12
end

function on_update(self, dt)
    aiState[aiStateIndex](self, dt)
end

function on_battle_start(self)
   print("on_battle_start called")
end

function on_battle_end(self)
    print("on_battle_end called")
end

function on_spawn(self, tile) 
    print("on_spawn called")
end

function can_move_to(tile) 
    if tile:IsEdge() then
        return false
    end

    return true
end