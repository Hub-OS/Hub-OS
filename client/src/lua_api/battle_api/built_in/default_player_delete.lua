---@type Entity
local entity = ...

-- delete
entity:delete()

-- play the first flinch frame
local animation = entity:animation()
animation:set_state("CHARACTER_HIT")
animation:pause()

-- spawn shine artifact
local game_folder = Resources.game_folder()

local shine = Artifact.new()
shine:set_facing(entity:facing())
shine:set_texture(game_folder .. "resources/scenes/battle/shine.png")
shine:load_animation(game_folder .. "resources/scenes/battle/shine.animation")

local shine_animation = shine:animation()
shine_animation:set_state("DEFAULT")
shine_animation:on_complete(function()
  shine:delete()
end)

Field.spawn(shine, entity:current_tile())

-- create component
local START_DELAY = 25
local TOTAL_DURATION = 50
local i = 0

local sprite = entity:sprite()
local color = Color.new(255, 255, 255, 255)

local animate_component = entity:create_component(Lifetime.ActiveBattle)
animate_component.on_update_func = function()
  i = i + 1

  if i == 1 then
    Resources.play_audio(game_folder .. "resources/sfx/player_deleted.ogg")
  end

  if i > START_DELAY then
    local progress =
        (i - START_DELAY) / (TOTAL_DURATION - START_DELAY)

    color.a = (1.0 - progress) * 255
  end

  if i > TOTAL_DURATION then
    entity:erase()
  end
end


local color_component = entity:create_component(Lifetime.Scene)
color_component.on_update_func = function()
  sprite:set_shader_effect(SpriteShaderEffect.Pixelate)
  sprite:set_color(color)
end
