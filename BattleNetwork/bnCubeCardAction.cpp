#include "bnCubeCardAction.h"
#include "bnCharacter.h"
#include "bnCube.h"
#include "bnField.h"

CubeCardAction::CubeCardAction(Character& owner) : 
  CardAction(owner, "PLAYER_IDLE"){
  this->SetLockout({ ActionLockoutType::sequence });
}

void CubeCardAction::OnExecute() {
  auto owner = GetOwner();
  
  auto* cube = new Cube(GetOwner()->GetField());

  GetOwner()->GetField()->AddEntity(*cube, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  // On start of idle frame, spawn
  AddStep({
    // update step
    [cube](double elapsed, Step& self) {
      if (cube->IsFinishedSpawning()) {
        if (cube->DidSpawnCorrectly() == false) {
          cube->Remove();
        }

        self.markDone();
      }
    }
  });
}

CubeCardAction::~CubeCardAction()
{
}

void CubeCardAction::OnUpdate(double _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void CubeCardAction::OnAnimationEnd()
{
}

void CubeCardAction::OnEndAction()
{
  Eject();
}
