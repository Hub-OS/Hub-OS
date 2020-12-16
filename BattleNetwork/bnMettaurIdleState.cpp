#include "bnMettaurIdleState.h"
#include "bnMettaurMoveState.h"
#include "bnAnimationComponent.h"

MettaurIdleState::MettaurIdleState() : cooldown(0.5), AIState<Mettaur>() { ; }
MettaurIdleState::~MettaurIdleState() { ; }

void MettaurIdleState::OnEnter(Mettaur& met) {
  auto& animation = *met.GetFirstComponent<AnimationComponent>();

  animation.SetAnimation("IDLE");

  if (met.GetRank() != Mettaur::Rank::_1) {
    cooldown = 0.3f;
  }
}

void MettaurIdleState::OnUpdate(double _elapsed, Mettaur& met) {
  if (!met.IsMyTurn())
    return;

  cooldown -= _elapsed;

  if (cooldown < 0) {
    met.ChangeState<MettaurMoveState>();
  }
}

void MettaurIdleState::OnLeave(Mettaur& met) {
}

