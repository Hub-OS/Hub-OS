---@param status Status
function status_init(status)
  local entity = status:owner()

  local component = entity:create_component(Lifetime.Scene)
  local ejected = false

  component.on_update_func = function()
    local movement_offset = entity:movement_offset()

    entity:set_movement_offset(
      movement_offset.x + math.random(-1, 1),
      movement_offset.y + math.random(-1, 1)
    )

    if not TurnGauge.frozen() then
      return
    end

    -- update time during time freeze
    local remaining_time = status:remaining_time() - 1
    status:set_remaining_time(remaining_time)

    if remaining_time <= 0 then
      component:eject()
      ejected = true
    end
  end

  status.on_delete_func = function()
    if not ejected then
      component:eject()
    end
  end
end
