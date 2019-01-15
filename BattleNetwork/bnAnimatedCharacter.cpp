#include "bnAnimatedCharacter.h"
#include "bnTile.h"
#include "bnField.h"

AnimatedCharacter::AnimatedCharacter(Character::Rank _rank) : animationComponent(this), Character(_rank) {
}

AnimatedCharacter::~AnimatedCharacter() {
}

void AnimatedCharacter::Update(float _elapsed) {
  Entity::Update(_elapsed);
}

vector<Drawable*> AnimatedCharacter::GetMiscComponents() {
  assert(false && "GetMiscComponents shouldn't be called directly from AnimatedCharacter");
  return vector<Drawable*>();
}

void AnimatedCharacter::AddAnimation(string _state, FrameList _frameList, float _duration) {
}

void AnimatedCharacter::SetAnimation(string _state, std::function<void()> onFinish) {
  animationComponent.SetAnimation(_state, onFinish);
  animationComponent.Update(0);
}

void AnimatedCharacter::SetCounterFrame(int frame)
{
  auto onFinish = [&]() { this->ToggleCounter(); };
  auto onNext = [&]() { this->ToggleCounter(false); };
  animationComponent.AddCallback(frame, onFinish, onNext);
}

void AnimatedCharacter::OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave, bool doOnce)
{
  animationComponent.AddCallback(frame, onEnter, onLeave, doOnce);
}

const float AnimatedCharacter::GetHitHeight() const {
  //assert(false && "GetHitHeight shouldn't be called directly from Entity");
  return 0;
}

const bool AnimatedCharacter::Hit(int damage, HitProperties props) {
  //assert(false && "Hit shouldn't be called directly from Entity");
  return false;
}