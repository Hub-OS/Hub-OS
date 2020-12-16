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

  if (a.GetTarget() && last != a.GetTarget()->GetTile()) {
    if (a.GetTarget()->GetTile()) {
      last = a.GetTarget()->GetTile();
    }
  }

  if (!last) {
    last = a.GetField()->GetAt(1 + (rand() % 3), 1);
  }

  SpawnRightArm(a);
}

void AlphaClawSwipeState::OnUpdate(double _elapsed, AlphaCore& a) {
  if (a.GetTarget() && last != a.GetTarget()->GetTile()) {
    if (a.GetTarget()->GetTile()) {
      last = a.GetTarget()->GetTile();
    }
  }

  // Check if alpha dies
  if (a.IsDeleted()) {
    if (rightArm) { rightArm->Delete(); rightArm = nullptr; }
    if (leftArm)  { leftArm->Delete();  leftArm = nullptr;  }
  }

  // right claw finished swiping, spawn left claw
  if (rightArm && rightArm->GetIsFinished()) {
      SpawnLeftArm(a);
      rightArm->GetTile()->RemoveEntityByID(rightArm->GetID());
      rightArm->Delete();
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

  if (leftArm) leftArm->Delete();
  if (rightArm) rightArm->Delete();

  leftArm = rightArm = nullptr;

}

void AlphaClawSwipeState::SpawnLeftArm(AlphaCore& a) {
    a.HideLeftArm();

    leftArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::LEFT_SWIPE);

    Entity::RemoveCallback& cb = leftArm->CreateRemoveCallback();
    cb.Slot([this, alphaCore = &a]() {
        leftArm = nullptr;
        alphaCore->GoToNextState();
    });

    auto props = leftArm->GetHitboxProperties();
    props.aggressor = &a;
    leftArm->SetHitboxProperties(props);

    Field* field = a.GetField();
    field->AddEntity(*leftArm, 4, last->GetY());

    if (goldenArmState) {
        leftArm->LeftArmChangesTileState();
    }

    rightArm = nullptr;
}

void AlphaClawSwipeState::SpawnRightArm(AlphaCore& a) {

    // spawn right claw
    rightArm = new AlphaArm(nullptr, a.GetTeam(), AlphaArm::Type::RIGHT_SWIPE);

    Entity::RemoveCallback& cb = rightArm->CreateRemoveCallback();
    cb.Slot([this, alphaCore=&a]() {
        rightArm = nullptr;
        SpawnLeftArm(*alphaCore);
    });

    auto props = rightArm->GetHitboxProperties();
    props.aggressor = &a;
    rightArm->SetHitboxProperties(props);

    Field* field = a.GetField();

    if (goldenArmState) {
        rightArm->RightArmChangesTileTeam();
        field->AddEntity(*rightArm, 3, 1);
    }
    else {
        field->AddEntity(*rightArm, last->GetX(), 1);
    }
}