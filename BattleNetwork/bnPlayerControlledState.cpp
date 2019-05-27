#include "bnPlayerControlledState.h"
#include "bnInputManager.h"
#include "bnPlayer.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

#include <iostream>

PlayerControlledState::PlayerControlledState() : AIState<Player>()
{
  isChargeHeld = false;
}


PlayerControlledState::~PlayerControlledState()
{
}

void PlayerControlledState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
}

void PlayerControlledState::OnUpdate(float _elapsed, Player& player) {
  if (!player.IsBattleActive()) return;

  // Action controls take priority over movement
<<<<<<< HEAD
#ifndef __ANDROID__
  if (!inputManager->Has(HELD_A)) {
#else
    if(inputManager->Has(PRESSED_A) && !inputManager->Has(RELEASED_B)) {
#endif
=======
  if (!INPUT.Has(HELD_A)) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    if (player.chargeComponent.GetChargeCounter() > 0 && isChargeHeld == true) {
      player.Attack();
      player.chargeComponent.SetCharging(false);
      isChargeHeld = false;
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

<<<<<<< HEAD
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
=======
  Direction direction = Direction::NONE;
  
  if (player.IsBattleActive()) {
    if (INPUT.Has(PRESSED_UP) || INPUT.Has(HELD_UP)) {
      direction = Direction::UP;
    }
    else if (INPUT.Has(PRESSED_LEFT) || INPUT.Has(HELD_LEFT)) {
      direction = Direction::LEFT;
    }
    else if (INPUT.Has(PRESSED_DOWN) || INPUT.Has(HELD_DOWN)) {
      direction = Direction::DOWN;
    }
    else if (INPUT.Has(PRESSED_RIGHT) || INPUT.Has(HELD_RIGHT)) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      direction = Direction::RIGHT;
    }
  }

<<<<<<< HEAD
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
=======
  if (INPUT.Has(HELD_A) && isChargeHeld == false) {
    isChargeHeld = true;
    
    // TODO: player.GetComponent<ChargeComponent>()->SetCharging(true);
    player.chargeComponent.SetCharging(true);
  }

  if (INPUT.Has(RELEASED_UP)) {
    direction = Direction::NONE;
  }
  else if (INPUT.Has(RELEASED_LEFT)) {
    direction = Direction::NONE;
  }
  else if (INPUT.Has(RELEASED_DOWN)) {
    direction = Direction::NONE;
  }
  else if (INPUT.Has(RELEASED_RIGHT)) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    direction = Direction::NONE;
  }

  //std::cout << "Is player slideing: " << player.isSliding << std::endl;

<<<<<<< HEAD
  if (direction != Direction::NONE && player.state != PLAYER_SHOOTING && !player.isSliding) {
=======
  if (direction != Direction::NONE && player.state == PLAYER_IDLE && !player.isSliding) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    bool moved = player.Move(direction);

    if (moved) {
      auto onFinish = [&]() {
        player.SetAnimation("PLAYER_MOVED", [p = &player]() {
<<<<<<< HEAD
			p->SetAnimation(PLAYER_IDLE);
        });

=======
			p->SetAnimation(PLAYER_IDLE); });
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
		player.AdoptNextTile();
        direction = Direction::NONE;
      }; // end lambda
      player.SetAnimation(PLAYER_MOVING, onFinish);
    }
    else {
      player.SetAnimation(PLAYER_IDLE);
    }
  }
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Mega loses charge when we release control */
  player.chargeComponent.SetCharging(false);
}
