#include "bnPlayerControlledState.h"
#include "bnInputManager.h"
#include "bnPlayer.h"
#include "bnCardAction.h"
#include "bnTile.h"
#include "bnSelectedCardsUI.h"
#include "bnAudioResourceManager.h"
#include "netplay/bnPlayerInputReplicator.h"

#include <iostream>

PlayerControlledState::PlayerControlledState() : 
  AIState<Player>(), 
  InputHandle(), 
  replicator(nullptr),
  isChargeHeld(false)
{
}


PlayerControlledState::~PlayerControlledState()
{
}

void PlayerControlledState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
  replicator = player.GetFirstComponent<PlayerInputReplicator>();
}

void PlayerControlledState::OnUpdate(double _elapsed, Player& player) {
  // Actions with animation lockout controls take priority over movement
  bool canMove = player.IsLockoutComplete();

  // One of our active actions are preventing us from moving
  if (!canMove) return;

  bool notAnimating = player.GetFirstComponent<AnimationComponent>()->GetAnimationString() == PLAYER_IDLE;

  // Are we creating an action this frame?
    if (Input().Has(InputEvents::pressed_use_chip)) {
      auto cardsUI = player.GetFirstComponent<SelectedCardsUI>();
      if (cardsUI) {
        cardsUI->UseNextCard();
      }
      // If the card used was successful, we may have a card in queue
    }
    else if (Input().Has(InputEvents::released_special)) {
      if (replicator) replicator->SendUseSpecialSignal();
      player.UseSpecial();
    }    // queue attack based on input behavior (buster or charge?)
    else if ((!Input().Has(InputEvents::held_shoot) && isChargeHeld) || Input().Has(InputEvents::released_shoot)) {
      // This routine is responsible for determining the outcome of the attack
      player.Attack();

      if (replicator) {
        replicator->SendShootSignal();
        replicator->SendChargeSignal(false);
      }

      isChargeHeld = false;
      player.chargeEffect.SetCharging(false);
    }


  // Movement increments are restricted based on anim speed at this time
  if (player.IsMoving())
    return;

  Direction direction = Direction::none;
  if (Input().Has(InputEvents::pressed_move_up) || Input().Has(InputEvents::held_move_up)) {
    direction = Direction::up;
  }
  else if (Input().Has(InputEvents::pressed_move_left) || Input().Has(InputEvents::held_move_left)) {
    direction = Direction::left;
  }
  else if (Input().Has(InputEvents::pressed_move_down) || Input().Has(InputEvents::held_move_down)) {
    direction = Direction::down;
  }
  else if (Input().Has(InputEvents::pressed_move_right) || Input().Has(InputEvents::held_move_right)) {
    direction = Direction::right;
  }

  bool shouldShoot = Input().Has(InputEvents::held_shoot) && isChargeHeld == false && player.CanAttack();

#ifdef __ANDROID__
  shouldShoot = Input().Has(PRESSED_A);
#endif

  if (shouldShoot) {
    isChargeHeld = true;
    if (replicator) replicator->SendChargeSignal(true);
    player.chargeEffect.SetCharging(true);
  }

  if(direction != Direction::none) {
    replicator ? replicator->SendMoveSignal(direction) : (void(0));

    player.Teleport(player.GetTile() + direction);
  } 
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Navis lose charge when we leave this state */
  player.chargeEffect.SetCharging(false);

  replicator? replicator->SendChargeSignal(false) : (void(0));
}
