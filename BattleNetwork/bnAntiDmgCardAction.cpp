#include "bnAntiDmgCardAction.h"
#include "bnCharacter.h"
#include "bnNinjaAntiDamage.h"

AntiDmgCardAction::AntiDmgCardAction(std::shared_ptr<Character> actor, int damage) : 
  damage(damage),
  CardAction(actor, "PLAYER_IDLE"){
  this->SetLockout(CardAction::LockoutProperties{
    CardAction::LockoutType::animation,
    3000, // milliseconds
    CardAction::LockoutGroup::card
  });
}

void AntiDmgCardAction::OnExecute(std::shared_ptr<Character> user) {
  user->CreateComponent<NinjaAntiDamage>(user)->Update(0);
}

AntiDmgCardAction::~AntiDmgCardAction()
{
}

void AntiDmgCardAction::OnAnimationEnd()
{
}

void AntiDmgCardAction::OnActionEnd()
{

}
