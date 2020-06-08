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
  auto onFinished = [&player]() { player.ChangeState<PlayerControlledState>(); };
  player.SetAnimation(PLAYER_HIT,onFinished);
  player.Audio().Play(AudioType::HURT, AudioPriority::lowest);
}

void PlayerHitState::OnUpdate(float _elapsed, Player& player) {
}

void PlayerHitState::OnLeave(Player& player) {

}