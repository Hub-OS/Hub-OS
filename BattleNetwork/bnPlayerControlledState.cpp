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
  if (!INPUT.Has(HELD_A)) {
    if (player.chargeComponent.GetChargeCounter() > 0 && isChargeHeld == true) {
      player.Attack();
      player.chargeComponent.SetCharging(false);
      isChargeHeld = false;
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
      direction = Direction::RIGHT;
    }
  }
 

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
    direction = Direction::NONE;
  }

  //std::cout << "Is player slideing: " << player.isSliding << std::endl;

  if (direction != Direction::NONE && player.state == PLAYER_IDLE && !player.isSliding) {
    bool moved = player.Move(direction);

    if (moved) {
      auto onFinish = [&]() {
        player.SetAnimation("PLAYER_MOVED", [p = &player]() {
			p->SetAnimation(PLAYER_IDLE); });
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
