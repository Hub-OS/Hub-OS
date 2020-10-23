#include "bnRollCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnRollHeal.h"

RollCardAction::RollCardAction(Character* owner, int damage) : CardAction(owner, "PLAYER_IDLE", nullptr, ""){
  this->SetLockout(ActionLockoutProperties{
    ActionLockoutType::animation,
    3000, // milliseconds
    ActionLockoutGroup::card
  });
}

void RollCardAction::Execute() {
  auto owner = GetOwner();

  // On start of idle frame, spawn roll
  auto onSpawnRoll = [this]() -> void {
    GetOwner()->Hide();
    auto* roll = new RollHeal(GetOwner()->GetField(), GetOwner()->GetTeam(), GetOwner(), damage);

    GetOwner()->GetField()->AddEntity(*roll, GetOwner()->GetTile()->GetX(), GetOwner()->GetTile()->GetY());
  };

  onSpawnRoll();
  //AddAction(0, onSpawnRoll);
}

RollCardAction::~RollCardAction()
{
}

void RollCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void RollCardAction::OnAnimationEnd()
{
}

void RollCardAction::EndAction()
{
  GetOwner()->Reveal();
  Eject();
}
