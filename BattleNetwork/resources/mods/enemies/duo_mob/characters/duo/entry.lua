nonce = function() end 

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

function create_upward_fist(duo) 
    local fist = Battle.Spell.new(Team.Blue)
    fist:never_flip(true)
    fist:set_texture(texture, true)
    fist:highlight_tile(Highlight.Flash)

    fist:set_hit_props(
        make_hit_props(
            50, 
            Hit.Impact | Hit.Flash | Hit.Flinch, 
            Element.None, 
            duo:get_id(), 
            no_drag()
        )
    )

    local fistAnim = fist:get_animation()
    fist.startSwinging = false

    fistAnim:copy_from(anim)
    fistAnim:set_state("HAND_SWIPE_BOTTOM_UP")
    fistAnim:set_playback(Playback.Once)

    local closure = function() 
        local fistRef = fist
        return function()
            fistRef:highlight_tile(Highlight.Solid)
            fistRef.startSwinging = true
        end 
    end 

    fistAnim:on_frame(2, closure(), true)

    fist.update_func = function(self, dt) 
        self:get_current_tile():attack_entities(self)

        if self:is_sliding() == false then 
            if self:get_current_tile():is_edge() then 
                self:delete()
            end 

            if self.startSwinging == true then
                local dest = self:get_tile(Direction.Up, 1)
                self:slide(dest, frames(4), frames(0), ActionOrder.Voluntary, nonce)
            end
        end
    end

    fist.attack_func = function(self, other) 
        -- nothing
    end

    fist.delete_func = function(self) 
    end

    fist.can_move_to_func = function(tile)
        return true
    end

    return fist
end 

function create_downward_fist(duo) 
    local fist = Battle.Spell.new(Team.Blue)
    fist:set_texture(texture, true)
    fist:never_flip(true)

    fist:set_hit_props(
        make_hit_props(
            50, 
            Hit.Impact | Hit.Flash | Hit.Flinch, 
            Element.None, 
            duo:get_id(), 
            no_drag()
        )
    )

    local fistAnim = fist:get_animation()
    fist.startSwinging = false

    fistAnim:copy_from(anim)
    fistAnim:set_state("HAND_SWIPE_TOP_DOWN")
    fistAnim:set_playback(Playback.Once)

    local closure = function() 
        local fistRef = fist
        return function()
            fistRef.startSwinging = true
            fistRef:highlight_tile(Highlight.Solid)
        end 
    end 

    fistAnim:on_frame(2, closure(), true)

    fist.update_func = function(self, dt) 
        self:get_current_tile():attack_entities(self)

        if self:is_sliding() == false then 
            if self:get_current_tile():y() == 4 then 
                self:delete()
            end 

            if self.startSwinging == true then
                local dest = self:get_tile(Direction.Down, 1)
                self:slide(dest, frames(4), frames(0), ActionOrder.Voluntary, nonce)
            end
        end
    end

    fist.attack_func = function(self, other) 
        -- nothing
    end

    fist.delete_func = function(self) 
    end

    fist.can_move_to_func = function(tile)
        return true
    end

    return fist
end 

