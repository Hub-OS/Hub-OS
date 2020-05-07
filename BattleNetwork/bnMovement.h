#pragma once
#include "bnDirection.h"
#include "bnTile.h"

enum class MoveState : int {
  none,
  teleport,
  slide
};

struct Movement {
  MoveState state;
  Direction direction;
  float slideTime; // from tile A to tile B in seconds
  Battle::Tile* next;

  Movement() {
    state = MoveState::none;
    slideTime = 0.5; // 30 frames
    direction = Direction::none;
    next = nullptr;
  }

  Movement(const Movement& rhs) {
    state = rhs.state;
    slideTime = rhs.slideTime;
    direction = rhs.direction;
    next = rhs.next;
  }
};