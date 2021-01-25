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

void ProgsManMoveState::OnUpdate(double _elapsed, ProgsMan& progs) {
  if (isMoving) return; // We're already moving (animations take time)

  nextDirection = Direction::none;

  Battle::Tile* temp = progs.GetTile();
  Battle::Tile* next = nullptr;

  int random = rand() % 50;

  // Always punch obstacles
  Battle::Tile* tile = progs.GetField()->GetAt(progs.GetTile()->GetX() - 1, progs.GetTile()->GetY());

  if (tile->ContainsEntityType<Obstacle>()) {
    return progs.GoToNextState();
  } else if (random > 15) {
    // Hunt the player
    Entity* target = progs.GetTarget();

    if (target && target->GetTile()) {
      if (target->GetTile()->GetY() == progs.GetTile()->GetY()) {
        if (target->GetTile()->GetY() < progs.GetTile()->GetY()) {
          nextDirection = Direction::up;
        }
        else if (target->GetTile()->GetY() > progs.GetTile()->GetY()) {
          nextDirection = Direction::down;
        }

        // special. if player is hanging at the middle, threaten them with a punch
        if (progs.GetField()->GetAt(target->GetTile()->GetX() + 1, target->GetTile()->GetY())->GetTeam() == progs.GetTeam()
          && progs.GetTile()->GetX() != target->GetTile()->GetX() + 1) {
          nextDirection = Direction::left;
        }
      }
    }
  }

  // otherwise aimlessly move around 
  int randDirection = rand() % 4;

  if (nextDirection == Direction::none) {
    nextDirection = static_cast<Direction>(randDirection + 1);
  }

  bool moved = progs.Move(nextDirection);

  if (moved) {
    progs.AdoptNextTile();
    auto onFinish = [&progs, this]() { progs.GoToNextState(); progs.FinishMove(); };
    progs.GetFirstComponent<AnimationComponent>()->SetAnimation("MOVING", onFinish);
    isMoving = true;
  }
  else {
    // If we can't move, go back to idle state
    progs.GoToNextState();
  }
}

void ProgsManMoveState::OnLeave(ProgsMan& progs) {
  isMoving = false;
}

