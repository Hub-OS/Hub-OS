#include "bnAlphaClawSwipeState.h"
#include "bnAlphaGunState.h"
#include "bnAlphaCore.h"
#include "bnAlphaArm.h"

AlphaClawSwipeState::AlphaClawSwipeState() : AIState<AlphaCore>() { 
  leftArm = rightArm = nullptr;
}

AlphaClawSwipeState::~AlphaClawSwipeState() { ; }

void AlphaClawSwipeState::OnEnter(AlphaCore& a) {
  a.HideLeftArm();

  // spawn left claw
  leftArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::LEFT_SWIPE);
  Field* field = a.GetField();
  field->AddEntity(*leftArm, a.GetTarget()->GetTile()->GetX(), 1);
}

void AlphaClawSwipeState::OnUpdate(float _elapsed, AlphaCore& a) {
  // left claw finished swiping, spawn right claw
  if (leftArm && leftArm->IsDeleted()) {
    a.HideRightArm();

    rightArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::RIGHT_SWIPE);
    Field* field = a.GetField();
    field->AddEntity(*rightArm, 4, a.GetTarget()->GetTile()->GetY());

    leftArm = nullptr;
  }

  if (rightArm && rightArm->IsDeleted()) {
    this->ChangeState<AlphaGunState>();
  }
}

void AlphaClawSwipeState::OnLeave(AlphaCore& a) {
  a.RevealLeftArm();
  a.RevealRightArm();
}
