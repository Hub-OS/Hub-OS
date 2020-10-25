#include "bnCubeCardAction.h"
#include "bnCharacter.h"
#include "bnCube.h"

CubeCardAction::CubeCardAction(Character* owner) : CardAction(owner, "PLAYER_IDLE", nullptr, ""){
  this->SetLockout(ActionLockoutProperties{
    ActionLockoutType::animation,
    3000, // milliseconds
    ActionLockoutGroup::card
  });
}

void CubeCardAction::Execute() {
  auto owner = GetOwner();

  // On start of idle frame, spawn
  auto onSpawn = [this]() -> void {
    GetOwner()->Hide();
    auto* cube = new Cube(GetOwner()->GetField(), GetOwner()->GetTeam());

    GetOwner()->GetField()->AddEntity(*cube, GetOwner()->GetTile()->GetX()+1, GetOwner()->GetTile()->GetY());
  };

  onSpawn();
}

CubeCardAction::~CubeCardAction()
{
}

void CubeCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void CubeCardAction::OnAnimationEnd()
{
}

void CubeCardAction::EndAction()
{
  GetOwner()->Reveal();
  Eject();
}
