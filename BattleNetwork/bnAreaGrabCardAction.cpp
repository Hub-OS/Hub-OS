#include "bnAreaGrabCardAction.h"
#include "bnCharacter.h"
#include "bnPanelGrab.h"

AreaGrabCardAction::AreaGrabCardAction(Character* owner, int damage) : 
  damage(damage),
  CardAction(*owner, "PLAYER_IDLE"){
  this->SetLockout({ ActionLockoutType::sequence });
}

void AreaGrabCardAction::Execute() {
  auto owner = GetOwner();
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
