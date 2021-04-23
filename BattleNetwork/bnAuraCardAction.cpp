#include "bnAuraCardAction.h"
#include "bnCharacter.h"

AuraCardAction::AuraCardAction(Character& actor, Aura::Type type) : 
  type(type),
  CardAction(actor, "PLAYER_IDLE"){
  this->SetLockout({CardAction::LockoutType::animation,3});
}

void AuraCardAction::OnExecute(Character* user) {
  user->CreateComponent<Aura>(type, user);
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

void AuraCardAction::OnActionEnd()
{
}
