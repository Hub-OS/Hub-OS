#include "bnAntiDmgCardAction.h"
#include "bnCharacter.h"
#include "bnDefenseAntiDamage.h"

AntiDmgCardAction::AntiDmgCardAction(Character* owner, int damage) : 
  damage(damage),
  CardAction(owner, "PLAYER_IDLE", nullptr, ""){
  this->SetLockout(ActionLockoutProperties{
    ActionLockoutType::animation,
    3000, // milliseconds
    ActionLockoutGroup::card
  });
}

void AntiDmgCardAction::Execute() {
  auto owner = GetOwner();
}

AntiDmgCardAction::~AntiDmgCardAction()
{
}

void AntiDmgCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void AntiDmgCardAction::OnAnimationEnd()
{
}

void AntiDmgCardAction::EndAction()
{
  GetOwner()->Reveal();
  Eject();
}
