#include <SFML/Graphics.hpp>
using sf::Sprite;
using sf::IntRect;

#include "bnAnimationComponent.h"
#include "bnFileUtil.h"
#include "bnLogger.h"
#include "bnEntity.h"

AnimationComponent::AnimationComponent(Entity* _entity) {
  entity = _entity;
  speed = 1.0;
}

AnimationComponent::~AnimationComponent() {
}

void AnimationComponent::Update(float _elapsed)
{
  animation.Update(_elapsed, *entity, speed);
}

void AnimationComponent::Setup(string _path)
{
  path = _path;
}

void AnimationComponent::Reload() {
  animation = Animation(path);
  animation.Reload();
  this->Update(0);
}

const std::string AnimationComponent::GetAnimationString() const
{
  return animation.GetAnimationString();
}

void AnimationComponent::SetPlaybackSpeed(const double playbackSpeed)
{
  speed = playbackSpeed;
}

void AnimationComponent::SetAnimation(string state, std::function<void()> onFinish)
{
  animation.SetAnimation(state);
  animation << 0 << onFinish;
}

void AnimationComponent::SetAnimation(string state, char playbackMode, std::function<void()> onFinish)
{
  animation.SetAnimation(state);
  animation << playbackMode << onFinish;
}

void AnimationComponent::AddCallback(int frame, std::function<void()> onFrame, std::function<void()> outFrame, bool doOnce) {
  animation << Animate::On(frame, onFrame, doOnce) << Animate::On(frame+1, outFrame, doOnce);
}

void AnimationComponent::CancelCallbacks()
{
  animation.RemoveCallbacks();
}
