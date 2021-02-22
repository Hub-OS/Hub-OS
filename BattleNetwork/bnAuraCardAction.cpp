#include "bnAuraCardAction.h"
#include "bnCharacter.h"

AuraCardAction::AuraCardAction(Character& owner, Aura::Type type) : 
  type(type),
  CardAction(owner, "PLAYER_IDLE"){
  this->SetLockout({ActionLockoutType::animation,3000});
}

void AuraCardAction::OnExecute() {
  auto owner = GetOwner();

  owner->CreateComponent<Aura>(type, owner);
}

AuraCardAction::~AuraCardAction()
{
}

void AuraCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void AuraCardAction::OnAnimationEnd()
{
}

void AuraCardAction::OnEndAction()
{
  GetOwner()->Reveal();
  Eject();
}
