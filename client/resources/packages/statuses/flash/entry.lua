local COLOR = Color.new(0, 0, 0, 0)

---@param status Status
function status_init(status)
  local entity = status:owner()

  if entity:intangible() and status:reapplied() then
    entity:remove_status(Hit.Flash)
    return
  end

  local entity_sprite = entity:sprite()

  local removed_intangible_rule = false
  local reapplying_intangible_rule = false

  local function apply_rule()
    removed_intangible_rule = false

    local intangible_rule = IntangibleRule.new()
    intangible_rule.duration = status:remaining_time()
    intangible_rule.on_deactivate_func = function()
      if not reapplying_intangible_rule then
        removed_intangible_rule = true
        entity:remove_status(Hit.Flash)
      end
      reapplying_intangible_rule = false
    end
    entity:set_intangible(true, intangible_rule)
  end

  apply_rule()

  -- animation
  local component = entity:create_component(Lifetime.ActiveBattle)
  local time = 0
  local prev_remaining_time = status:remaining_time()

  component.on_update_func = function()
    if time // 2 % 2 == 1 then
      entity_sprite:set_color_mode(ColorMode.Multiply)
      entity_sprite:set_color(COLOR)
    end

    local remaining_time = status:remaining_time()

    if prev_remaining_time - remaining_time ~= 1 and remaining_time ~= 0 then
      reapplying_intangible_rule = true
      apply_rule()
    end

    prev_remaining_time = remaining_time

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
