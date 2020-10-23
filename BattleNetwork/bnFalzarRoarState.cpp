#include "bnFalzarRoarState.h"
#include "bnFalzar.h"
#include "bnAnimationComponent.h"
#include "bnAudioResourceManager.h"

FalzarRoarState::FalzarRoarState()
{
}

FalzarRoarState::~FalzarRoarState()
{
}

void FalzarRoarState::OnEnter(Falzar& falzar)
{
  AUDIO.Play(AudioType::FANFARE, AudioPriority::high);

  animateCount = 0; // reset the animate count
  auto animation = falzar.GetFirstComponent<AnimationComponent>();

  auto falzarPtr = &falzar;
  auto onFinish = [falzarPtr, this]() {
    this->animateCount++;

    if (this->animateCount == this->maxAnimateCount) {
      falzarPtr->GoToNextState();
    }
  };

  animation->SetAnimation("ROAR", onFinish);
  animation->SetPlaybackMode(Animator::Mode::Loop);
}

void FalzarRoarState::OnUpdate(float _elapsed, Falzar&)
{
}

void FalzarRoarState::OnLeave(Falzar&)
{
}
