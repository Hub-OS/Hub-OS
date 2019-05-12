#include "bnMetalManMoveState.h"
#include "bnMetalMan.h"
#include "bnMissile.h"

MetalManIdleState::MetalManIdleState(float cooldown) : cooldown(cooldown), AIState<MetalMan>()
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

  if (cooldown < 0) {
      this->ChangeState<MetalManMoveState>();
  }
}

void MetalManIdleState::OnLeave(MetalMan& metal) {}