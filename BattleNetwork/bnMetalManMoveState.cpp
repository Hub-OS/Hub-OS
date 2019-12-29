#include "bnMetalMan.h"
#include "bnHitbox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMetalManIdleState.h"
#include "bnMetalManMoveState.h"
#include "bnMetalManPunchState.h"
#include "bnMetalManThrowState.h"
#include "bnMetalManMissileState.h"

MetalManMoveState::MetalManMoveState() : isMoving(false), AIState<MetalMan>() { ; }
MetalManMoveState::~MetalManMoveState() { ; }

void MetalManMoveState::OnEnter(MetalMan& metal) {
  isMoving = false;
}

void MetalManMoveState::OnUpdate(float _elapsed, MetalMan& metal) {
  if (isMoving || !metal.GetTarget() || !metal.GetTarget()->GetTile()) return; // We're already moving (animations take time)

  nextDirection = Direction::NONE;

  bool moved = false;

  auto oldTile = metal.GetTile();

  int tries = 50;

  do {
    // Find a new spot that is on our team
    moved = metal.Teleport((rand() % 6) + 1, (rand() % 3) + 1);
    tries--;
  } while ((!moved || metal.GetNextTile()->GetTeam() != metal.GetTeam()) && tries > 0);

  if(tries == 0) {
      oldTile->ReserveEntityByID(metal.GetID());
      moved = metal.Teleport(oldTile->GetX(), oldTile->GetY());
  }

  Battle::Tile* next = nullptr;

  if (moved) {
    metal.AdoptNextTile();

    auto onFinish = [this, m = &metal]() { 
      m->FinishMove();
        m->GoToNextState();
    };

    metal.SetAnimation(MOB_MOVING, onFinish);
    isMoving = true;
  }
  else {
  metal.GoToNextState();
  }
}

void MetalManMoveState::OnLeave(MetalMan& metal) {
  metal.GetFirstComponent<AnimationComponent>()->SetAnimation(MOB_IDLE, Animator::Mode::Loop);
}

