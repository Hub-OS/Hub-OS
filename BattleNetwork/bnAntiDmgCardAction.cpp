#include "bnAntiDmgCardAction.h"
#include "bnCharacter.h"
#include "bnNinjaAntiDamage.h"

AntiDmgCardAction::AntiDmgCardAction(Character& owner, int damage) : 
  damage(damage),
  CardAction(owner, "PLAYER_IDLE"){
  this->SetLockout(CardAction::LockoutProperties{
    CardAction::LockoutType::animation,
    3000, // milliseconds
    CardAction::LockoutGroup::card
  });
}

void AntiDmgCardAction::OnExecute() {
  GetCharacter().CreateComponent<NinjaAntiDamage>(&GetCharacter());
}

AntiDmgCardAction::~AntiDmgCardAction()
{
}

void AntiDmgCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void AntiDmgCardAction::OnAnimationEnd()
{
}

void AntiDmgCardAction::OnEndAction()
{
  GetCharacter().Reveal();
}
