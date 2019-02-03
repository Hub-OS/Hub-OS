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
  progs.SetAnimation(MOB_HIT);
}

void ProgsManHitState::OnUpdate(float _elapsed, ProgsMan& progs) {
  cooldown -= _elapsed;

  // printf("cooldown: %f", cooldown);

  if (cooldown < 0) {
    this->ChangeState<ProgsManIdleState>();
  }
}

void ProgsManHitState::OnLeave(ProgsMan& progs) {}