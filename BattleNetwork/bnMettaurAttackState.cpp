#pragma once
#include "bnMettaurAttackState.h"
#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMettaurIdleState.h"

MettaurAttackState::MettaurAttackState() : AIState<Mettaur>() { ; }
MettaurAttackState::~MettaurAttackState() { ; }

void MettaurAttackState::OnEnter(Mettaur& met) {
  auto onFinish = [this, &met]() {this->DoAttack(met); };
  met.SetAnimation(MOB_ATTACKING, onFinish);
  met.SetCounterFrame(4);
}

void MettaurAttackState::OnUpdate(float _elapsed, Mettaur& met) {
  /* Nothing, just wait the animation out*/
}

void MettaurAttackState::OnLeave(Mettaur& met) { 
  met.NextMettaurTurn();
}

void MettaurAttackState::DoAttack(Mettaur& met) {
  if (met.GetField()->GetAt(met.tile->GetX() - 1, met.tile->GetY())->IsWalkable()) {
    Spell* spell = new Wave(met.field, met.team, (met.GetRank() == Mettaur::Rank::SP)? 1.5 : 1.0);
    spell->SetDirection(Direction::LEFT);
    met.field->OwnEntity(spell, met.tile->GetX() - 1, met.tile->GetY());
  }

  this->ChangeState<MettaurIdleState>();
}
