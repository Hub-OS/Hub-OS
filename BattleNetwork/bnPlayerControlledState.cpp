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
  player.SetAnimation("PLAYER_IDLE");
  replicator = player.GetFirstComponent<PlayerInputReplicator>();
}

void PlayerControlledState::OnUpdate(double _elapsed, Player& player) {
  // Actions with animation lockout controls take priority over movement
  bool canMove = player.IsLockoutAnimationComplete();

  // One of our active actions are preventing us from moving
  if (!canMove) {
    player.chargeEffect.SetCharging(false);
    isChargeHeld = false;
    return;
  }

  bool missChargeKey = isChargeHeld && !Input().Has(InputEvents::held_shoot);

  // Are we creating an action this frame?
  if (Input().Has(InputEvents::pressed_use_chip)) {
    auto cardsUI = player.GetFirstComponent<SelectedCardsUI>();
    if (cardsUI && player.CanAttack()) {
      cardsUI->UseNextCard();
      isChargeHeld = false;
    }
    // If the card used was successful, we may have a card in queue
  }
  else if (Input().Has(InputEvents::released_special)) {
    const auto actions = player.AsyncActionList();
    bool canUseSpecial = player.CanAttack();

    // Just make sure one of these actions are not from an ability
    for (const CardAction* action : actions) {
      canUseSpecial = canUseSpecial && action->GetLockoutGroup() != CardAction::LockoutGroup::ability;
    }

    if (canUseSpecial) {
      if (replicator) replicator->SendUseSpecialSignal();
      player.UseSpecial();
    }
  } // queue attack based on input behavior (buster or charge?)
  else if (Input().Has(InputEvents::released_shoot) || missChargeKey) {
    // This routine is responsible for determining the outcome of the attack
    if (replicator) {
      replicator->SendShootSignal();
      replicator->SendChargeSignal(false);
    }

    isChargeHeld = false;
    player.chargeEffect.SetCharging(false);
    player.Attack();

  } else if (Input().Has(InputEvents::held_shoot)) {
    if (replicator && !isChargeHeld) replicator->SendChargeSignal(true);
    isChargeHeld = true;
    player.chargeEffect.SetCharging(true);
  }

  // Movement increments are restricted based on anim speed at this time
  if (player.IsMoving()) {
    if (replicator) {
      auto tile = player.GetTile();
      replicator->SendTileSignal(tile->GetX(), tile->GetY());
    }
    return;
  }

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

  if(direction != Direction::none) {
    auto next_tile = player.GetTile() + direction;
    auto onMoveBegin = [player = &player, next_tile, this] {
      if (replicator) {
        replicator->SendTileSignal(next_tile->GetX(), next_tile->GetY());
      }

      auto anim = player->GetFirstComponent<AnimationComponent>();
      std::string animationStr = anim->GetAnimationString();

      const std::string& move_anim = player->GetMoveAnimHash();

      anim->CancelCallbacks();

      auto idle_callback = [anim]() {
        anim->SetAnimation("PLAYER_IDLE");
      };

      anim->SetAnimation(move_anim, [idle_callback] {
        idle_callback();
      });

      anim->SetInterruptCallback(idle_callback);
    };

    if (player.playerControllerSlide) {
      player.Slide(next_tile, player.slideFrames, frames(0), ActionOrder::voluntary);
    }
    else {
      player.Teleport(next_tile, ActionOrder::voluntary, onMoveBegin);
    }
  } 
}

void PlayerControlledState::OnLeave(Player& player) {
  /* Navis lose charge when we leave this state */
  player.Charge(false);
  replicator? replicator->SendChargeSignal(false) : (void(0));
}
