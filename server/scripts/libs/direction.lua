local Direction = {
  UP = "UP",
  LEFT = "LEFT",
  DOWN = "DOWN",
  RIGHT = "RIGHT",
  UP_LEFT = "UP LEFT",
  UP_RIGHT = "UP RIGHT",
  DOWN_LEFT = "DOWN LEFT",
  DOWN_RIGHT = "DOWN RIGHT",
}

local reverse_table = {
  [Direction.UP] = Direction.DOWN,
  [Direction.LEFT] = Direction.LEFT,
  [Direction.DOWN] = Direction.UP,
  [Direction.RIGHT] = Direction.LEFT,
  [Direction.UP_LEFT] = Direction.DOWN_RIGHT,
  [Direction.UP_RIGHT] = Direction.DOWN_LEFT,
  [Direction.DOWN_LEFT] = Direction.UP_RIGHT,
  [Direction.DOWN_RIGHT] = Direction.UP_LEFT,
}

function Direction.reverse(direction)
  return reverse_table[direction]
end

function Direction.from_points(point_a, point_b)
  local a_z_offset = point_a.z / 2
  local a_x = point_a.x - a_z_offset
  local a_y = point_a.y - a_z_offset

  local b_z_offset = point_b.z / 2
  local b_x = point_b.x - b_z_offset
  local b_y = point_b.y - b_z_offset

  return Direction.from_offset(b_x - a_x, b_y - a_y)
end

function Direction.diagonal_from_points(point_a, point_b)
  local a_z_offset = point_a.z / 2
  local a_x = point_a.x - a_z_offset
  local a_y = point_a.y - a_z_offset

  local b_z_offset = point_b.z / 2
  local b_x = point_b.x - b_z_offset
  local b_y = point_b.y - b_z_offset

  return Direction.diagonal_from_offset(b_x - a_x, b_y - a_y)
end

local directions = {
  Direction.UP_LEFT,
  Direction.UP,
  Direction.UP_RIGHT,
  Direction.RIGHT,
  Direction.DOWN_RIGHT,
  Direction.DOWN,
  Direction.DOWN_LEFT,
  Direction.LEFT,
}

function Direction.from_offset(x, y)
  local angle = math.atan(y, x)
  local direction_index = math.floor(angle / math.pi * 4 + 4.5)
  return directions[direction_index % 8 + 1]
end

function Direction.diagonal_from_offset(x, y)
  if math.abs(x) > math.abs(y) then
    -- x axis direction
    if x < 0 then
      return Direction.UP_LEFT
    else
      return Direction.DOWN_RIGHT
    end
  else
    -- y axis direction
    if y < 0 then
      return Direction.UP_RIGHT
    else
      return Direction.DOWN_LEFT
    end
  end
end

return Direction
