#include "bnPlayerControlledState.h"
#include "bnInputManager.h"
#include "bnPlayer.h"
#include "bnChipAction.h"
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
  // Action controls take priority over movement
  if (player.GetComponentsDerivedFrom<ChipAction>().size()) return;

#ifndef __ANDROID__
  if (!INPUT.Has(HELD_A) && !player.IsSliding()) {
#else
    if(INPUT.Has(PRESSED_A) && !INPUT.Has(RELEASED_B) && !player.IsSliding() && !player.GetNextTile()) {
#endif
    if (player.chargeComponent.GetChargeCounter() > 0 && isChargeHeld == true) {
      player.Attack();
      player.chargeComponent.SetCharging(false);
      isChargeHeld = false;
      auto onFinish = [&]() {player.SetAnimation(PLAYER_IDLE);};
      player.SetAnimation(PLAYER_SHOOTING, onFinish);
    }
    else if(!player.GetNextTile()){
      isChargeHeld = false;

#ifdef __ANDROID__
      player.chargeComponent.SetCharging(false);
#endif
    }
  }

  // Movement increments are restricted based on anim speed at this time
  if (player.state != PLAYER_IDLE)
    return;

  static Direction direction = Direction::NONE;
  if (player.IsBattleActive()) {
    if (INPUT.Has(PRESSED_UP) ||INPUT.Has(HELD_UP)) {
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

  bool shouldShoot = INPUT.Has(HELD_A) && isChargeHeld == false;

#ifdef __ANDROID__
  shouldShoot = INPUT.Has(PRESSED_A);
#endif

  if (shouldShoot) {
    isChargeHeld = true;

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

  if (direction != Direction::NONE && player.state == PLAYER_IDLE && !player.IsSliding()) {
    if (player.PlayerControllerSlideEnabled()) {
      player.SlideToTile(true);
    }

    player.Move(direction);

    bool moved = player.GetNextTile();

    if (moved) {
      auto onFinish = [&]() {
        player.SetAnimation("PLAYER_MOVED", [p = &player]() {
			    p->SetAnimation(PLAYER_IDLE); 
          p->FinishMove();
        });

		    player.AdoptNextTile();
        direction = Direction::NONE;
      }; // end lambda
      player.SetAnimation(PLAYER_MOVING, onFinish);
    }
  }
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Mega loses charge when we leave this state */
  player.chargeComponent.SetCharging(false);

  /* Cancel chip actions */
  auto actions = player.GetComponentsDerivedFrom<ChipAction>();

  for (auto a : actions) {
    a->EndAction();
  }
}
