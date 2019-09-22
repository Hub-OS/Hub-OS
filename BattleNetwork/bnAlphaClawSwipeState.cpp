#include "bnAlphaClawSwipeState.h"
#include "bnAlphaCore.h"
#include "bnAlphaArm.h"

AlphaClawSwipeState::AlphaClawSwipeState(bool goldenArmState) : AIState<AlphaCore>(), goldenArmState(goldenArmState) { 
  leftArm = rightArm = nullptr;
  last = nullptr;
}

AlphaClawSwipeState::~AlphaClawSwipeState() { ; }

void AlphaClawSwipeState::OnEnter(AlphaCore& a) {
  a.HideRightArm();

  leftArm = rightArm = nullptr;

  if (last != a.GetTarget()->GetTile()) {
    if (a.GetTarget()->GetTile()) {
      last = a.GetTarget()->GetTile();
    }
  }

  if (!last) {
    last = a.GetField()->GetAt(1 + (rand() % 3), 1);
  }

  // spawn right claw
  rightArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::RIGHT_SWIPE);

  Field* field = a.GetField();

  if (goldenArmState) {
    rightArm->RightArmChangesTileTeam();
    field->AddEntity(*rightArm, 3, 1);
  }
  else {
    field->AddEntity(*rightArm, last->GetX(), 1);
  }


}

void AlphaClawSwipeState::OnUpdate(float _elapsed, AlphaCore& a) {
  if (last != a.GetTarget()->GetTile()) {
    if (a.GetTarget()->GetTile()) {
      last = a.GetTarget()->GetTile();
    }
  }

  // right claw finished swiping, spawn left claw
  if (rightArm && rightArm->GetIsFinished()) {
    a.HideLeftArm();

    leftArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::LEFT_SWIPE);
    Field* field = a.GetField();
    field->AddEntity(*leftArm, 4, last->GetY());

    if (goldenArmState) {
      leftArm->LeftArmChangesTileState();
    }

    rightArm->GetTile()->RemoveEntityByID(rightArm->GetID());
    rightArm->Delete();
    rightArm = nullptr;
  }

  if (leftArm && leftArm->GetIsFinished()) {
    leftArm->GetTile()->RemoveEntityByID(leftArm->GetID());
    leftArm->Delete();
    leftArm = nullptr;
    a.GoToNextState();
  }
}

void AlphaClawSwipeState::OnLeave(AlphaCore& a) {
  a.RevealLeftArm();
  a.RevealRightArm();

  leftArm = rightArm = nullptr;

}
