#include "bnPlayerControlledState.h"
#include "bnInputManager.h"
#include "bnPlayer.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

#include <iostream>

#define MOVE_KEY_PRESS_COOLDOWN 200.0f
#define MOVE_LAG_COOLDOWN 40.0f
#define ATTACK_KEY_PRESS_COOLDOWN 300.0f
#define ATTACK_TO_IDLE_COOLDOWN 150.0f
#define HIT_COOLDOWN 300.0f

PlayerControlledState::PlayerControlledState() : inputManager(&InputManager::GetInstance()), AIState<Player>()
{
  //Cooldowns. TODO: Take these out. We base actions on animation speed now.
  moveKeyPressCooldown = MOVE_KEY_PRESS_COOLDOWN;
  attackKeyPressCooldown = ATTACK_KEY_PRESS_COOLDOWN;
  attackToIdleCooldown = 0.0f;
  previousDirection = Direction::NONE;
  isChargeHeld = false;
}


PlayerControlledState::~PlayerControlledState()
{
  inputManager = nullptr;
}

void PlayerControlledState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
}

void PlayerControlledState::OnUpdate(float _elapsed, Player& player) {
  if (!player.IsBattleActive()) return;

  // Action controls take priority over movement
  if (inputManager->has(RELEASED_A)) {
    if (player.chargeComponent.GetChargeCounter() > 0 && isChargeHeld == true) {
      player.Attack(player.chargeComponent.GetChargeCounter());
      player.chargeComponent.SetCharging(false);
      isChargeHeld = false;
      attackKeyPressCooldown = 0.0f;
      auto onFinish = [&]() {player.SetAnimation(PLAYER_IDLE);};
      player.SetAnimation(PLAYER_SHOOTING, onFinish);
    }
    else {
      isChargeHeld = false;
    }
  }

  // Movement increments are restricted based on anim speed
  if (player.state != PLAYER_IDLE)
    return;

  static Direction direction = Direction::NONE;
  if (moveKeyPressCooldown >= MOVE_KEY_PRESS_COOLDOWN && player.IsBattleActive()) {
    if (inputManager->has(PRESSED_UP) || inputManager->has(HELD_UP)) {
      direction = Direction::UP;
    }
    else if (inputManager->has(PRESSED_LEFT) || inputManager->has(HELD_LEFT)) {
      direction = Direction::LEFT;
    }
    else if (inputManager->has(PRESSED_DOWN) || inputManager->has(HELD_DOWN)) {
      direction = Direction::DOWN;
    }
    else if (inputManager->has(PRESSED_RIGHT) || inputManager->has(HELD_RIGHT)) {
      direction = Direction::RIGHT;
    }
  }
 

  if (inputManager->has(HELD_A) && isChargeHeld == false) {
    isChargeHeld = true;
    attackKeyPressCooldown = 0.0f;

    player.chargeComponent.SetCharging(true);
    this->attackKeyPressCooldown = ATTACK_KEY_PRESS_COOLDOWN; 
  }

  if (inputManager->has(RELEASED_UP)) {
    direction = Direction::NONE;
  }
  else if (inputManager->has(RELEASED_LEFT)) {
    direction = Direction::NONE;
  }
  else if (inputManager->has(RELEASED_DOWN)) {
    direction = Direction::NONE;
  }
  else if (inputManager->has(RELEASED_RIGHT)) {
    direction = Direction::NONE;
  }

  //std::cout << "Is player slideing: " << player.isSliding << std::endl;

  if (direction != Direction::NONE && player.state != PLAYER_SHOOTING && !player.isSliding) {
    bool moved = player.Move(direction);

    if (moved) {
      moveKeyPressCooldown = 0.0f;
      auto onFinish = [&]() {
        //Cooldown until player's movement catches up to actual position (avoid walking through spells)
       //if (moved) {
          player.AdoptNextTile();
        //}
        player.SetAnimation(PLAYER_IDLE);
        direction = Direction::NONE;
      }; // end lambda
      player.SetAnimation(PLAYER_MOVING, onFinish);
    }
    else {
      player.SetAnimation(PLAYER_IDLE);
    }
    moveKeyPressCooldown = MOVE_KEY_PRESS_COOLDOWN;
  }
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Mega loses charge when we release control */
  player.chargeComponent.SetCharging(false);
}
