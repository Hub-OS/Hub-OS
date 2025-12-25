local COLOR = Color.new(0, 0, 0, 255)

---@param status Status
function status_init(status)
  local entity = status:owner()
  local entity_sprite = entity:sprite()

  local component = entity:create_component(Lifetime.ActiveBattle)
  local time = 0
  local apply_color = false

  component.on_update_func = function()
    apply_color = time // 2 % 2 == 1

    if apply_color then
      entity_sprite:set_color_mode(ColorMode.Multiply)

      local color = entity:color()
      color.r = COLOR.r
      color.g = COLOR.g
      color.b = COLOR.b
      entity_sprite:set_color(color)
    end

    time = time + 1
  end

  local shift_timing_component = entity:create_component(Lifetime.Scene)
  shift_timing_component.on_update_func = function()
    if not apply_color then
      return
    end

    apply_color = false

    local color = entity_sprite:color()

    if
        entity_sprite:color_mode() == ColorMode.Multiply and
        color.r == COLOR.r and
        color.g == COLOR.g and
        color.b == COLOR.b and
        color.a == COLOR.a
    then
      return
    end

    -- color differs, shift time
    time = time + 1
  end

  -- clean up
  status.on_delete_func = function()
    component:eject()
    shift_timing_component:eject()
  end
end
