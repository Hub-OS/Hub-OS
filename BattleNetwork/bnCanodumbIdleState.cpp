#pragma once
#include "bnCanodumbIdleState.h"
#include "bnCanodumbAttackState.h"
#include "bnTile.h"
#include "bnCanodumbCursor.h"

#include <iostream>

CanodumbIdleState::CanodumbIdleState() : AIState<Canodumb>() { cursor = nullptr; }
CanodumbIdleState::~CanodumbIdleState() { ; }

void CanodumbIdleState::OnEnter(Canodumb& can) {
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
  if (can.GetTarget()->GetTile()) {
    if (can.GetTarget()->GetTile()->GetY() == can.GetTile()->GetY() && !can.GetTarget()->IsPassthrough()) {
      // Spawn tracking cursor object
      if (cursor == nullptr || cursor->IsDeleted()) {
        cursor = new CanodumbCursor(can.GetField(), can.GetTeam(), &can);
        can.GetField()->AddEntity(*cursor, can.GetTile()->GetX() - 1, can.GetTile()->GetY());
      }
    }
  }
}

void CanodumbIdleState::OnLeave(Canodumb& can) {
  if (cursor) {
    cursor->Delete();
  }
}

