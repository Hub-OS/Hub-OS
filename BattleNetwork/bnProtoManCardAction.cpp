#include "bnProtoManCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnProtoManSummon.h"

ProtoManCardAction::ProtoManCardAction(Character* owner, int damage) :
  damage(damage),
  CardAction(*owner, "PLAYER_IDLE"){
  this->SetLockout(ActionLockoutProperties{
    ActionLockoutType::animation,
    3000, // milliseconds
    ActionLockoutGroup::card
  });
}

void ProtoManCardAction::Execute() {
  auto owner = GetOwner();

  // On start of idle frame, spawn
  auto onSpawn = [this]() -> void {
    GetOwner()->Hide();
    auto* proto = new ProtoManSummon(GetOwner(), damage);

    GetOwner()->GetField()->AddEntity(*proto, GetOwner()->GetTile()->GetX(), GetOwner()->GetTile()->GetY());
  };

  onSpawn();
}

ProtoManCardAction::~ProtoManCardAction()
{
}

void ProtoManCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void ProtoManCardAction::OnAnimationEnd()
{
}

void ProtoManCardAction::EndAction()
{
  GetOwner()->Reveal();
  Eject();
}
