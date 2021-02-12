#include <SFML/Graphics.hpp>
using sf::Sprite;
using sf::IntRect;

#include "bnAnimationComponent.h"
#include "bnFileUtil.h"
#include "bnLogger.h"
#include "bnEntity.h"
#include "bnCharacter.h"

AnimationComponent::AnimationComponent(Entity* _entity) : Component(_entity) {
  speed = 1.0;
  character = GetOwnerAs<Character>();
}

AnimationComponent::~AnimationComponent() {
}

void AnimationComponent::OnUpdate(double _elapsed)
{
  // Since animations can be used on non-characters
  // we check if the owning entity is non-null 
  // we also check if it is a character, because stunned
  // characters do not update
  if (!GetOwner() || (character && character->IsStunned() && stunnedLastFrame)) {
    return;
  }

  stunnedLastFrame = (character && character->IsStunned());
  animation.Update(_elapsed, GetOwner()->getSprite(), speed);
}

void AnimationComponent::SetPath(string _path)
{
  path = _path;
}

void AnimationComponent::Load() {
  Reload();
}

void AnimationComponent::Reload() {
  animation = Animation(path);
  animation.Reload();
}

void AnimationComponent::CopyFrom(const AnimationComponent& rhs)
{
  animation = rhs.animation;
  path = rhs.path;
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

const double AnimationComponent::GetPlaybackSpeed()
{
  return speed;
}

void AnimationComponent::SetAnimation(string state, FrameFinishCallback onFinish)
{
  animation.SetAnimation(state);

  for (auto&& o : overrideList) {
    o->SetAnimation(state);
  }

  animation << onFinish;
  animation.Refresh(GetOwner()->getSprite());
}

void AnimationComponent::SetAnimation(string state, char playbackMode, FrameFinishCallback onFinish)
{
  animation.SetAnimation(state);

  for (auto&& o : overrideList) {
    o->SetAnimation(state);
  }

  animation << playbackMode << onFinish;

  animation.Refresh(GetOwner()->getSprite());
}

void AnimationComponent::SetPlaybackMode(char playbackMode)
{
  animation << playbackMode;

  for (auto&& o : overrideList) {
    (*o) << playbackMode;
  }
}

void AnimationComponent::AddCallback(int frame, FrameCallback onFrame, bool doOnce) {
  animation << Animator::On(frame, onFrame, doOnce);
}

void AnimationComponent::SetCounterFrameRange(int frameStart, int frameEnd)
{
  if (frameStart == frameEnd) frameEnd = frameEnd + 1;
  if (frameStart > frameEnd || frameStart <= 0 || frameEnd <= 0) return;

  auto c = GetOwnerAs<Character>();
  if (c == nullptr) return;

  auto enableCounterable  = [c]() { c->ToggleCounter(); };
  auto disableCounterable = [c]() { c->ToggleCounter(false); };

  animation << Animator::On(frameStart, enableCounterable, true);
  animation << Animator::On(frameEnd,  disableCounterable, true);

}

void AnimationComponent::CancelCallbacks()
{
  // We just want to cancel callbacks
  // Preserve some other state info
  auto mode = animation.GetMode();
  animation.RemoveCallbacks();
  animation << mode;

  for (auto&& o : overrideList) {
    o->RemoveCallbacks();
    (*o) << mode;
  }
}

void AnimationComponent::OnFinish(const FrameFinishCallback& onFinish)
{
  CancelCallbacks();
  animation << onFinish;
}

sf::Vector2f AnimationComponent::GetPoint(const std::string & pointName)
{
  return animation.GetPoint(pointName);
}

Animation & AnimationComponent::GetAnimationObject()
{
  return animation;
}

void AnimationComponent::OverrideAnimationFrames(const std::string& animation, std::list<OverrideFrame> data, std::string & uuid)
{
  AnimationComponent::animation.OverrideAnimationFrames(animation, data, uuid);

  for (auto&& o : overrideList) {
    o->OverrideAnimationFrames(animation, data, uuid);
  }
}

void AnimationComponent::SyncAnimation(Animation & other)
{
  animation.SyncAnimation(other);
}

void AnimationComponent::SyncAnimation(AnimationComponent * other)
{
  animation.SyncAnimation(other->animation);
  other->OnUpdate(0);
}

void AnimationComponent::AddToOverrideList(Animation * other)
{
  if (other == &animation) return;

  auto iter = std::find(overrideList.begin(), overrideList.end(), other);

  if (iter == overrideList.end()) {
    overrideList.push_back(other);
  }
}

void AnimationComponent::RemoveFromOverrideList(Animation * other)
{
  if (other == &animation) return;

  auto iter = std::find(overrideList.begin(), overrideList.end(), other);

  if (iter != overrideList.end()) {
    overrideList.erase(iter);
  }
}

void AnimationComponent::SetInterruptCallback(const FrameFinishCallback& onInterrupt)
{
  animation.SetInterruptCallback(onInterrupt);
}

void AnimationComponent::SetFrame(const int index)
{
  animation.SetFrame(index, GetOwner()->getSprite());

  for (auto&& o : overrideList) {
    // TODO: track the override animation's target sprite?
    // o->SetFrame(index);
  }
}

void AnimationComponent::Refresh()
{
  this->OnUpdate(0);
}
