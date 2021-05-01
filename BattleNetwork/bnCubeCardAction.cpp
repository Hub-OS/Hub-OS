#include "bnCubeCardAction.h"
#include "bnCharacter.h"
#include "bnCube.h"
#include "bnField.h"

CubeCardAction::CubeCardAction(Character& actor) : 
  CardAction(actor, "PLAYER_IDLE"){
  this->SetLockout({ CardAction::LockoutType::sequence });
}

void CubeCardAction::OnExecute(Character* user) {
  auto& actor = GetActor();
  
  auto* cube = new Cube(actor.GetField());

  int step = user->GetFacing() == Direction::right ? 1 : -1;
  auto tile = actor.GetTile()->Offset(step, 0);

  if (tile) {
    actor.GetField()->AddEntity(*cube, *tile);

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

void CubeCardAction::OnActionEnd()
{
}
