#include "bnPlayerHitState.h"
#include "bnPlayerControlledState.h"
#include "bnPlayer.h"
#include "bnAudioResourceManager.h"

PlayerHitState::PlayerHitState() : AIState<Player>()
{
}


PlayerHitState::~PlayerHitState()
{
}

void PlayerHitState::OnEnter(Player& player) {
  // When movement is interrupted because of a hit, we need to flush the action queue
  player.ClearActionQueue();
  
  // player.animationComponent->CancelCallbacks();

  player.SetAnimation(PLAYER_HIT);
  player.Audio().Play(AudioType::HURT, AudioPriority::lowest);
}

void PlayerHitState::OnUpdate(double _elapsed, Player& player) {
}

void PlayerHitState::OnLeave(Player& player) {

}