---@type Entity, Tile
local entity, tile = ...

if not entity:can_move_to(tile) then
  return
end

tile:reserve_for(entity)

local movement

if Player.from(entity) and entity:slide_when_moving() then
  movement = Movement.new_slide(tile, 14)
else
  movement = Movement.new_teleport(tile)
  movement.delay = 5
  movement.endlag = 8

  local FRAMES = {
    { 1, 2 },
    { 2, 1 },
    { 3, 1 },
    { 4, 1 },
    { 3, 1 },
    { 2, 1 },
    { 1, 2 },
  }

  local animation = entity:animation()
  animation:set_state("CHARACTER_MOVE", FRAMES)

  -- idle when movement finishes
  animation:on_complete(function()
    entity:set_idle()
  end)
end

movement.on_end_func = function()
  if entity:can_move_to(tile) then
    tile:add_entity(entity)
  end
  tile:remove_reservation_for(entity)
end

entity:queue_movement(movement)
