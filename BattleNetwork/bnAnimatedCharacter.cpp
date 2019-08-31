#include "bnAnimatedCharacter.h"
#include "bnTile.h"
#include "bnField.h"

AnimatedCharacter::AnimatedCharacter(Character::Rank _rank) : Character(_rank) {
  animationComponent= new AnimationComponent(this);
  this->RegisterComponent(animationComponent);
}

AnimatedCharacter::~AnimatedCharacter() {
}

void AnimatedCharacter::SetAnimation(string _state, std::function<void()> onFinish) {
  animationComponent->SetAnimation(_state, onFinish);
}

void AnimatedCharacter::SetCounterFrame(int frame)
{
  auto onFinish = [&]() { this->ToggleCounter(); };
  auto onNext = [&]() { this->ToggleCounter(false); };
  animationComponent->AddCallback(frame, onFinish, onNext, true);
}

void AnimatedCharacter::OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave, bool doOnce)
{
  animationComponent->AddCallback(frame, onEnter, onLeave, doOnce);
}
