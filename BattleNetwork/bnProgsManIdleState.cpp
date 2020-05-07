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
  progs.SetAnimation("IDLE");
}

void ProgsManIdleState::OnUpdate(float _elapsed, ProgsMan& progs) {
  cooldown -= _elapsed;

  if (cooldown < 0) {
    progs.GoToNextState();
  }
}

void ProgsManIdleState::OnLeave(ProgsMan& progs) {}