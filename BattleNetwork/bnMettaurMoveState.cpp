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

void MettaurMoveState::OnUpdate(double _elapsed, Mettaur& met) {
  if (isMoving) return; // We're already moving (animations take time)

  Battle::Tile* temp = met.tile;
  Battle::Tile* next = nullptr;

  Entity* target = met.GetTarget();

  Direction nextDirection = Direction::none;

  if (target && target->GetTile()) {
    if (target->GetTile()->GetY() < met.GetTile()->GetY()) {
      nextDirection = Direction::up;
    }
    else if (target->GetTile()->GetY() > met.GetTile()->GetY()) {
      nextDirection = Direction::down;
    }
    else {
      // Try attacking if facing an available tile
      return met.ChangeState<MettaurAttackState>();
    }
  }
  
  if (met.Teleport(met.GetTile() + nextDirection)) {
    auto onFinish = [this, ptr = &met]() { 
      ptr->ChangeState<MettaurIdleState>(); 
    };

    auto anim = met.GetFirstComponent<AnimationComponent>();
    anim->SetAnimation("MOVING", onFinish);
    anim->SetInterruptCallback(onFinish);

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

