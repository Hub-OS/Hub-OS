#include "bnMettaurMoveState.h"
#include "bnMettaur.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMettaurAttackState.h"
#include "bnMettaurIdleState.h"
#include "bnAnimationComponent.h"

MettaurMoveState::MettaurMoveState() : isMoving(false), AIState<Mettaur>() { ; }
MettaurMoveState::~MettaurMoveState() { ; }

void MettaurMoveState::OnEnter(Mettaur& met) {
}

void MettaurMoveState::OnUpdate(float _elapsed, Mettaur& met) {
  if (isMoving) return; // We're already moving (animations take time)

  Battle::Tile* temp = met.tile;
  Battle::Tile* next = nullptr;

  Entity* target = met.GetTarget();

  if (target && target->GetTile()) {
    if (target->GetTile()->GetY() < met.GetTile()->GetY()) {
      nextDirection = Direction::up;
    }
    else if (target->GetTile()->GetY() > met.GetTile()->GetY()) {
      nextDirection = Direction::down;
    }
    else {
      // Try attacking if facing an available tile
      Battle::Tile* forward = met.GetField()->GetAt(temp->GetX() - 1, temp->GetY());

      if (forward && forward->IsWalkable()) {
        return met.ChangeState<MettaurAttackState>();
      }
      else {
        // Forfeit turn.
        met.ChangeState<MettaurIdleState>();
        met.EndMyTurn();
        return;
      }
    }
  }

  bool moved = met.Move(nextDirection);
  
  if (moved) {
    met.AdoptNextTile();
    auto onFinish = [this, &met]() { met.ChangeState<MettaurIdleState>(); met.FinishMove(); };

    met.GetFirstComponent<AnimationComponent>()->SetAnimation("MOVING", onFinish);
    isMoving = true;
  }
  else {
    // Cannot move or attack. Forfeit turn.
    met.ChangeState<MettaurIdleState>();
    met.EndMyTurn();
  }
}

void MettaurMoveState::OnLeave(Mettaur& met) {

}

