#include "bnProtoManCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnProtoManSummon.h"
#include "bnField.h"

ProtoManCardAction::ProtoManCardAction(Character& actor, int damage) :
  damage(damage),
  CardAction(actor, "PLAYER_IDLE"){
  this->SetLockout({ CardAction::LockoutType::sequence });
}

void ProtoManCardAction::OnExecute(Character* user) {
  auto& actor = GetActor();

  actor.Hide();
  auto* proto = new ProtoManSummon(&actor, damage);

  actor.GetField()->AddEntity(*proto, *actor.GetTile());

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
  GetActor().Reveal();
}
