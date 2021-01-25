#include "bnProgsManHitState.h"
#include "bnProgsManIdleState.h"
#include "bnProgsMan.h"


ProgsManHitState::ProgsManHitState() : cooldown(0.5f), AIState<ProgsMan>()
{
}


ProgsManHitState::~ProgsManHitState()
{
}

void ProgsManHitState::OnEnter(ProgsMan& progs) {
  auto anim = progs.GetFirstComponent<AnimationComponent>();
  anim->SetAnimation("HIT");
}

void ProgsManHitState::OnUpdate(double _elapsed, ProgsMan& progs) {
  cooldown -= _elapsed;

  if (cooldown < 0) {
    progs.GoToNextState();
  }
}

void ProgsManHitState::OnLeave(ProgsMan& progs) {}