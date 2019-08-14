#include <SFML/Graphics.hpp>
using sf::Sprite;
using sf::IntRect;

#include "bnAnimationComponent.h"
#include "bnFileUtil.h"
#include "bnLogger.h"
#include "bnEntity.h"

AnimationComponent::AnimationComponent(Entity* _entity) : Component(_entity) {
  speed = 1.0;
}

AnimationComponent::~AnimationComponent() {
}

void AnimationComponent::OnUpdate(float _elapsed)
{
  animation.Update(_elapsed, *GetOwner(), speed);
}

void AnimationComponent::Setup(string _path)
{
  path = _path;
}

void AnimationComponent::Load() {
  this->Reload();
}

void AnimationComponent::Reload() {
  animation = Animation(path);
  animation.Reload();
}

const std::string AnimationComponent::GetAnimationString() const
{
  return animation.GetAnimationString();
}

const std::string& AnimationComponent::GetFilePath() const
{
  return path;
}

void AnimationComponent::SetPlaybackSpeed(const double playbackSpeed)
{
  speed = playbackSpeed;
}

void AnimationComponent::SetAnimation(string state, std::function<void()> onFinish)
{
  animation.SetAnimation(state);
  animation << onFinish;

  // Todo: this should apply the new rects immediately on set but it
  //       removes callbacks for some reason
  // animation.Refresh(*entity);
}

void AnimationComponent::SetAnimation(string state, char playbackMode, std::function<void()> onFinish)
{
  animation.SetAnimation(state);
  animation << playbackMode << onFinish;

  // See line 49 in this file
  // animation.Refresh(*entity);
}

void AnimationComponent::SetPlaybackMode(char playbackMode)
{
	animation << playbackMode;
}

void AnimationComponent::AddCallback(int frame, std::function<void()> onFrame, std::function<void()> outFrame, bool doOnce) {
  animation << Animate::On(frame, onFrame, doOnce) << Animate::On(frame+1, outFrame, doOnce);
}

void AnimationComponent::CancelCallbacks()
{
  animation.RemoveCallbacks();
}

sf::Vector2f AnimationComponent::GetPoint(const std::string & pointName)
{
  return animation.GetPoint(pointName);
}

void AnimationComponent::SetFrame(const int index)
{
  animation.SetFrame(index, *GetOwner());
}
