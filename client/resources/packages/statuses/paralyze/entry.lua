-- mashing helper
local inputs = {}

for _, value in pairs(Input.Pressed) do
  inputs[#inputs + 1] = value
end

---@param entity Entity
local function is_mashing(entity)
  for _, value in ipairs(inputs) do
    if entity:input_has(value) then
      return true
    end
  end

  return false
end

local COLOR = Color.new(255, 255, 0, 255)

---@param status Status
function status_init(status)
  local entity = status:owner()
  local entity_sprite = entity:sprite()

  -- this component updates the status's animation and handles mashing
  local component = entity:create_component(Lifetime.Battle)
  local time = 0

  component.on_update_func = function()
    if time // 2 % 2 == 0 then
      entity_sprite:set_color_mode(ColorMode.Additive)
      entity_sprite:set_color(COLOR)
    end

    time = time + 1

    if is_mashing(entity) then
      local remaining_time = status:remaining_time()
      status:set_remaining_time(remaining_time - 1)
    end
  end

  -- clean up
  status.on_delete_func = function()
    component:eject()
  end
end
