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

<<<<<<< HEAD
  // printf("cooldown: %f", cooldown);

=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  if (cooldown < 0) {
    this->ChangeState<ProgsManIdleState>();
  }
}

void ProgsManHitState::OnLeave(ProgsMan& progs) {}