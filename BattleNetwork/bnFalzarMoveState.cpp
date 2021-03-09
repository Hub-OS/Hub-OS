#include "bnFalzarMoveState.h"
#include "bnAnimationComponent.h"
#include "bnFalzar.h"

FalzarMoveState::FalzarMoveState(int moveCount) : maxMoveCount(moveCount), moveCount(0)
{
}

FalzarMoveState::~FalzarMoveState()
{
}

void FalzarMoveState::OnEnter(Falzar& falzar)
{
  auto animation = falzar.GetFirstComponent<AnimationComponent>();

  moveCount = 0; // reset the move count
  auto falzarPtr = &falzar;
  animation->SetAnimation("MOVE", [this, falzarPtr] {this->OnMoveComplete(*falzarPtr); });
}

void FalzarMoveState::OnUpdate(double _elapsed, Falzar&)
{
}

void FalzarMoveState::OnLeave(Falzar&)
{
}

void FalzarMoveState::OnMoveComplete(Falzar& falzar)
{
  int randX = (rand() % 3)+1;
  int randY = (rand() % 3)+1;

  // +3 to teleport on the blue team side of the field
  falzar.Teleport(falzar.GetField()->TileAt(randX+3, randY));
  falzar.AdoptNextTile();
  auto animation = falzar.GetFirstComponent<AnimationComponent>();

  auto falzarPtr = &falzar;
  animation->SetAnimation("IDLE", [this, falzarPtr] {this->OnFinishIdle(*falzarPtr); });
}

void FalzarMoveState::OnFinishIdle(Falzar& falzar)
{
  auto animation = falzar.GetFirstComponent<AnimationComponent>();

  this->moveCount++;

  auto falzarPtr = &falzar;

  if (this->moveCount == this->maxMoveCount) {
    falzar.GoToNextState();
  }
  else {
    animation->SetAnimation("MOVE", [this, falzarPtr] {this->OnMoveComplete(*falzarPtr); });
  }
}
