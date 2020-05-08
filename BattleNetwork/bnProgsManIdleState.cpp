#include "bnProgsManIdleState.h"
#include "bnProgsManMoveState.h"
#include "bnProgsMan.h"


ProgsManIdleState::ProgsManIdleState(const float cooldown) : initialCooldown(cooldown), AIState<ProgsMan>()
{
}


ProgsManIdleState::~ProgsManIdleState()
{
}

void ProgsManIdleState::OnEnter(ProgsMan& progs) {
  progs.SetAnimation("IDLE");
  cooldown = initialCooldown;
}

void ProgsManIdleState::OnUpdate(float _elapsed, ProgsMan& progs) {
  cooldown -= _elapsed;

  if (cooldown < 0) {
    progs.GoToNextState();
  }
}

void ProgsManIdleState::OnLeave(ProgsMan& progs) {}