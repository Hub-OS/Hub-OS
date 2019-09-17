#pragma once

/**
* @brief Used by entity movement
*/
enum class Direction : int {
  NONE,
  UP,
  LEFT,
  DOWN,
  RIGHT
};

inline Direction Reverse(Direction in) {
  switch (in) {
  case Direction::UP:
    return Direction::DOWN;
  case Direction::LEFT:
    return Direction::RIGHT;
  case Direction::DOWN:
    return Direction::UP;
  case Direction::RIGHT:
    return Direction::LEFT;
  }

  return Direction::NONE;
}