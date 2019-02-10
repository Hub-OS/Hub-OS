#pragma once
#include "bnMetalMan.h"
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
  if (isMoving) return; // We're already moving (animations take time)

  nextDirection = Direction::NONE;

  bool moved = metal.Teleport((rand()%3) + 4, (rand()%3)+1);

  Battle::Tile* next = nullptr;

  if ((rand() % 30 > 23)) {
    next = metal.GetTarget()->GetTile();

    if(metal.Teleport(next->GetX()+1, next->GetY())) {
      metal.AdoptNextTile();

      auto onFinish = [this, &metal]() {
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

    auto onFinish = [this, &metal]() { 
      int targetY = metal.GetTarget()->GetTile()->GetY();
      int targetX = metal.GetTarget()->GetTile()->GetX();

      if ((targetX == 1 || targetY != 2) && (rand()%4) == 0) {
        this->ChangeState<MetalManThrowState>();
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