function create_mine(duo) 
    local mine = Battle.Obstacle.new(Team.Blue)
    mine:set_health(20)
    mine:share_tile(true)
    mine:never_flip(true)

    mine:set_hit_props(
        make_hit_props(
            20, 
            Hit.Impact | Hit.Flash | Hit.Flinch, 
            Element.None, 
            duo:get_id(), 
            no_drag()
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
    mine.node:set_texture(texture, true)
    mine.node:set_position(0,0)
    mine:add_node(mine.node)

    miscAnim:set_state("MINE")
    miscAnim:set_playback(Playback.Once)
    miscAnim:refresh(mine.node:sprite())

    mine.update_func = function(self, dt) 
        --print("updating mine")
        local tile = self:get_current_tile()

        -- every frame try to attack shared tile
        tile:attack_entities(self)
        self:highlight_tile(Highlight.Solid)

        local tileW = tile:width()/2.0 -- downscale 2x
        local tileH = tile:height()/2.0 
        local offset = -self.timer*(1.0/(mine.slideFrames/60.0))*0.5
        -- local x = (-offset*tileW)+(tileW/2.0)
        -- local x = math.cos(offset*2.0*math.pi)*tileW/2.0
        local y = math.sin(offset*2.0*math.pi)*tileH/2.0
        self.node:set_position(0, y)

        if self.moveStart then
            self.waitTimer = self.waitTimer - dt
            
            if(self.waitTimer > 0) then 
                return 
            end
        
            self.timer = self.timer + dt
        end 

        if self:is_sliding() == false then

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
            local dest = self:get_tile(self.dir, 1)
            local slide = self:slide(dest, frames(MINE_SLIDE_FRAMES), frames(0), ActionOrder.Voluntary, 
                function() 
                    ref.moveStart = true

                    if ref.moveOneColumn == false then 
                        ref.verticalDir = ref.dir
                    end

                    ref.moveOneColumn = not ref.moveOneColumn

                end)

            if not slide then 
                self:remove()
            end
        end 
    end

    mine.attack_func = function(self, other) 
        self:delete()
    end

    mine.delete_func = function(self) 
        local explosion = Battle.Explosion.new(1, 1)
        local x = self:get_current_tile():x()
        local y = self:get_current_tile():y()
        self:get_field():spawn(explosion, x, y)
        self:remove()
    end

    mine.can_move_to_func = function(tile)
        -- mines fly over any tiles
        return true
    end

    return mine
end

function create_laser_beam(duo)
    laserComplete = false
    local laser = Battle.Spell.new(Team.Blue) 

    laser:never_flip(true)
    laser:set_texture(texture, true)
    laser:set_layer(-2)

    laser:set_hit_props(
        make_hit_props(
            100, 
            Hit.Impact | Hit.Flash | Hit.Flinch, 
            Element.None, 
            duo:get_id(), 
            no_drag()
        )
    )

    local origin = duo:get_animation():point("origin")
    local point  = duo:get_animation():point("NO_SHOOT")
    laser:set_position(point.x - origin.x + 20, point.y - origin.y)

    local laserAnim = laser:get_animation()
    laserAnim:copy_from(anim)

    local closure = function() 
        local laserRef = laser
        local animRef = laserAnim
        return function()
            laserRef.laserCount = 0
            animRef:set_state("LASER_BEAM")
            animRef:set_playback(Playback.Loop)
            animRef:on_complete(function() 
                laserRef.laserCount = laserRef.laserCount + 1

                if laserRef.laserCount > 40 then
                    animRef:set_state("LASER_BEAM_OPEN")
                    animRef:set_playback(Playback.Reverse)
                    animRef:on_complete(function()
                        laserComplete = true
                        laserRef:delete()
                    end)
                end
            end)
        end 
    end 

    laserAnim:set_state("LASER_BEAM_OPEN")
    laserAnim:set_playback(Playback.Once)
    laserAnim:on_complete(closure())

    laser.update_func = function(self, dt) 
        local count = 1
        local dest = self:get_tile(Direction.Left, 1)
 
        while dest ~= nil do
             count = count + 1
             local hitbox = Battle.Hitbox.new(self:get_team())
             hitbox:set_hit_props(self:copy_hit_props())
             self:get_field():spawn(hitbox, dest:x(), dest:y())
             dest = self:get_tile(Direction.Left, count)
        end
    end

    laser.attack_func = function(self, other) 
        -- nothing
    end

    laser.delete_func = function(self) 
    end

    laser.can_move_to_func = function(tile)
        return false
    end

    laser.on_spawn_func = function(self, tile) 
        -- nothing special on spawn
    end

    return laser
end

function create_missile(duo)
    local missile = Battle.Obstacle.new(Team.Blue)
    missile:set_health(20)
    missile:set_texture(texture, true)
    missile:set_height(20.0)
    missile:set_layer(-2)
    missile:show_shadow(false)
    missile:share_tile(true)
    missile:never_flip(true)
    missile.waitTimer = 1 -- second

    missile:set_hit_props(
        make_hit_props(
            30, 
            Hit.Impact | Hit.Flash | Hit.Flinch, 
            Element.None, 
            duo:get_id(), 
            no_drag()
        )
    )

    local missileAnim = missile:get_animation()
    missileAnim:copy_from(anim)
    missileAnim:set_state("MISSILE")
    missileAnim:set_playback(Playback.Loop)

    missile.update_func = function(self, dt) 
        --print("updating missile")

        self.waitTimer = self.waitTimer - dt
        if(self.waitTimer > 0) then 
            return 
        end

        if self:is_sliding() == false then
            if self:get_current_tile():is_edge() then 
                self:remove()
            end

            -- otherwise keep sliding
            local dest = self:get_tile(Direction.Left, 1)
            self:slide(dest, frames(20), frames(0), ActionOrder.Voluntary, nonce)
        end 

        if self:get_current_tile():is_hole() == false then 
            missile:show_shadow(true)
        end

        -- every frame try to attack shared tile
        self:get_current_tile():attack_entities(self)
    end

    missile.attack_func = function(self, other) 
        local explosion = Battle.Explosion.new(1, 1)
        local dest = self:get_current_tile()
        self:get_field():spawn(explosion, dest:x(), dest:y())
        self:delete()
    end

    missile.delete_func = function(self) 
        local explosion = Battle.Explosion.new(1, 1)
        local dest = self:get_current_tile()
        self:get_field():spawn(explosion, dest:x(), dest:y())
    end

    missile.can_move_to_func = function(tile)
        -- missiles fly over any tile
        return true
    end

    return missile
end

function next_state()
    -- increment our AI state counter
    aiStateIndex = aiStateIndex + 1
    if aiStateIndex > #aiState then 
        aiStateIndex = 1
    end
end

function restore_hand(self, dt) 
    hand:show()

    pos = hand:get_position()

    -- slide in
    hand:set_position(pos.x - (dt*5*60), pos.y + (dt*2*60))

    pos = self:get_position()
    self:set_position(pos.x - (dt*40), pos.y)

    if hand:get_position().x <= 0 then
        self:set_position(idleDuoPos.x, idleDuoPos.y)
        hand:set_position(idleHandPos.x, idleHandPos.y)
        next_state()
    end
end

function move_hand_away(self, dt) 
    local pos = hand:get_position()

    -- slide out and up 5px a frame
    hand:set_position(pos.x + (dt*5*60), pos.y - (dt*2*60))

    pos = self:get_position()

    self:set_position(pos.x + (dt*40), pos.y)

    if hand:get_position().x > 92 then
        hand:hide()
        next_state()
    end
end

function shoot_laser_state(self, dt)
    if miscAnim:get_state() ~= "CHEST_GUN" then
        laserOpening = true
        laserComplete = false
        function closure() 
            local duoRef = self 
            return function() 
                local laser = create_laser_beam(duoRef)
                local x = duoRef:get_current_tile():x()-1
                local y = duoRef:get_current_tile():y()
                duoRef:get_field():spawn(laser, x, y)
                next_state()
            end 
        end

        miscAnim:set_state("CHEST_GUN")
        miscAnim:set_playback(Playback.Once)
        miscAnim:on_complete(closure())
    end

    miscAnim:update(dt, middle:sprite(), 1.0)
end

function shoot_missile_state(self, dt)
    middle:hide() -- reveal red center
    local tile = self:get_current_tile()
    self:get_field():spawn(create_missile(self), tile:x()-1, tile:y())
    next_state()
end 

function shoot_mine_state(self, dt) 
    middle:hide() -- reveal red center
    local tile = self:get_current_tile()
    self:get_field():spawn(create_mine(self), tile:x()-1, tile:y())
    next_state()
end 

function downward_fist_state(self, dt)
    self:get_field():spawn(create_downward_fist(self), 2, 0)
    next_state()
end

function upward_fist_state(self, dt)
    -- back-left corner
    self:get_field():spawn(create_upward_fist(self), 1, 3)
    next_state()
end

function laser_state(self, dt)
    next_state()
end

function face_attack_state(self, dt) 
    next_state()
end

function set_wait_timer(seconds) 
    return function(self, dt) 
        waitTime = seconds
        next_state()
    end 
end 

function wait_laser_state(self, dt)
    miscAnim:update(dt, middle:sprite(), 1.0) 

    -- also wait for laser to complete
    if laserComplete == true then 
        middle:show()

        miscAnim:set_state("CHEST_GUN")
        miscAnim:set_playback(Playback.Reverse)
        miscAnim:on_complete(function()
                miscAnim:set_state("NO_SHOOT")
                miscAnim:set_playback(Playback.Once)
                miscAnim:refresh(middle:sprite())
                miscAnim:on_complete(next_state)
            end)
            
        laserOpening = false
        laserComplete = false
        return 
    end 
end

function wait_state(self, dt)
    if not self:is_jumping() then
        waitTime = waitTime - dt 
    end

    if waitTime <= 0 then
        next_state()
    end
end

function move_state(self, dt) 
    if self:is_jumping() == false then      
        -- we arrive, shake the ground
        if shake then
            self:shake_camera(3.0, 1.0)
            shake = false
            next_state()
            return
        end

        local tile = nil
        
        waitTime = waitTime - dt

        -- pause before moving again
        if waitTime <= 0 then
            if dir == Direction.Up then 
                local tile = self:get_current_tile()
                tile = self:get_field():tile_at(tile:x(), tile:y()-1)

                if can_move_to(tile) == false then 
                    dir = Direction.Down
                end
            elseif dir == Direction.Down then
                local tile = self:get_current_tile()
                tile = self:get_field():tile_at(tile:x(), tile:y()+1)

                if can_move_to(tile) == false then
                    dir = Direction.Up
                end
            end

            middle:show() -- coverup red center
            local dest = self:get_tile(dir, 1)
            self:jump(dest, 40.0, frames(60), frames(60), ActionOrder.Voluntary, 
                function() 
                    print("jumping started")
                    shake = true 
                end)
        end
    end 
end

function package_init(self)
    aiStateIndex = 1
    aiState = { 
        set_wait_timer(IDLE_TIME),
        move_state,
        shoot_missile_state,
        set_wait_timer(IDLE_TIME),
        move_state,
        shoot_mine_state,
        set_wait_timer(IDLE_TIME),
        move_state,
        shoot_missile_state,
        set_wait_timer(IDLE_TIME),
        move_state,
        set_wait_timer(IDLE_TIME),
        wait_state,
        move_hand_away,
        set_wait_timer(IDLE_TIME*0.5),
        wait_state,
        upward_fist_state,
        set_wait_timer(IDLE_TIME),
        wait_state,
        downward_fist_state,
        set_wait_timer(IDLE_TIME),
        wait_state,
        restore_hand,
        set_wait_timer(IDLE_TIME*0.5),
        wait_state,
        shoot_laser_state,
        wait_laser_state,
        set_wait_timer(IDLE_TIME),
        wait_state,
    }

    texture = Engine.load_texture(_modpath.."duo_compressed.png")

    self:set_name("Duo")
    self:set_rank(Rank.EX)
    self:set_health(3000)
    self:set_texture(texture, true)
    self:set_height(60)
    self:share_tile(true)
    self:set_explosion_behavior(12, 1.0, true)

    self:set_position(idleDuoPos.x, idleDuoPos.y)

    anim = self:get_animation()
    anim:load(_modpath.."duo_compressed.animation")
    anim:set_state("BODY")
    anim:set_playback(Playback.Once)

    hand = Engine.SpriteNode.new()
    hand:set_texture(texture, true)
    hand:set_position(idleHandPos.x, idleHandPos.y)
    hand:set_layer(-2) -- put it in front at all times
    hand:enable_parent_shader(true)
    self:add_node(hand)

    miscAnim = Engine.Animation.new(anim)
    miscAnim:set_state("HAND_IDLE")
    miscAnim:refresh(hand:sprite())

    middle = Engine.SpriteNode.new()
    middle:set_texture(texture, true)
    middle:set_layer(-1)

    miscAnim:set_state("NO_SHOOT")
    miscAnim:refresh(middle:sprite())

    local origin = anim:point("origin")
    local point  = anim:point("NO_SHOOT")
    middle:set_position(point.x - origin.x, point.y - origin.y)
    self:add_node(middle)

    self.defense = Battle.DefenseRule.new(0, DefenseOrder.Always)

    print("defense was created")

    self.defense.filter_statuses_func = function(props) 
        print("inside filter_statuses_func")
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

    self:add_defense_rule(self.defense)

    self.update_func = on_update
    self.battle_start_func = on_battle_start
    self.battle_end_func = on_battle_end
    self.on_spawn_func = on_spawn
    self.can_move_to_func = can_move_to
    self.delete_func = on_delete
    
    print("done")
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
    if tile:is_edge() then
        return false
    end

    return true
end

function on_delete(self)
    print("on_delete called")
end