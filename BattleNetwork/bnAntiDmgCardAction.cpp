#include "bnAntiDmgCardAction.h"
#include "bnCharacter.h"
#include "bnNinjaAntiDamage.h"

AntiDmgCardAction::AntiDmgCardAction(Character& actor, int damage) : 
  damage(damage),
  CardAction(actor, "PLAYER_IDLE"){
  this->SetLockout(CardAction::LockoutProperties{
    CardAction::LockoutType::animation,
    3000, // milliseconds
    CardAction::LockoutGroup::card
  });
}

void AntiDmgCardAction::OnExecute(Character* user) {
  user->CreateComponent<NinjaAntiDamage>(user);
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

}
