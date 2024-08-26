local COLOR = Color.new(0, 0, 0, 0)

---@param status Status
function status_init(status)
  local entity = status:owner()
  local entity_sprite = entity:sprite()

  local removed_intangible_rule = false

  local intangible_rule = IntangibleRule.new()
  intangible_rule.duration = status:remaining_time()
  intangible_rule.on_deactivate_func = function()
    removed_intangible_rule = true
    entity:remove_status(Hit.Flash)
  end
  entity:set_intangible(true, intangible_rule)

  -- animation
  local component = entity:create_component(Lifetime.Battle)
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

    if not removed_intangible_rule then
      entity:set_intangible(false)
    end
  end
end
