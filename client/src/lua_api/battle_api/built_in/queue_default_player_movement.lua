---@type Entity, Tile
local entity, tile = ...

if not entity:can_move_to(tile) then
  return
end

tile:reserve_for(entity)

local movement

if entity:slide_when_moving() then
  movement = Movement.new_slide(tile, 14)
else
  movement = Movement.new_teleport(tile)
  movement.delay = 5
  movement.endlag = 7

  local animation = entity:animation()
  animation:set_state(entity:player_move_state())

  -- idle when movement finishes
  animation:on_complete(function()
    entity:set_idle()
  end)
end

movement.on_end_func = function()
  tile:add_entity(entity)
  tile:remove_reservation_for(entity)
end

entity:queue_movement(movement)
