#include "bnRollCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnRollHeal.h"
#include "bnRollHeart.h"
#include "bnField.h"

RollCardAction::RollCardAction(Character& owner, int damage) :
  CardAction(owner, "PLAYER_IDLE"), damage(damage)
{
  this->SetLockout({ CardAction::LockoutType::sequence });
}

void RollCardAction::OnExecute() {
  auto& owner = GetCharacter();

  // On start of idle frame, spawn roll
  owner.Hide();
  auto* roll = new RollHeal(owner.GetTeam(), &owner, damage);

  owner.GetField()->AddEntity(*roll, *owner.GetTile());

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

  step2.updateFunc = [this](double elapsed, Step& self) {
    if (!heart) {
      auto owner = &GetCharacter();
      heart = new RollHeart(owner, damage * 3);
      owner->GetField()->AddEntity(*heart, *owner->GetTile());
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
  GetCharacter().Reveal();
}
