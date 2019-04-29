#pragma once
#include "bnMetalMan.h"
#include "bnHitBox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMetalManIdleState.h"
#include "bnMetalManMoveState.h"
#include "bnMetalManPunchState.h"
#include "bnMetalManThrowState.h"

MetalManMoveState::MetalManMoveState() : isMoving(false), AIState<MetalMan>() { ; }
MetalManMoveState::~MetalManMoveState() { ; }

void MetalManMoveState::OnEnter(MetalMan& metal) {
}

void MetalManMoveState::OnUpdate(float _elapsed, MetalMan& metal) {
  if (isMoving || !metal.GetTarget() || !metal.GetTarget()->GetTile()) return; // We're already moving (animations take time)

  nextDirection = Direction::NONE;

  bool moved = false;
  
  int tries = 6;
  // TODO: query tiles like we do with entities
  do {
    // Find a new spot that is on our team
    moved = metal.Teleport((rand() % 6) + 1, (rand() % 3) + 1);
    tries--;
  } while ((!moved || metal.GetNextTile()->GetTeam() != metal.GetTeam()) && tries > 0);

  Battle::Tile* next = nullptr;

  if ((rand() % 30 > 23)) {
    next = metal.GetTarget()->GetTile();

    if(next && metal.Teleport(next->GetX()+1, next->GetY())) {
      metal.AdoptNextTile();
      auto onFinish = [this]() {
        // TODO: Reserve next tile to move to after punching and pass it to state
        this->ChangeState<MetalManPunchState>();
      };

      metal.SetAnimation(MOB_MOVING, onFinish);
      isMoving = true;
    }
    else {
      isMoving = false;
    }
  } else if (moved) {
    metal.AdoptNextTile();

    auto onFinish = [this, m = &metal]() { 
      if (m->GetTarget() && m->GetTarget()->GetTile()) {
        int targetY = m->GetTarget()->GetTile()->GetY();
        int targetX = m->GetTarget()->GetTile()->GetX();

        if ((targetX == 1 || targetY != 2) && (rand() % 4) == 0) {
          this->ChangeState<MetalManThrowState>();
        }
        else {
          this->ChangeState<MetalManIdleState>();
        }
      }
      else {
        this->ChangeState<MetalManIdleState>();
      }
    };

    metal.SetAnimation(MOB_MOVING, onFinish);
    isMoving = true;
  }
  else {
    this->ChangeState<MetalManIdleState>();
  }
}

void MetalManMoveState::OnLeave(MetalMan& metal) {

}

