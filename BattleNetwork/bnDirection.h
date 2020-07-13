#pragma once

/**
* @brief Used by entity movement
*/
enum class Direction : char {
  none = 0x00,
  up = 0x01,
  left = 0x02,
  down = 0x04,
  right = 0x08
};

inline Direction Reverse(Direction in) {
  switch (in) {
  case Direction::up:
    return Direction::down;
  case Direction::left:
    return Direction::right;
  case Direction::down:
    return Direction::up;
  case Direction::right:
    return Direction::left;
  }

  return Direction::none;
}