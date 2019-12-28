#include "bnPlayerControlledState.h"
#include "bnInputManager.h"
#include "bnPlayer.h"
#include "bnChipAction.h"
#include "bnTile.h"
#include "bnSelectedChipsUI.h"
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

const bool PlayerControlledState::CanTakeAction(Player& player) const
{
  auto anim = player.GetFirstComponent<AnimationComponent>();
  return player.GetTile()
    && anim && anim->GetAnimationString() == "PLAYER_IDLE"
    && !player.IsSliding();
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
  if (player.GetComponentsDerivedFrom<ChipAction>().size()) return;

  // Are we creating an action this frame?
  if (CanTakeAction(player) && INPUT.Has(EventTypes::PRESSED_USE_CHIP)) {
    auto chipsUI = player.GetFirstComponent<SelectedChipsUI>();
    if (chipsUI) {
      chipsUI->UseNextChip();
      // If the chip used was successful, the player will now have an active chip component
      auto activeChips = player.GetComponentsDerivedFrom<ChipAction>();

      if (activeChips.size()) {
        // We have a chip action,
        // execute the first action
        // There shouldn't be any more, but if there are
        // they will be flushed
        activeChips[0]->OnExecute();

        return;
      }
    }
  }

  if (CanTakeAction(player)) {
    if (INPUT.Has(EventTypes::PRESSED_SPECIAL)) {
      player.UseSpecial();
      QueueAction(player);
    }
#ifndef __ANDROID__
    else if (!INPUT.Has(EventTypes::HELD_SHOOT)) {
#else
    else if (CanTakeAction(player) && INPUT.Has(EventTypes::PRESSED_USE_CHIP) && !INPUT.Has(EventTypes::RELEASED_SHOOT)) {
#endif
      if (player.chargeEffect.GetChargeCounter() > 0 && isChargeHeld == true) {
        player.Attack();
        QueueAction(player);
      }
      else if (!player.GetNextTile()) {
        isChargeHeld = false;

#ifdef __ANDROID__
        player.chargeComponent.SetCharging(false);
#endif
      }
    }  }

  // Movement increments are restricted based on anim speed at this time
  if (player.state != PLAYER_IDLE)
    return;

  static Direction direction = Direction::NONE;
  if (player.IsBattleActive()) {
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

  if (direction != Direction::NONE && player.GetFirstComponent<AnimationComponent>()->GetAnimationString() == PLAYER_IDLE && !player.IsSliding()) {
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
      player.GetFirstComponent<AnimationComponent>()->CancelCallbacks();
      player.SetAnimation(PLAYER_MOVING, onFinish);
    }
  }
  else if(queuedAction) {
    if (this->CanTakeAction(player)) {
      queuedAction->OnExecute();
      player.RegisterComponent(queuedAction);
      queuedAction = nullptr;

      player.chargeEffect.SetCharging(false);
      isChargeHeld = false;
    }
  }
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Navis lose charge when we leave this state */
  player.chargeEffect.SetCharging(false);
  player.queuedAction = nullptr;
  
  /* Cancel chip actions */
  auto actions = player.GetComponentsDerivedFrom<ChipAction>();

  for (auto a : actions) {
    a->EndAction();
  }
}
