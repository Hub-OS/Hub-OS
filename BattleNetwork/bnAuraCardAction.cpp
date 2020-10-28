#include "bnAuraCardAction.h"
#include "bnCharacter.h"

AuraCardAction::AuraCardAction(Character* owner, Aura::Type type) : 
  type(type),
  CardAction(*owner, "PLAYER_IDLE"){
  this->SetLockout(ActionLockoutProperties{
    ActionLockoutType::animation,
    3000, // milliseconds
    ActionLockoutGroup::card
  });
}

void AuraCardAction::Execute() {
  auto owner = GetOwner();

  owner->CreateComponent<Aura>(type, owner);
}

AuraCardAction::~AuraCardAction()
{
}

void AuraCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void AuraCardAction::OnAnimationEnd()
{
}

void AuraCardAction::EndAction()
{
  GetOwner()->Reveal();
  Eject();
}
