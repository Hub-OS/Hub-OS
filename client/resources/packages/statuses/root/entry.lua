local COLOR = Color.new(0, 0, 0, 255)

---@param status Status
function status_init(status)
  local entity = status:owner()
  local entity_sprite = entity:sprite()

  local component = entity:create_component(Lifetime.ActiveBattle)
  local time = 0

  component.on_update_func = function()
    if time // 2 % 2 == 1 then
      entity_sprite:set_color_mode(ColorMode.Multiply)
      entity_sprite:set_color(COLOR)
    end

    time = time + 1
  end

  -- clean up
  status.on_delete_func = function()
    component:eject()
  end
end
