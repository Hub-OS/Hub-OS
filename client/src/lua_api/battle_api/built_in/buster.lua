local user, charged, damage = ...
local card_action = Battle.CardAction.new(user, "PLAYER_SHOOTING")
local context = user:get_context()
local speed_level = user:get_speed_level()

-- override animation

local frame_data = { { 1, 1 }, { 2, 2 }, { 3, 2 }, { 1, 1 } }

card_action:override_animation_frames(frame_data)

-- setup buster attachment
local buster_attachment = card_action:add_attachment("BUSTER")

local buster_sprite = buster_attachment:sprite()
buster_sprite:set_texture(user:get_texture())
buster_sprite:set_layer(-2)

local buster_animation = buster_attachment:get_animation()
buster_animation:copy_from(user:get_animation())
local derived_state = buster_animation:derive_state("BUSTER", frame_data)
buster_animation:set_state(derived_state)

-- spell
local cooldown_table = {
    { 5, 9, 13, 17, 21, 25 },
    { 4, 8, 11, 15, 18, 21 },
    { 4, 7, 10, 13, 16, 18 },
    { 3, 5, 7, 9, 11, 13 },
    { 3, 4, 5, 6, 7, 8 }
}

speed_level = math.max(math.min(speed_level, #cooldown_table), 1)

local cooldown = cooldown_table[speed_level][6]
local elapsed_frames = 0
local spell_erased_frame = 0

local spell = Battle.Spell.new(user:get_team())
local can_move = false

card_action.on_update_func = function()
    if spell_erased_frame == 0 and spell:will_erase_eof() then
        spell_erased_frame = elapsed_frames
    end

    elapsed_frames = elapsed_frames + 1

    if spell_erased_frame > 0 and elapsed_frames - spell_erased_frame >= cooldown then
        user:get_animation():resume()
    end

    if can_move then
        local motion_x = 0
        local motion_y = 0

        if user:has_input(Input.Left) then
            motion_x = motion_x - 1
        end

        if user:has_input(Input.Right) then
            motion_x = motion_x + 1
        end

        if user:has_input(Input.Up) then
            motion_y = motion_y - 1
        end

        if user:has_input(Input.Down) then
            motion_y = motion_y + 1
        end

        if (motion_x ~= 0 and user:can_move_to(user:get_tile(Direction.Right, motion_x))) or
            (motion_y ~= 0 and user:can_move_to(user:get_tile(Direction.Down, motion_y))) then
            card_action:end_action()
        end
    end
end

card_action:add_anim_action(2, function()
    Engine.play_audio(_game_folder_path .. "resources/sfx/pew.ogg", AudioPriority.High);

    local field = user:get_field()

    spell:set_hit_props(HitProps.new(
        damage,
        Hit.Impact,
        Element.None,
        context,
        Drag.None
    ))

    local tiles_travelled = 1
    local move_timer = 0

    spell.on_update_func = function()
        spell:get_tile():attack_entities(spell)

        move_timer = move_timer + 1

        if move_timer < 2 then
            return
        end

        local tile = spell:get_tile(spell:get_facing(), 1)

        if tile then
            tiles_travelled = tiles_travelled + 1
            spell:teleport(tile)
        else
            spell:delete()
        end

        move_timer = 0
    end

    spell.on_collision_func = function(self, entity)
        Engine.play_audio(_game_folder_path .. "resources/sfx/hurt.ogg");

        local hit_x = 0
        local hit_y = entity:get_height()
        local state = "HIT"

        if charged then
            hit_y = hit_y / 2
            state = "CHARGED_HIT"
        else
            hit_x = entity:sprite():get_width() * (math.random(0, 1) - 0.5)
            hit_y = math.random() * hit_y
        end

        local hit_artifact = Battle.Artifact.new()
        hit_artifact:set_animation(_game_folder_path .. "resources/scenes/battle/spell_bullet_hit.animation")
        hit_artifact:set_texture(_game_folder_path .. "resources/scenes/battle/spell_bullet_hit.png")
        hit_artifact:set_offset(hit_x, -hit_y)

        local hit_animation = hit_artifact:get_animation()
        hit_animation:set_state(state)
        hit_animation:on_complete(function()
            hit_artifact:erase()
        end)

        spell:get_field():spawn(hit_artifact, spell:get_tile())
        spell:delete()
    end

    spell.on_delete_func = function()
        local calculated_cooldown = cooldown_table[speed_level][tiles_travelled]

        if calculated_cooldown ~= nil then
            cooldown = calculated_cooldown
        end

        spell:erase()
    end

    field:spawn(spell, user:get_tile())
end)

-- flare attachment
card_action:add_anim_action(3, function()
    can_move = true
    local flare_attachment = buster_attachment:add_attachment("ENDPOINT")
    local flare_sprite = flare_attachment:sprite()
    flare_sprite:set_texture(_game_folder_path .. "resources/scenes/battle/buster_flare.png")
    flare_sprite:set_layer(-3)

    local animation = flare_attachment:get_animation()
    animation:load(_game_folder_path .. "resources/scenes/battle/buster_flare.animation")
    animation:set_state("DEFAULT")

    animation:refresh(flare_sprite)
end)

card_action:add_anim_action(4, function()
    local animation = user:get_animation()

    animation:on_interrupt(function()
        animation:resume()
    end)

    animation:pause()
end)

return card_action
