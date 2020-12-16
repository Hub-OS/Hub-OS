#include "bnPlayerControlledState.h"
#include "bnInputManager.h"
#include "bnPlayer.h"
#include "bnCardAction.h"
#include "bnTile.h"
#include "bnSelectedCardsUI.h"
#include "bnAudioResourceManager.h"
#include "netplay/bnPlayerInputReplicator.h"

#include <iostream>

PlayerControlledState::PlayerControlledState() : AIState<Player>(), InputHandle(), replicator(nullptr)
{
  isChargeHeld = false;
  queuedAction = nullptr;
}


PlayerControlledState::~PlayerControlledState()
{
}

void PlayerControlledState::QueueAction(Player & player)
{
  // peek into the player's queued Action property
  auto action = player.DequeueAction();

  // We already have one action queued, delete the next one
  if (!queuedAction) {
    queuedAction = action;
    if(replicator) this->startupDelay = STARTUP_DELAY_LEN;
  }
  else {
    delete action;
  }

  player.GetChargeComponent().SetCharging(false);
  if (replicator) replicator->SendChargeSignal(false);

  isChargeHeld = false;
}

void PlayerControlledState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
  replicator = player.GetFirstComponent<PlayerInputReplicator>();
}

void PlayerControlledState::OnUpdate(double _elapsed, Player& player) {
  // Action startup time and actions themselves prevent player input
  if (this->startupDelay > 0.f) {
    this->startupDelay -= _elapsed;
    return;
  }

  // Actions with animation lockout controls take priority over movement
  auto actions = player.GetComponentsDerivedFrom<CardAction>();
  bool canMove = true;

  for (auto&& action : actions) {
    if (action->GetLockoutType() == ActionLockoutType::async && !action->IsAnimationOver()) {
      canMove = false;
      break;
    }
    else if(action->GetLockoutType() == ActionLockoutType::animation){
      canMove = canMove && action->IsLockoutOver();
    }
  }

  // One of our active actions are preventing us from moving
  if (!canMove) return;

  bool notAnimating = player.GetFirstComponent<AnimationComponent>()->GetAnimationString() == PLAYER_IDLE;

  // Are we creating an action this frame?
  if (player.CanAttack() && notAnimating) {
    if (Input().Has(InputEvents::pressed_use_chip)) {
      auto cardsUI = player.GetFirstComponent<SelectedCardsUI>();
      if (cardsUI && cardsUI->UseNextCard()) {
        // If the card used was successful, we may have a card in queue
        QueueAction(player);
        //return; // wait one more frame to use
      }
    }
    else if (Input().Has(InputEvents::released_special)) {
      if (replicator) replicator->SendUseSpecialSignal();
      player.UseSpecial();
      QueueAction(player);
     // return; // wait one more frame to use
    }    // queue attack based on input behavior (buster or charge?)
    else if ((!Input().Has(InputEvents::held_shoot) && isChargeHeld) || Input().Has(InputEvents::released_shoot)) {
      // This routine is responsible for determining the outcome of the attack
      player.Attack();

      if (replicator) {
        replicator->SendShootSignal();
        replicator->SendChargeSignal(false);
      }

      QueueAction(player);
      isChargeHeld = false;
      player.chargeEffect.SetCharging(false);
      //return; // wait one more frame to use
    }
  }

  // Movement increments are restricted based on anim speed at this time
  if (player.state != PLAYER_IDLE)
    return;

  static Direction direction = Direction::none;
  //if (!player.IsTimeFrozen()) { // TODO: take out IsTimeFrozen from API
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
  //}

  bool shouldShoot = Input().Has(InputEvents::held_shoot) && isChargeHeld == false && actions.empty();

#ifdef __ANDROID__
  shouldShoot = Input().Has(PRESSED_A);
#endif

  if (shouldShoot) {
    isChargeHeld = true;
    if (replicator) replicator->SendChargeSignal(true);
    player.chargeEffect.SetCharging(true);
  }

  if (Input().Has(InputEvents::released_move_up)) {
    direction = Direction::none;
  }
  else if (Input().Has(InputEvents::released_move_left)) {
    direction = Direction::none;
  }
  else if (Input().Has(InputEvents::released_move_down)) {
    direction = Direction::none;
  }
  else if (Input().Has(InputEvents::released_move_right)) {
    direction = Direction::none;
  }

  if (player.GetFirstComponent<AnimationComponent>()->GetAnimationString() != PLAYER_IDLE || player.IsSliding()) return;

  // TODO: this is a dumb hack, take this out
  if (player.PlayerControllerSlideEnabled()) {
    player.SlideToTile(true);
  }

  replicator? replicator->SendMoveSignal(direction) : (void(0));

  if(player.Move(direction)) {
    bool moved = player.GetNextTile();

    if (moved) {
      auto playerPtr = &player;

      auto onFinish = [playerPtr, actions, this]() {
        playerPtr->SetAnimation("PLAYER_MOVED", [playerPtr, actions, this]() {
          playerPtr->SetAnimation(PLAYER_IDLE);
          playerPtr->FinishMove();

          // Player should shoot or execute action on the immediate next tile
          // Note: I added this check because buster shoots would stay in queue
          // if the player was pressing the D-pad this frame too
          if (queuedAction && actions.empty()) {
            playerPtr->RegisterComponent(queuedAction);
            queuedAction->Execute();
            queuedAction = nullptr;
          }

          auto t = playerPtr->GetTile();

          replicator? replicator->SendTileSignal(t->GetX(), t->GetY()) : (void(0));
        });

        playerPtr->AdoptNextTile();
        direction = Direction::none;
      }; // end lambda
      player.GetFirstComponent<AnimationComponent>()->CancelCallbacks();
      player.SetAnimation(PLAYER_MOVING, onFinish);
    }
  }
  else if(queuedAction && !player.IsSliding()) {
    player.RegisterComponent(queuedAction);
    queuedAction = nullptr;
  }
  else if (player.IsSliding() && player.GetNextTile()) {
    auto t = player.GetTile();

    // we are sliding and have a valid next tile, update remote
    replicator ? replicator->SendTileSignal(t->GetX(), t->GetY()) : (void(0));
  }
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Navis lose charge when we leave this state */
  player.chargeEffect.SetCharging(false);

  if (auto queuedAction = player.DequeueAction(); queuedAction) {
    delete queuedAction;
  }

  replicator? replicator->SendChargeSignal(false) : (void(0));

  /* Cancel card actions */
  auto actions = player.GetComponentsDerivedFrom<CardAction>();

  for (auto a : actions) {
    a->EndAction();
  }
}
