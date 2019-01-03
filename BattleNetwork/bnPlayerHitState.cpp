#include "bnPlayerHitState.h"
#include "bnPlayerControlledState.h"
#include "bnPlayer.h"
#include "bnAudioResourceManager.h"

PlayerHitState::PlayerHitState(float _cooldown) : cooldown(_cooldown), AIState<Player>()
{
}


PlayerHitState::~PlayerHitState()
{
}

void PlayerHitState::OnEnter(Player& player) {
  auto onFinished = [&player]() { player.ChangeState<PlayerControlledState>(); };
  player.SetAnimation(PLAYER_HIT,onFinished);
  AUDIO.Play(AudioType::HURT, AudioPriority::LOWEST);
}

void PlayerHitState::OnUpdate(float _elapsed, Player& player) {
}

void PlayerHitState::OnLeave(Player& player) {
  // Disable previous ailments... e.g. confusion, barrier, soul-cross if damage was bad enough
  player.invincibilityCooldown = 2.0f; // 2 seconds
}