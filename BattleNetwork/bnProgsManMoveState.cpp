#include "bnProgsManMoveState.h"
#include "bnProgsMan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnObstacle.h"
#include "bnProgsManIdleState.h"
#include "bnProgsManPunchState.h"
#include "bnProgsManThrowState.h"
#include "bnProgsManShootState.h"

ProgsManMoveState::ProgsManMoveState() : isMoving(false), AIState<ProgsMan>() { ; }
ProgsManMoveState::~ProgsManMoveState() { ; }

void ProgsManMoveState::OnEnter(ProgsMan& progs) {
}

void ProgsManMoveState::OnUpdate(float _elapsed, ProgsMan& progs) {
  if (isMoving) return; // We're already moving (animations take time)

  nextDirection = Direction::NONE;

  Battle::Tile* temp = progs.GetTile();
  Battle::Tile* next = nullptr;

  int random = rand() % 50;

  // Always punch obstacles
  Battle::Tile* tile = progs.GetField()->GetAt(progs.GetTile()->GetX() - 1, progs.GetTile()->GetY());

  if (tile->ContainsEntityType<Obstacle>()) {
    this->ChangeState<ProgsManPunchState>();
    return;
  } else if (random > 15) {
    // Hunt the player
    Entity* target = progs.GetTarget();

    if (target && target->GetTile()) {
      if (target->GetTile()->GetY() == progs.GetTile()->GetY()) {
        if (target->GetTile()->GetX() == progs.GetTile()->GetX() - 1) {
          // Punch
          this->ChangeState<ProgsManPunchState>();
          return;
        }
        else if (rand() % 50 > 30) {
          // Throw bombs.
          this->ChangeState<ProgsManThrowState>();
          return;
        }
        else {
          // Try shooting. 
          this->ChangeState<ProgsManShootState>();
          return;
        }
      }
      else if (rand() % 50 > 20) {
        // Throw bombs.
        this->ChangeState<ProgsManThrowState>();
        return;
      }

      if (target->GetTile()->GetY() < progs.GetTile()->GetY()) {
        nextDirection = Direction::UP;
      }
      else if (target->GetTile()->GetY() > progs.GetTile()->GetY()) {
        nextDirection = Direction::DOWN;
      }

      // special. if player is hanging at the middle, threaten them with a punch
      if (progs.GetField()->GetAt(target->GetTile()->GetX() + 1, target->GetTile()->GetY())->GetTeam() == progs.GetTeam()
        && progs.GetTile()->GetX() != target->GetTile()->GetX() + 1) {
        nextDirection = Direction::LEFT;
      }
    }
  }

  // otherwise aimlessly move around 
  int randDirection = rand() % 4;

  if (nextDirection == Direction::NONE) {
    nextDirection = static_cast<Direction>(randDirection + 1);
  }

  bool moved = progs.Move(nextDirection);

  if (moved) {
    progs.AdoptNextTile();
    auto onFinish = [&]() { this->ChangeState<ProgsManIdleState>(); };
    progs.SetAnimation(MOB_MOVING, onFinish);
    isMoving = true;
  }
  else {
    // If we can't move, go back to idle state
    this->ChangeState<ProgsManIdleState>();
  }
}

void ProgsManMoveState::OnLeave(ProgsMan& progs) {

}

