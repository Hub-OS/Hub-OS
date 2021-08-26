nonce = function() end

DAMAGE = 40
TEXTURE = nil
AUDIO = nil

function package_init(package) 
    package:declare_package_id("com.example.card.Beastman")
    package:set_icon_texture(Engine.load_texture(_modpath.."icon.png"))
    package:set_preview_texture(Engine.load_texture(_modpath.."preview.png"))

    local props = package:get_card_props()
    props.shortname = "BeastMan"
    props.code = 'B'
    props.damage = DAMAGE
    props.time_freeze = true
    props.element = Element.None
    props.description = "Claw atk 3 squares ahead!"

    -- assign the global resources
    TEXTURE = Engine.load_texture(_modpath.."beastman.png")
	AUDIO = Engine.load_audio(_modpath.."swipe.ogg")
end

--[[
    1. megaman stunt double warps out
    2. purple in
    3. beast man idle one cycle
    4. beast man roar one cycle
    5. beast man claw one cycle
    6. purple out
    7. beast man is hidden
    8. down_right claw spawned in the same column as the user, row 0
    9. after down_right claw reaches 2nd tile, up_right claw is spawned
        (same col as down_right claw, but row 4)
    10. after up_right claw reaches 2nd tile, head is spawned on the last col, row as user
    11. after all elements are off-screen and deleted, megaman is returned and time freeze ends
--]]
function card_create_action(user, props)
    print("in create_card_action()!")
    local action = Battle.CardAction.new(user, "PLAYER_IDLE")

    action:set_lockout(make_sequence_lockout())

    action.execute_func = function(self, user)
        print("in custom card action execute_func()!")

        local step1 = Battle.Step.new()
        local step2 = Battle.Step.new()
        local step3 = Battle.Step.new()
        local step4 = Battle.Step.new()

        self.down_right_claw = nil
        self.up_right_claw   = nil
        self.head            = nil
        self.beastman        = nil
        self.tile            = user:get_current_tile()

        local ref = self
        local actor = self:get_actor()

        step1.update_func = function(self, dt) 
            if ref.beastman == nil then
                ref.beastman = create_beast_intro()
                actor:hide()
                actor:get_field():spawn(ref.beastman, ref.tile:x(), ref.tile:y())
            end

            if ref.beastman:will_remove_eof() then
                self:complete_step()
            end
        end

        step2.update_func = function(self, dt)
            if ref.down_right_claw == nil then
                ref.down_right_claw = create_spell("claw_down_right", Direction.DownRight, actor)
                actor:get_field():spawn(ref.down_right_claw, ref.tile:x()+1, ref.tile:y()-2)
            end

            if ref.down_right_claw:will_remove_eof() then
                self:complete_step()
            elseif Engine.input_has(Input.Pressed.Use) then 
                local props = ref.down_right_claw:copy_hit_props()
                local tile = ref.down_right_claw:get_current_tile()
                local hitbox = Battle.Hitbox.new(actor:get_team())
                hitbox:set_hit_props(props)
                actor:get_field():spawn(hitbox, tile:x(), tile:y())
            end
        end

        step3.update_func = function(self, dt) 
            if ref.up_right_claw == nil then
                ref.up_right_claw = create_spell("claw_up_right", Direction.UpRight, actor)
                actor:get_field():spawn(ref.up_right_claw, ref.tile:x()+1, 4)
            end

            if ref.up_right_claw:will_remove_eof() then
                self:complete_step()
            elseif Engine.input_has(Input.Pressed.Use) then 
                local props = ref.up_right_claw:copy_hit_props()
                local tile = ref.up_right_claw:get_current_tile()
                local hitbox = Battle.Hitbox.new(actor:get_team())
                hitbox:set_hit_props(props)
                actor:get_field():spawn(hitbox, tile:x(), tile:y())
            end
        end

        step4.update_func = function(self, dt) 
            if ref.head == nil then
                ref.head = create_spell("head", Direction.Right, actor)
                actor:get_field():spawn(ref.head, 0, ref.tile:y())
            end

            if ref.head:will_remove_eof() then
                self:complete_step()
            elseif Engine.input_has(Input.Pressed.Use) then 
                local props = ref.head:copy_hit_props()
                local tile = ref.head:get_current_tile()
                local hitbox = Battle.Hitbox.new(actor:get_team())
                hitbox:set_hit_props(props)
                actor:get_field():spawn(hitbox, tile:x(), tile:y())
            end
        end

        self:add_step(step1)
        self:add_step(step2)
        self:add_step(step3)
        self:add_step(step4)
    end

    return action
end

function create_beast_intro() 
    local fx = Battle.Artifact.new()
    fx:set_texture(TEXTURE, true)
    fx:get_animation():load(_modpath.."beastman.animation")
    fx:get_animation():set_state("INTRO")
    fx:get_animation():on_complete(function() 
        fx:remove() 
    end)

    return fx
end

function create_spell(animation_state, direction, user)
    local spell = Battle.Spell.new(user:get_team())
    spell:set_texture(TEXTURE, true)
    spell:highlight_tile(Highlight.Solid)

    spell.slide_started = false

    spell:set_hit_props(
        make_hit_props(
            DAMAGE, 
            Hit.Impact | Hit.Flash | Hit.Flinch, 
            Element.None, 
            user:get_id(), 
            no_drag()
        )
    )

    local anim = spell:get_animation()
    anim:load(_modpath.."beastman.animation")
    anim:set_state(animation_state)

    spell.update_func = function(self, dt) 
        self:get_current_tile():attack_entities(self)

        if self:is_sliding() == false then 
            if self:get_current_tile():is_edge() and self.slide_started then 
                self:delete()
            end 

            local dest = self:get_tile(direction, 1)
            local ref = self
            self:slide(dest, frames(4), frames(0), ActionOrder.Voluntary, 
                function()
                    drop_trace_fx(ref)
                    ref.slide_started = true 
                end
            )
        end
    end

    spell.attack_func = function(self, other) 
        -- nothing
    end

    spell.delete_func = function(self) 
    end

    spell.can_move_to_func = function(tile)
        return true
    end

	Engine.play_audio(AUDIO, AudioPriority.High)

    return spell
end

function drop_trace_fx(obj)
    local fx = Battle.Artifact.new()
    local anim = obj:get_animation()
    fx:set_texture(TEXTURE, true)
    fx:get_animation():copy_from(anim)
    fx:get_animation():set_state(anim:get_state().."_dark")

    fx.update_func = function(self, dt)
        local a = math.max(0, self:get_alpha()-math.floor(dt*2000))
        self:set_alpha(a)

        if a == 0 then 
            self:remove()
        end
    end

	local tile = obj:get_current_tile()
    obj:get_field():spawn(fx, tile:x(), tile:y())
end