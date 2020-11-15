#pragma once
#include <assert.h>

/**
* @brief Used by entity movement
*/
enum class Direction : unsigned char {
  none = 0x00,

  // cardinal
  up = 0x01,
  left = 0x02,
  down = 0x04,
  right = 0x08,

  // diagonal 
  up_left = 0x10,
  up_right = 0x20,
  down_left = 0x40,
  down_right = 0x80
};

/**
* @brief Reverses the direction input
*/
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
  case Direction::up_left:
    return Direction::down_right;
  case Direction::up_right:
    return Direction::down_left;
  case Direction::down_left:
    return Direction::up_right;
  case Direction::down_right:
    return Direction::up_left;
  }

  return Direction::none;
}

/**
* @brief Splits 2-dimensional direction values into a 1-dimensional direction pair
*/
inline std::pair<Direction, Direction> Split(const Direction& dir) {
  Direction first = dir;
  Direction second = Direction::none;

  switch (dir) {
  case Direction::up_left:
    first = Direction::up;
    second = Direction::left;
    break;
  case Direction::up_right:
    first = Direction::up;
    second = Direction::right;
    break;
  case Direction::down_left:
    first = Direction::down;
    second = Direction::left;
    break;
  case Direction::down_right:
    first = Direction::down;
    second = Direction::right;
    break;
  }

  return std::make_pair(first, second);
}

/**
* @brief Joins two 1-dimensional direction values into a potential 2-dimensional direction value
* 
* If the 2 joined directions are polar opposites, Direction::none is returned
* If the 2 joined directions are the same, then only a 1-dimensional direction value is returned
* If the 2 joined directions are 1D and 2D then the polar sum directional value is returned
*/
inline Direction Join(const Direction& A, const Direction& B) {
  auto [A1, A2] = Split(A);

  auto addThunk = [](const Direction& C, const Direction& D) -> Direction {
    if (C == Direction::none) {
      return D;
    }

    if (C == Direction::left) {
      switch (D) {
      case Direction::up:
        return Direction::up_left;
      case Direction::left:
        return Direction::left;
      case Direction::down:
        return Direction::down_left;
      case Direction::right:
        return Direction::none;
      case Direction::up_left:
        return Direction::up_left;
      case Direction::up_right:
        return Direction::up;
      case Direction::down_left:
        return Direction::down_left;
      case Direction::down_right:
        return Direction::down;
      }
    }
    else if (C == Direction::right) {
      switch (D) {
      case Direction::up:
        return Direction::up_right;
      case Direction::left:
        return Direction::none;
      case Direction::down:
        return Direction::down_right;
      case Direction::right:
        return Direction::right;
      case Direction::up_left:
        return Direction::up;
      case Direction::up_right:
        return Direction::up_right;
      case Direction::down_left:
        return Direction::down;
      case Direction::down_right:
        return Direction::down_right;
      }
    }
    else if (C == Direction::up) {
      switch (D) {
      case Direction::up:
        return Direction::up;
      case Direction::left:
        return Direction::up_left;
      case Direction::down:
        return Direction::none;
      case Direction::right:
        return Direction::up_right;
      case Direction::up_left:
        return Direction::up_left;
      case Direction::up_right:
        return Direction::up_right;
      case Direction::down_left:
        return Direction::left;
      case Direction::down_right:
        return Direction::right;
      }
    }
    else if (C == Direction::down) {
      switch (D) {
      case Direction::up:
        return Direction::none;
      case Direction::left:
        return Direction::down_left;
      case Direction::down:
        return Direction::down;
      case Direction::right:
        return Direction::down_right;
      case Direction::up_left:
        return Direction::left;
      case Direction::up_right:
        return Direction::right;
      case Direction::down_left:
        return Direction::down_left;
      case Direction::down_right:
        return Direction::down_right;
      }
    }

    assert(false && "Case not checked in Join(Direction, Direction)");
  };

  Direction A1_plus_B = addThunk(A1, B);
  Direction A2_plus_B = addThunk(A2, B);

  // compare enum values for diagonal values
  // cardinal directions go in the thunk's first argument
  // This simply made it easy to avoid writing less case checks...
  if (A1_plus_B < A2_plus_B) {
    return addThunk(A1_plus_B, A2_plus_B);
  }

  return addThunk(A2_plus_B, A1_plus_B);
}