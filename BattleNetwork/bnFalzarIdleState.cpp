#include "bnFalzarIdleState.h"
#include "bnAnimationComponent.h"
#include "bnFalzar.h"

FalzarIdleState::FalzarIdleState()
{
}

FalzarIdleState::~FalzarIdleState()
{
}

void FalzarIdleState::OnEnter(Falzar& falzar)
{
  auto animation = falzar.GetFirstComponent<AnimationComponent>();
  //animation->SetAnimation("IDLE");
  //animation->SetPlaybackMode(Animator::Mode::Loop);

  this->cooldown = 300.0;
}

void FalzarIdleState::OnUpdate(double _elapsed, Falzar& falzar)
{
  cooldown -= _elapsed;

  // if cooldown for idle is over, move onto the next state
  falzar.GoToNextState();
}

void FalzarIdleState::OnLeave(Falzar&)
{
}
