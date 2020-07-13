#include "bnPlayerHitState.h"
#include "bnPlayerControlledState.h"
#include "netplay/bnPlayerNetworkState.h"
#include "bnPlayer.h"
#include "netplay/bnPlayerInputReplicator.h"
#include "netplay/bnPlayerNetworkProxy.h"
#include "bnAudioResourceManager.h"

PlayerHitState::PlayerHitState() : AIState<Player>()
{
}


PlayerHitState::~PlayerHitState()
{
}

void PlayerHitState::OnEnter(Player& player) {
  auto onFinished = [&player]() { 
    // This is so hacky and I hate it!
    // TODO: PlayerControlledState should be bare bones enough to
    //       somehow allow the swapping of controller types
    //       e.g. ChangeState<PlayerControlledState>(networkController);
    //       But for these cases where we revert states, we need to 
    //       allow the class to be smart enough to propogate that data along...
    auto replicator = player.GetFirstComponent<PlayerNetworkProxy>();
    if (replicator) {
      Logger::Logf("replicator is present");
      player.ChangeState<PlayerNetworkState>(replicator->GetNetPlayFlags());
    }
    else {
      player.ChangeState<Player::DefaultState>();
    }
  };
  player.SetAnimation(PLAYER_HIT,onFinished);
  AUDIO.Play(AudioType::HURT, AudioPriority::lowest);
}

void PlayerHitState::OnUpdate(float _elapsed, Player& player) {
}

void PlayerHitState::OnLeave(Player& player) {

}