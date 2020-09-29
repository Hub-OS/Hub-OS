#include "bnCardSelectBattleState.h"

CardSelectBattleState::CardSelectBattleState() {
  // Selection input delays
  heldCardSelectInputCooldown = 0.35f; // 21 frames @ 60fps = 0.35 second
  maxCardSelectInputCooldown = 1 / 12.f; // 5 frames @ 60fps = 1/12th second
  cardSelectInputCooldown = maxCardSelectInputCooldown;
}

bool CardSelectBattleState::OKIsPressed() {
    return true;
}