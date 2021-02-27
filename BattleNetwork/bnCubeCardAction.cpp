#include "bnCubeCardAction.h"
#include "bnCharacter.h"
#include "bnCube.h"
#include "bnField.h"

CubeCardAction::CubeCardAction(Character& owner) : 
  CardAction(owner, "PLAYER_IDLE"){
  this->SetLockout({ CardAction::LockoutType::sequence });
}

void CubeCardAction::OnExecute() {
  auto& owner = GetCharacter();
  
  auto* cube = new Cube(owner.GetField());

  auto tile = owner.GetTile()->Offset(1, 0);

  if (tile) {
    owner.GetField()->AddEntity(*cube, *tile);

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
}

CubeCardAction::~CubeCardAction()
{
}

void CubeCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void CubeCardAction::OnAnimationEnd()
{
}

void CubeCardAction::OnEndAction()
{
}
