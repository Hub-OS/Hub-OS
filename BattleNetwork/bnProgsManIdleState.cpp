#include "bnProgsManIdleState.h"
#include "bnProgsManMoveState.h"
#include "bnProgsMan.h"


ProgsManIdleState::ProgsManIdleState() : cooldown(0.5f), AIState<ProgsMan>()
{
}


ProgsManIdleState::~ProgsManIdleState()
{
}

void ProgsManIdleState::OnEnter(ProgsMan& progs) {
  progs.SetAnimation(MOB_IDLE);
}

void ProgsManIdleState::OnUpdate(float _elapsed, ProgsMan& progs) {
  cooldown -= _elapsed;

  // printf("cooldown: %f", cooldown);

  if (cooldown < 0) {
    progs.ChangeState<ProgsManMoveState>();
  }
}

void ProgsManIdleState::OnLeave(ProgsMan& progs) {}