#include "bnRollCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnRollHeal.h"
#include "bnRollHeart.h"
#include "bnField.h"

RollCardAction::RollCardAction(Character& owner, int damage) :
  CardAction(owner, "PLAYER_IDLE"), damage(damage)
{
  this->SetLockout(ActionLockoutProperties{ ActionLockoutType::sequence });
}

void RollCardAction::OnExecute() {
  auto owner = GetOwner();

  // On start of idle frame, spawn roll
  GetOwner()->Hide();
  auto* roll = new RollHeal(GetOwner()->GetTeam(), GetOwner(), damage);

  GetOwner()->GetField()->AddEntity(*roll, GetOwner()->GetTile()->GetX(), GetOwner()->GetTile()->GetY());

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
      heart = new RollHeart(this->GetOwner(), damage * 3);
      this->GetOwner()->GetField()->AddEntity(*heart, *this->GetOwner()->GetTile());
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
  GetOwner()->Reveal();
  Eject();
}
