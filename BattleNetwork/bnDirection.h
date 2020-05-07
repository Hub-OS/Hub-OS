#pragma once

/**
* @brief Used by entity movement
*/
enum class Direction : int {
  none,
  up,
  left,
  down,
  right
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