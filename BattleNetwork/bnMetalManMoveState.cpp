<<<<<<< HEAD
=======
#pragma once
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#include "bnMetalMan.h"
#include "bnHitBox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMetalManIdleState.h"
#include "bnMetalManMoveState.h"
<<<<<<< HEAD
#include "bnMetalManMissileState.h"
#include "bnMetalManPunchState.h"
#include "bnMetalManThrowState.h"

MetalManMoveState::MetalManMoveState() : isMoving(false), AIState<MetalMan>() {
}

MetalManMoveState::~MetalManMoveState() { ; }

void MetalManMoveState::OnEnter(MetalMan& metal) {
    mustMove = metal.GetTile()->GetTeam() != metal.GetTeam();
=======
#include "bnMetalManPunchState.h"
#include "bnMetalManThrowState.h"

MetalManMoveState::MetalManMoveState() : isMoving(false), AIState<MetalMan>() { ; }
MetalManMoveState::~MetalManMoveState() { ; }

void MetalManMoveState::OnEnter(MetalMan& metal) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void MetalManMoveState::OnUpdate(float _elapsed, MetalMan& metal) {
  if (isMoving || !metal.GetTarget() || !metal.GetTarget()->GetTile()) return; // We're already moving (animations take time)

  nextDirection = Direction::NONE;

  bool moved = false;
<<<<<<< HEAD

  auto oldTile = metal.GetTile();

  int tries = 50;
=======
  
  int tries = 6;
  // TODO: query tiles like we do with entities
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  do {
    // Find a new spot that is on our team
    moved = metal.Teleport((rand() % 6) + 1, (rand() % 3) + 1);
    tries--;
  } while ((!moved || metal.GetNextTile()->GetTeam() != metal.GetTeam()) && tries > 0);

<<<<<<< HEAD
  if(tries == 0) {
      oldTile->ReserveEntityByID(metal.GetID());
      moved = metal.Teleport(oldTile->GetX(), oldTile->GetY());
  }

  Battle::Tile* next = nullptr;

  bool shouldPunch = false;

  if(metal.GetHealth() > 300) {
      shouldPunch = (rand() % 30 > 23);
  } else {
      shouldPunch = (rand() % 30 > 16);
  }

  if (shouldPunch && !mustMove) {
=======
  Battle::Tile* next = nullptr;

  if ((rand() % 30 > 23)) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    next = metal.GetTarget()->GetTile();

    if(next && metal.Teleport(next->GetX()+1, next->GetY())) {
      metal.AdoptNextTile();
      auto onFinish = [this]() {
<<<<<<< HEAD
=======
        // TODO: Reserve next tile to move to after punching and pass it to state
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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
<<<<<<< HEAD
            if(rand() % 20 > 15) {
                this->ChangeState<MetalManMissileState>((m->GetHealth() <= 300)? 10 : 5);
            } else {
                this->ChangeState<MetalManIdleState>();
            }
=======
          this->ChangeState<MetalManIdleState>();
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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

