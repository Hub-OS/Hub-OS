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
  delete freeCursorCallback;
  freeCursorCallback = nullptr;
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

CanodumbIdleState::~CanodumbIdleState() { 
  FreeCursor();
}

void CanodumbIdleState::OnEnter(Canodumb& can) {
  this->can = &can;
  auto animation = can.GetFirstComponent<AnimationComponent>();

  switch (can.GetRank()) {
  case Canodumb::Rank::_1:
    animation->SetAnimation("IDLE_1");
    break;
  case Canodumb::Rank::_2:
    animation->SetAnimation("IDLE_2");
    break;
  case Canodumb::Rank::_3:
    animation->SetAnimation("IDLE_3");
    break;
  }
}

void CanodumbIdleState::OnUpdate(double _elapsed, Canodumb& can) {
  if (can.GetTarget() && can.GetTarget()->GetTile()) {
    if (can.GetTarget()->GetTile()->GetY() == can.GetTile()->GetY() && !can.GetTarget()->IsPassthrough()) {
      // Spawn tracking cursor object
      if (cursor == nullptr) {
        cursor = new CanodumbCursor(this);

        delete freeCursorCallback;
        freeCursorCallback = cursor->CreateRemoveCallback();

        freeCursorCallback->Slot([this](Entity*) {
            cursor = nullptr;
        });

        can.GetField()->AddEntity(*cursor, can.GetTile()->GetX() - 1, can.GetTile()->GetY());
      }
    }
  }
}

void CanodumbIdleState::OnLeave(Canodumb& can) {
  FreeCursor();
}

