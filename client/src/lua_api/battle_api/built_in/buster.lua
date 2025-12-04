local Buster = {}

---@param user Entity
---@param charged boolean
---@param damage number
function Buster.new(user, charged, damage)
    local action = Action.new(user, "CHARACTER_SHOOT")
    action:set_lockout(ActionLockout.new_sequence())
    action:create_step()

    local context = user:context()
    local rapid_level = user:rapid_level()

    -- override animation

    local frame_data = { { 1, 1 }, { 2, 2 }, { 3, 2 }, { 1, 1 } }

    action:override_animation_frames(frame_data)

    -- setup buster attachment
    local buster_attachment = action:create_attachment("BUSTER")

    local buster_sprite = buster_attachment:sprite()
    buster_sprite:set_texture(user:texture())
    buster_sprite:set_palette(user:palette())
    buster_sprite:set_layer(-2)
    buster_sprite:use_root_shader()

    local buster_animation = buster_attachment:animation()
    buster_animation:copy_from(user:animation())
    buster_animation:set_state("BUSTER", frame_data)

    -- spell
    local cooldown_table = {
        { 5, 9, 13, 17, 21, 25 },
        { 4, 8, 11, 15, 18, 21 },
        { 4, 7, 10, 13, 16, 18 },
        { 3, 5, 7,  9,  11, 13 },
        { 3, 4, 5,  6,  7,  8 }
    }

    rapid_level = math.max(math.min(rapid_level, #cooldown_table), 1)

    local cooldown = cooldown_table[rapid_level][6]
    local elapsed_frames = 0
    local spell_erased_frame = 0

    local spell = Spell.new(user:team())
    local can_move = false

    spell:set_facing(user:facing())

    action.on_update_func = function()
        if spell_erased_frame == 0 and spell:will_erase_eof() then
            spell_erased_frame = elapsed_frames
        end

        elapsed_frames = elapsed_frames + 1

        if spell_erased_frame > 0 and elapsed_frames - spell_erased_frame >= cooldown then
            action:end_action()
            return
        end

        if can_move then
            local motion_x = 0
            local motion_y = 0

            if user:input_has(Input.Held.Left) then
                motion_x = motion_x - 1
            end

            if user:input_has(Input.Held.Right) then
                motion_x = motion_x + 1
            end

            if user:input_has(Input.Held.Up) then
                motion_y = motion_y - 1
            end

            if user:input_has(Input.Held.Down) then
                motion_y = motion_y + 1
            end

            if user:team() == Team.Blue then
                motion_x = -motion_x
            end

            if (motion_x ~= 0 and user:can_move_to(user:get_tile(Direction.Right, motion_x))) or
                (motion_y ~= 0 and user:can_move_to(user:get_tile(Direction.Down, motion_y))) then
                action:end_action()
            end
        end
    end

    action:add_anim_action(2, function()
        Resources.play_audio(Resources.game_folder() .. "resources/sfx/pew.ogg")

        spell:set_hit_props(HitProps.new(
            damage,
            Hit.None,
            Element.None,
            context,
            Drag.None
        ))

        local tiles_travelled = 1
        local move_timer = 0

        spell.on_update_func = function()
            spell:attack_tile()

            move_timer = move_timer + 1

            if move_timer < 2 then
                return
            end

            local tile = spell:get_tile(spell:facing(), 1)

            if tile then
                tiles_travelled = tiles_travelled + 1
                spell:teleport(tile)
            else
                spell:delete()
            end

            move_timer = 0
        end

        spell.on_collision_func = function()
            spell:delete()
        end

        spell.on_attack_func = function(self, entity)
            local hit_x = 0
            local hit_y = entity:height()
            local state = "HIT"

            if charged then
                hit_y = hit_y / 2
                state = "CHARGED_HIT"
            else
                hit_x = entity:sprite():width() * (math.random(0, 1) - 0.5)
                hit_y = math.random() * hit_y
            end

            local movement_offset = entity:movement_offset()
            hit_x = hit_x + movement_offset.x
            hit_y = hit_y - movement_offset.y

            local hit_artifact = Artifact.new()
            hit_artifact:load_animation(Resources.game_folder() .. "resources/scenes/battle/spell_bullet_hit.animation")
            hit_artifact:set_texture(Resources.game_folder() .. "resources/scenes/battle/spell_bullet_hit.png")
            hit_artifact:set_offset(hit_x, -hit_y)
            hit_artifact:set_layer(-1)

            local hit_animation = hit_artifact:animation()
            hit_animation:set_state(state)
            hit_animation:on_complete(function()
                hit_artifact:erase()
            end)

            Field.spawn(hit_artifact, entity:current_tile())
        end

        spell.on_delete_func = function()
            local calculated_cooldown = cooldown_table[rapid_level][tiles_travelled]

            if calculated_cooldown ~= nil then
                cooldown = calculated_cooldown
            end

            spell:erase()
        end

        Field.spawn(spell, user:current_tile())
    end)

    -- flare attachment
    action:add_anim_action(3, function()
        local flare_attachment = buster_attachment:create_attachment("ENDPOINT")
        local flare_sprite = flare_attachment:sprite()
        flare_sprite:set_texture(Resources.game_folder() .. "resources/scenes/battle/buster_flare.png")
        flare_sprite:set_layer(-3)

        local animation = flare_attachment:animation()
        animation:load(Resources.game_folder() .. "resources/scenes/battle/buster_flare.animation")
        animation:set_state("DEFAULT")
        animation:on_frame(3, function()
            can_move = true
        end)

        animation:apply(flare_sprite)
    end)

    action.on_action_end_func = function()
        if not spell:deleted() and not spell:spawned() then
            spell:delete()
        end
    end

    return action
end

return Buster
