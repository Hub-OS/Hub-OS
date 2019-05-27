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

<<<<<<< HEAD
  // printf("cooldown: %f", cooldown);

=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  if (cooldown < 0) {
    this->ChangeState<ProgsManMoveState>();
  }
}

void ProgsManIdleState::OnLeave(ProgsMan& progs) {}