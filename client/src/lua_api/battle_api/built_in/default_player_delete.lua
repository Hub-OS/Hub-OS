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

entity:field():spawn(shine, entity:current_tile())

-- create component
local START_DELAY = 25
local TOTAL_DURATION = 50
local i = 0

local sprite = entity:sprite()
local color = Color.new(255, 255, 255, 255)

local component = entity:create_component(Lifetime.Battle)
component.on_update_func = function()
  i = i + 1

  if i == 1 then
    Resources.play_audio(game_folder .. "resources/sfx/player_deleted.ogg")
  end

  if i > START_DELAY then
    local progress =
        (i - START_DELAY) / (TOTAL_DURATION - START_DELAY)

    color.a = (1.0 - progress) * 255
  end

  sprite:set_color(color)

  if i > TOTAL_DURATION then
    entity:erase()
  end
end
