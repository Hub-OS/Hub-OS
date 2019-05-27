#include "bnMetalManMoveState.h"
#include "bnMetalMan.h"
<<<<<<< HEAD
#include "bnMissile.h"

MetalManIdleState::MetalManIdleState(float cooldown) : cooldown(cooldown), AIState<MetalMan>()
=======


MetalManIdleState::MetalManIdleState() : cooldown(0.8f), AIState<MetalMan>()
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
{
}


MetalManIdleState::~MetalManIdleState()
{
}

void MetalManIdleState::OnEnter(MetalMan& metal) {
  metal.animationComponent.SetAnimation(MOB_IDLE, Animate::Mode::Loop);
}

void MetalManIdleState::OnUpdate(float _elapsed, MetalMan& metal) {
  cooldown -= _elapsed;

<<<<<<< HEAD
  if (cooldown < 0) {
      this->ChangeState<MetalManMoveState>();
=======
  // printf("cooldown: %f", cooldown);

  if (cooldown < 0) {
    this->ChangeState<MetalManMoveState>();
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  }
}

void MetalManIdleState::OnLeave(MetalMan& metal) {}