---@type Entity
local entity, explosion_count = ...

local animation = entity:animation()

animation:pause()
entity:delete()

if animation:has_state("CHARACTER_HIT") then
  animation:set_state("CHARACTER_HIT", { { 1, 1 } })

  explosion_count = explosion_count or 5

  -- spawn shine artifact
  local game_folder = Resources.game_folder()

  local shine = Artifact.new()
  shine:set_facing(entity:facing())
  shine:set_texture(game_folder .. "resources/scenes/battle/shine.png")
  shine:load_animation(game_folder .. "resources/scenes/battle/shine.animation")

  shine:set_offset(0, -entity:height() // 2)

  local shine_animation = shine:animation()
  shine_animation:set_state("DEFAULT")
  shine_animation:set_playback(Playback.Loop)

  shine.on_update_func = function()
    if entity:will_erase_eof() then
      shine:delete()
    end
  end

  Field.spawn(shine, entity:current_tile())
else
  explosion_count = explosion_count or 2
end

local END_DELAY = 4
local EXPLOSION_RATE = 14
local TOTAL_DURATION = EXPLOSION_RATE * explosion_count + END_DELAY

local BLACK = Color.new(0, 0, 0, 255)
local WHITE = Color.new(255, 255, 255, 255)

local i = 0

local sprite = entity:sprite()
local component = entity:create_component(Lifetime.ActiveBattle)

component.on_update_func = function()
  i = i + 1

  -- spawn explosions
  if i % EXPLOSION_RATE == 1 and i < TOTAL_DURATION - END_DELAY then
    local explosion = Explosion.new()
    Field.spawn(explosion, entity:current_tile())

    explosion:sprite():set_layer(-1)

    -- random offset
    local x = math.random(-Tile:width() // 2, Tile:width() // 2)
    local y = math.random(-Tile:height() // 2, 0.0)

    -- add the parent's offsets
    local parent_offset = entity:offset()
    local movement_offset = entity:movement_offset()
    parent_offset.x = parent_offset.x + movement_offset.x
    parent_offset.y = parent_offset.y + movement_offset.y
    parent_offset.y = parent_offset.y + entity:elevation()

    explosion:set_offset(x + parent_offset.x, y + parent_offset.y)
  end

  if i >= TOTAL_DURATION then
    entity:erase()
  end
end


local color_component = entity:create_component(Lifetime.Scene)

color_component.on_update_func = function()
  -- flash the entity white
  local color = WHITE

  if (i // 2) % 2 == 0 then
    color = BLACK
  end

  sprite:set_color_mode(ColorMode.Add)
  sprite:set_color(color)
end
