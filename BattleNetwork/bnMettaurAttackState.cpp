#include "bnMettaurAttackState.h"
#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMettaurIdleState.h"

MettaurAttackState::MettaurAttackState() : AIState<Mettaur>() { ; }
MettaurAttackState::~MettaurAttackState() { ; }

void MettaurAttackState::OnEnter(Mettaur& met) {
  auto metPtr = &met;
  auto onAttack = [this, metPtr]() {DoAttack(*metPtr); };
  auto onFinish = [this, metPtr]() { metPtr->ChangeState<MettaurIdleState>(); };

  auto& animation = *met.GetFirstComponent<AnimationComponent>();

  animation.SetAnimation("ATTACK", onFinish);

  animation.AddCallback(10, onAttack, true);
  animation.SetCounterFrameRange(6, 11);
}

void MettaurAttackState::OnUpdate(double _elapsed, Mettaur& met) {
  /* Nothing, just wait the animation out*/
}

void MettaurAttackState::OnLeave(Mettaur& met) { 
  met.EndMyTurn();
}

void MettaurAttackState::DoAttack(Mettaur& met) {
  if (met.GetField()->GetAt(met.tile->GetX() - 1, met.tile->GetY())->IsWalkable()) {
    Wave* spell = new Wave(met.field, met.team, (met.GetRank() == Mettaur::Rank::SP)? 1.2 : 1.0);

    auto props = spell->GetHitboxProperties();
    props.aggressor = &met;
    spell->SetHitboxProperties(props);

    if (met.GetRank() == Mettaur::Rank::SP) {
      spell->CrackTiles(true);
    }

    spell->SetDirection(Direction::left);
    met.field->AddEntity(*spell, met.tile->GetX() - 1, met.tile->GetY());
  }
}
