#include "bnAreaGrabCardAction.h"
#include "bnCharacter.h"
#include "bnPanelGrab.h"

AreaGrabCardAction::AreaGrabCardAction(Character* owner, int damage) : 
  damage(damage),
  CardAction(*owner, "PLAYER_IDLE"){
  this->SetLockout(ActionLockoutProperties{
    ActionLockoutType::animation,
    3000, // milliseconds
    ActionLockoutGroup::card
  });
}

void AreaGrabCardAction::Execute() {
  auto owner = GetOwner();

  // On start of idle frame, spawn
  auto onSpawn = [this]() -> void {

  };

  onSpawn();
}

AreaGrabCardAction::~AreaGrabCardAction()
{
}

void AreaGrabCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void AreaGrabCardAction::OnAnimationEnd()
{
}

void AreaGrabCardAction::EndAction()
{
  GetOwner()->Reveal();
  Eject();
}
