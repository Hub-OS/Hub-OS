---@param augment Augment
function augment_init(augment)
  local owner = augment:owner()

  -- reversed input
  augment.movement_input_func = function()
    local direction = Direction.None

    if owner:input_has(Input.Held.Left) then
      direction = Direction.join(direction, Direction.Right)
    end

    if owner:input_has(Input.Held.Right) then
      direction = Direction.join(direction, Direction.Left)
    end

    if owner:team() == Team.Blue then
      direction = Direction.reverse(direction)
    end

    if owner:input_has(Input.Held.Up) then
      direction = Direction.join(direction, Direction.Down)
    end

    if owner:input_has(Input.Held.Down) then
      direction = Direction.join(direction, Direction.Up)
    end

    if direction == Direction.None then
      return nil
    end

    return direction
  end
end
