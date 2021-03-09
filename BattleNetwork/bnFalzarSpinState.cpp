#include "bnFalzarSpinState.h"
#include "bnFalzar.h"
#include "bnAnimationComponent.h"
#include "bnAudioResourceManager.h"

FalzarSpinState::FalzarSpinState(int spinCount) : spinCount(0), maxSpinCount(spinCount)
{
}

FalzarSpinState::~FalzarSpinState()
{
}

void FalzarSpinState::OnEnter(Falzar& falzar)
{
  spinCount = 0; // reset the spin count
  auto animation = falzar.GetFirstComponent<AnimationComponent>();
  animation->SetAnimation("SPIN");
  animation->SetPlaybackMode(Animator::Mode::Loop);
  
  // replace with drill sfx?
  falzar.Audio().Play(AudioType::INVISIBLE);
}

void FalzarSpinState::OnUpdate(double _elapsed, Falzar& falzar)
{
  
}

void FalzarSpinState::OnLeave(Falzar&)
{
}
