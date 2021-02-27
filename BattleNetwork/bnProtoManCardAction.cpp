#include "bnProtoManCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnProtoManSummon.h"
#include "bnField.h"

ProtoManCardAction::ProtoManCardAction(Character& owner, int damage) :
  damage(damage),
  CardAction(owner, "PLAYER_IDLE"){
  this->SetLockout({ CardAction::LockoutType::sequence });
}

void ProtoManCardAction::OnExecute() {
  auto& owner = GetCharacter();

  owner.Hide();
  auto* proto = new ProtoManSummon(&owner, damage);

  owner.GetField()->AddEntity(*proto, *owner.GetTile());

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

void ProtoManCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void ProtoManCardAction::OnAnimationEnd()
{
  // the animation does nothing special on end
}

void ProtoManCardAction::OnEndAction()
{
  GetCharacter().Reveal();
}
