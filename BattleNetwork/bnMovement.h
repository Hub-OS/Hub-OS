#pragma once
#include "bnDirection.h"
#include "bnTile.h"

enum class MoveState : int {
  NONE,
  TELEPORT,
  SLIDE
};

struct Movement {
  MoveState state;
  Direction direction;
  float slideTime; // from tile A to tile B in seconds
  Battle::Tile* next;

  Movement() {
    state = MoveState::NONE;
    slideTime = 0.5; // 30 frames
    direction = Direction::NONE;
    next = nullptr;
  }

  Movement(const Movement& rhs) {
    state = rhs.state;
    slideTime = rhs.slideTime;
    direction = rhs.direction;
    next = rhs.next;
  }
};