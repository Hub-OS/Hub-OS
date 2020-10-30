#include "bnRollCardAction.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnRollHeal.h"

RollCardAction::RollCardAction(Character* owner, int damage) :
  CardAction(*owner, "PLAYER_IDLE"), damage(damage)
{
  this->SetLockout(ActionLockoutProperties{ ActionLockoutType::sequence });
}

void RollCardAction::Execute() {
  auto owner = GetOwner();

  // On start of idle frame, spawn roll
  GetOwner()->Hide();
  auto* roll = new RollHeal(GetOwner()->GetField(), GetOwner()->GetTeam(), GetOwner(), damage);

  GetOwner()->GetField()->AddEntity(*roll, GetOwner()->GetTile()->GetX(), GetOwner()->GetTile()->GetY());

  // step 1 wait for her animation to end
  CardAction::Step step;

  step.updateFunc = [roll](double elapsed, Step& self) {
    if (roll->WillRemoveLater()) {
      self.markDone();
    }
  };

  AddStep(step);

  // step 2 spawn the heart

  // step 3 heal players

  // when steps are over, animation sequence is over and the card ends
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
