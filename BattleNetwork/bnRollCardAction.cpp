#include "bnRollCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnRollHeal.h"
#include "bnRollHeart.h"
#include "bnField.h"

RollCardAction::RollCardAction(Character& actor, int damage) :
  CardAction(actor, "PLAYER_IDLE"), damage(damage)
{
  this->SetLockout({ CardAction::LockoutType::sequence });
}

void RollCardAction::OnExecute(Character* user) {
  auto& actor = GetActor();

  // On start of idle frame, spawn roll
  actor.Hide();
  auto* roll = new RollHeal(actor.GetTeam(), &actor, damage);

  actor.GetField()->AddEntity(*roll, *actor.GetTile());

  // step 1 wait for her animation to end
  CardAction::Step step1;

  step1.updateFunc = [roll](double elapsed, Step& self) {
    if (roll->WillRemoveLater()) {
      self.markDone();
    }
  };

  AddStep(step1);

  // step 2 spawn the heart
  CardAction::Step step2;

  step2.updateFunc = [=](double elapsed, Step& self) {
    if (!heart) {
      auto actor = &GetActor();
      heart = new RollHeart(user, damage * 3);
      actor->GetField()->AddEntity(*heart, *actor->GetTile());
    }
    else if (heart->WillRemoveLater()) {
      self.markDone();
    }
  };

  AddStep(step2);

  // when steps are over, animation sequence is over and the card ends
}

RollCardAction::~RollCardAction()
{
}

void RollCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void RollCardAction::OnAnimationEnd()
{
}

void RollCardAction::OnEndAction()
{
}
