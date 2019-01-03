#pragma once
#include "bnMettaurIdleState.h"
#include "bnMettaurMoveState.h"
#include <iostream>

MettaurIdleState::MettaurIdleState() : cooldown(1), AIState<Mettaur>() { ; }
MettaurIdleState::~MettaurIdleState() { ; }

void MettaurIdleState::OnEnter(Mettaur& met) {
  met.SetAnimation(MOB_IDLE);

  if (met.GetRank() == Mettaur::Rank::SP) {
    cooldown = 0.5;
  }
}

void MettaurIdleState::OnUpdate(float _elapsed, Mettaur& met) {
  if (!met.IsMettaurTurn())
    return;

  cooldown -= _elapsed;

  if (cooldown < 0) {
    this->ChangeState<MettaurMoveState>();
  }
}

void MettaurIdleState::OnLeave(Mettaur& met) {
}

