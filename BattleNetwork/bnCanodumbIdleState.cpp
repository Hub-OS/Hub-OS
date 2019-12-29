#include "bnCanodumbIdleState.h"
#include "bnCanodumbAttackState.h"
#include "bnTile.h"
#include "bnCanodumbCursor.h"

#include <iostream>

void CanodumbIdleState::Attack()
{
  can->ChangeState<CanodumbAttackState>();
}

void CanodumbIdleState::FreeCursor()
{
  if (cursor) {
    cursor->Delete();
  }

  cursor = nullptr;
}

Character::Rank CanodumbIdleState::GetCanodumbRank()
{
  return can->GetRank();
}

Entity* CanodumbIdleState::GetCanodumbTarget()
{
  return can->GetTarget();
}

CanodumbIdleState::CanodumbIdleState() : AIState<Canodumb>() { cursor = nullptr; can = nullptr; }
CanodumbIdleState::~CanodumbIdleState() { ; }

void CanodumbIdleState::OnEnter(Canodumb& can) {
  this->can = &can;
  
  switch (can.GetRank()) {
  case Canodumb::Rank::_1:
    can.SetAnimation(MOB_CANODUMB_IDLE_1);
    break;
  case Canodumb::Rank::_2:
    can.SetAnimation(MOB_CANODUMB_IDLE_2);
    break;
  case Canodumb::Rank::_3:
    can.SetAnimation(MOB_CANODUMB_IDLE_3);
    break;
  }
}

void CanodumbIdleState::OnUpdate(float _elapsed, Canodumb& can) {
  if (can.GetTarget() && can.GetTarget()->GetTile()) {
    if (can.GetTarget()->GetTile()->GetY() == can.GetTile()->GetY() && !can.GetTarget()->IsPassthrough()) {
      // Spawn tracking cursor object
      if (cursor == nullptr || cursor->IsDeleted()) {
        cursor = new CanodumbCursor(can.GetField(), can.GetTeam(), this);
        can.GetField()->AddEntity(*cursor, can.GetTile()->GetX() - 1, can.GetTile()->GetY());
      }
    }
  }
}

void CanodumbIdleState::OnLeave(Canodumb& can) {
  FreeCursor();
}

