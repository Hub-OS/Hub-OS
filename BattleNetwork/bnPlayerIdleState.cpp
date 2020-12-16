#include "bnPlayerIdleState.h"
#include "bnPlayer.h"


PlayerIdleState::PlayerIdleState() : AIState<Player>()
{
}


PlayerIdleState::~PlayerIdleState()
{
}

void PlayerIdleState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
}

void PlayerIdleState::OnUpdate(double _elapsed, Player& player) {
  /* silence is golden */
}

void PlayerIdleState::OnLeave(Player& player) {}