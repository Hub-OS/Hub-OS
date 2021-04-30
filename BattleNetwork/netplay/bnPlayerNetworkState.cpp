#include "bnPlayerNetworkState.h"

#include "../bnPlayer.h"
#include "../bnCardAction.h"
#include "../bnTile.h"
#include "../bnSelectedCardsUI.h"
#include "../bnAudioResourceManager.h"

#include <iostream>

PlayerNetworkState::PlayerNetworkState(NetPlayFlags& netflags) : 
  AIState<Player>(), 
  netflags(netflags),
  isChargeHeld(false)
{
}


PlayerNetworkState::~PlayerNetworkState()
{
}

void PlayerNetworkState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
}

void PlayerNetworkState::OnUpdate(double _elapsed, Player& player) {
  player.SetHealth(netflags.remoteHP);

  // Action controls take priority over movement
  if(!player.IsLockoutAnimationComplete()) return;

  if (!netflags.isRemoteReady) {
    netflags.remoteCharge = netflags.remoteShoot = netflags.remoteUseSpecial = false;
    netflags.remoteDirection = Direction::none;
    return;
  }

  if (netflags.remoteUseSpecial) {
    player.UseSpecial();
    netflags.remoteUseSpecial = false;
  }    // queue attack based on input behavior (buster or charge?)
  else if ((!netflags.remoteCharge && isChargeHeld) || netflags.remoteShoot == true ) {
    // This routine is responsible for determining the outcome of the attack
    player.Attack();
    netflags.remoteShoot = false;
    isChargeHeld = false;
    player.chargeEffect.SetCharging(false);
  }

  // Movement increments are restricted based on anim speed at this time
  if (player.state != PLAYER_IDLE)
    return;

  bool shouldShoot = netflags.remoteCharge && isChargeHeld == false;

#ifdef __ANDROID__
  shouldShoot = Input().Has(PRESSED_A);
#endif

  if (shouldShoot) {
    isChargeHeld = true;

    player.chargeEffect.SetCharging(true);
  }

  auto& tile = *player.GetTile();

  if (tile.GetX() != netflags.remoteTileX || tile.GetY() != netflags.remoteTileY) {
    auto onMoveBegin = [player = &player] {
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

    auto destTile = player.GetField()->GetAt(netflags.remoteTileX, netflags.remoteTileY);

    if (player.playerControllerSlide) {
      player.Slide(destTile, player.slideFrames, frames(0), ActionOrder::voluntary);
    }
    else {
      player.Teleport(destTile, ActionOrder::voluntary, onMoveBegin);
    }
  }
}

void PlayerNetworkState::OnLeave(Player& player) {
  /* Navis lose charge when we leave this state */
  player.Charge(false);
}
