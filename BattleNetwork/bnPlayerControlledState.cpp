#include "bnPlayerControlledState.h"
#include "bnInputManager.h"
#include "bnPlayer.h"
#include "bnCardAction.h"
#include "bnTile.h"
#include "bnSelectedCardsUI.h"
#include "bnAudioResourceManager.h"

#include <iostream>

PlayerControlledState::PlayerControlledState() : AIState<Player>()
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
  auto action = player.queuedAction;
  player.queuedAction = nullptr;

  // We already have one action queued, delete the next one
  if (!queuedAction) {
    queuedAction = action;
  }
  else {
    delete action;
  }
}

void PlayerControlledState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
}

void PlayerControlledState::OnUpdate(float _elapsed, Player& player) {
  // Action controls take priority over movement
  if (player.GetComponentsDerivedFrom<CardAction>().size()) return;

  // Are we creating an action this frame?
  if (INPUT.Has(EventTypes::PRESSED_USE_CHIP)) {
    auto cardsUI = player.GetFirstComponent<SelectedCardsUI>();
    if (cardsUI) {
      cardsUI->UseNextCard();
      // If the card used was successful, the player will now have an active card in queue
      QueueAction(player);
    }
  } else if (INPUT.Has(EventTypes::PRESSED_SPECIAL)) {
    player.UseSpecial();
    QueueAction(player);
  }    // queue attack based on input behavior (buster or charge?)
  else if ((!INPUT.Has(EventTypes::HELD_SHOOT) && isChargeHeld) || INPUT.Has(EventTypes::RELEASED_SHOOT)) {
    // This routine is responsible for determining the outcome of the attack
    player.Attack();
    QueueAction(player);
    isChargeHeld = false;
    player.chargeEffect.SetCharging(false);
  }

  // Movement increments are restricted based on anim speed at this time
  if (player.state != PLAYER_IDLE)
    return;

  static Direction direction = Direction::NONE;
  if (!player.IsTimeFrozen()) {
    if (INPUT.Has(EventTypes::PRESSED_MOVE_UP) ||INPUT.Has(EventTypes::HELD_MOVE_UP)) {
      direction = Direction::UP;
    }
    else if (INPUT.Has(EventTypes::PRESSED_MOVE_LEFT) || INPUT.Has(EventTypes::HELD_MOVE_LEFT)) {
      direction = Direction::LEFT;
    }
    else if (INPUT.Has(EventTypes::PRESSED_MOVE_DOWN) || INPUT.Has(EventTypes::HELD_MOVE_DOWN)) {
      direction = Direction::DOWN;
    }
    else if (INPUT.Has(EventTypes::PRESSED_MOVE_RIGHT) || INPUT.Has(EventTypes::HELD_MOVE_RIGHT)) {
      direction = Direction::RIGHT;
    }
  }

  bool shouldShoot = INPUT.Has(EventTypes::HELD_SHOOT) && isChargeHeld == false;

#ifdef __ANDROID__
  shouldShoot = INPUT.Has(PRESSED_A);
#endif

  if (shouldShoot) {
    isChargeHeld = true;

    player.chargeEffect.SetCharging(true);
  }

  if (INPUT.Has(EventTypes::RELEASED_MOVE_UP)) {
    direction = Direction::NONE;
  }
  else if (INPUT.Has(EventTypes::RELEASED_MOVE_LEFT)) {
    direction = Direction::NONE;
  }
  else if (INPUT.Has(EventTypes::RELEASED_MOVE_DOWN)) {
    direction = Direction::NONE;
  }
  else if (INPUT.Has(EventTypes::RELEASED_MOVE_RIGHT)) {
    direction = Direction::NONE;
  }

  if (player.GetFirstComponent<AnimationComponent>()->GetAnimationString() != PLAYER_IDLE || player.IsSliding()) return;
    if (player.PlayerControllerSlideEnabled()) {
      player.SlideToTile(true);
    }

    if(player.Move(direction)) {

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
      player.GetFirstComponent<AnimationComponent>()->CancelCallbacks();
      player.SetAnimation(PLAYER_MOVING, onFinish);
    }
  }
  else if(queuedAction && !player.IsSliding()) {
    player.RegisterComponent(queuedAction);
    queuedAction->OnExecute();
    queuedAction = nullptr;

    player.chargeEffect.SetCharging(false);
    isChargeHeld = false;
  }
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Navis lose charge when we leave this state */
  player.chargeEffect.SetCharging(false);
  player.queuedAction = nullptr;
  
  /* Cancel card actions */
  auto actions = player.GetComponentsDerivedFrom<CardAction>();

  for (auto a : actions) {
    a->EndAction();
  }
}
