#include "bnProtoManCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnProtoManSummon.h"

ProtoManCardAction::ProtoManCardAction(Character* owner, int damage) :
  damage(damage),
  CardAction(*owner, "PLAYER_IDLE"){
  this->SetLockout({ ActionLockoutType::sequence });
}

void ProtoManCardAction::Execute() {
  auto owner = GetOwner();

  owner->Hide();
  auto* proto = new ProtoManSummon(owner, damage);

  owner->GetField()->AddEntity(*proto, owner->GetTile()->GetX(), owner->GetTile()->GetY());

  CardAction::Step protoman;
  protoman.updateFunc = [proto](double elapsed, Step& step) {
    if (proto->WillRemoveLater()) {
      step.markDone();
    }
  };

  AddStep(protoman);
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
  // the animation does nothing special on end
}

void ProtoManCardAction::EndAction()
{
  GetOwner()->Reveal();
  Eject();
}
