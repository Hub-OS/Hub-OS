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
#ifndef __ANDROID__
  if (!inputManager->Has(HELD_A)) {
#else
    if(inputManager->Has(PRESSED_A) && !inputManager->Has(RELEASED_B)) {
#endif
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

#ifdef __ANDROID__
      player.chargeComponent.SetCharging(false);
#endif
    }
  }

  // Movement increments are restricted based on anim speed
  if (player.state != PLAYER_IDLE)
    return;

  static Direction direction = Direction::NONE;
  if (moveKeyPressCooldown >= MOVE_KEY_PRESS_COOLDOWN && player.IsBattleActive()) {
    if (inputManager->Has(PRESSED_UP) || inputManager->Has(HELD_UP)) {
      direction = Direction::UP;
    }
    else if (inputManager->Has(PRESSED_LEFT) || inputManager->Has(HELD_LEFT)) {
      direction = Direction::LEFT;
    }
    else if (inputManager->Has(PRESSED_DOWN) || inputManager->Has(HELD_DOWN)) {
      direction = Direction::DOWN;
    }
    else if (inputManager->Has(PRESSED_RIGHT) || inputManager->Has(HELD_RIGHT)) {
      direction = Direction::RIGHT;
    }
  }

  bool shouldShoot = inputManager->Has(HELD_A) && isChargeHeld == false;

#ifdef __ANDROID__
  shouldShoot = inputManager->Has(PRESSED_A);
#endif

  if (shouldShoot) {
    isChargeHeld = true;
    attackKeyPressCooldown = 0.0f;

    player.chargeComponent.SetCharging(true);
    this->attackKeyPressCooldown = ATTACK_KEY_PRESS_COOLDOWN; 
  }

  if (inputManager->Has(RELEASED_UP)) {
    direction = Direction::NONE;
  }
  else if (inputManager->Has(RELEASED_LEFT)) {
    direction = Direction::NONE;
  }
  else if (inputManager->Has(RELEASED_DOWN)) {
    direction = Direction::NONE;
  }
  else if (inputManager->Has(RELEASED_RIGHT)) {
    direction = Direction::NONE;
  }

  //std::cout << "Is player slideing: " << player.isSliding << std::endl;

  if (direction != Direction::NONE && player.state != PLAYER_SHOOTING && !player.isSliding) {
    bool moved = player.Move(direction);

    if (moved) {
      moveKeyPressCooldown = 0.0f;
      auto onFinish = [&]() {
        player.SetAnimation("PLAYER_MOVED", [p = &player]() {
			p->SetAnimation(PLAYER_IDLE);
        });

		player.AdoptNextTile();
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
