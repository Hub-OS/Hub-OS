#include "bnAlphaClawSwipeState.h"
#include "bnAlphaGunState.h"
#include "bnAlphaCore.h"
#include "bnAlphaArm.h"

AlphaClawSwipeState::AlphaClawSwipeState() : AIState<AlphaCore>() { 
  leftArm = rightArm = nullptr;
}

AlphaClawSwipeState::~AlphaClawSwipeState() { ; }

void AlphaClawSwipeState::OnEnter(AlphaCore& a) {
  a.HideRightArm();

  // spawn right claw
  rightArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::RIGHT_SWIPE);
  Field* field = a.GetField();
  field->AddEntity(*rightArm, a.GetTarget()->GetTile()->GetX(), 1);
}

void AlphaClawSwipeState::OnUpdate(float _elapsed, AlphaCore& a) {
  // right claw finished swiping, spawn left claw
  if (rightArm && rightArm->IsDeleted()) {
    a.HideLeftArm();

    leftArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::LEFT_SWIPE);
    Field* field = a.GetField();
    field->AddEntity(*leftArm, 4, a.GetTarget()->GetTile()->GetY());

    rightArm = nullptr;
  }

  if (leftArm && leftArm->IsDeleted()) {
    this->ChangeState<AlphaGunState>();
  }
}

void AlphaClawSwipeState::OnLeave(AlphaCore& a) {
  a.RevealLeftArm();
  a.RevealRightArm();
}
