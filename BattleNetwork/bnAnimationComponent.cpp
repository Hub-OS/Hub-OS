#include <SFML/Graphics.hpp>
using sf::Sprite;
using sf::IntRect;

#include "bnAnimationComponent.h"
#include "bnSpriteProxyNode.h"
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

  for (auto& s : syncList) {
    animation.SyncAnimation(*s.anim);
    s.anim->Refresh(s.node->getSprite());
    RefreshSyncItem(s);
  }
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

  for (auto& s : syncList) {
    s.anim->SetAnimation(state);
    s.anim->Refresh(s.node->getSprite());
    RefreshSyncItem(s);
  }

  animation << onFinish;
  animation.Refresh(GetOwner()->getSprite());
}

void AnimationComponent::SetAnimation(string state, char playbackMode, FrameFinishCallback onFinish)
{
  animation.SetAnimation(state);

  for (auto& s : syncList) {
    s.anim->SetAnimation(state);
    s.anim->Refresh(s.node->getSprite());
    RefreshSyncItem(s);
  }

  animation << playbackMode << onFinish;

  animation.Refresh(GetOwner()->getSprite());
}

void AnimationComponent::SetPlaybackMode(char playbackMode)
{
  animation << playbackMode;

  for (auto&& o : syncList) {
    (*o.anim) << playbackMode;
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

  for (auto& o : syncList) {
    o.anim->RemoveCallbacks();
    (*o.anim) << mode;
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

  for (auto& o : syncList) {
    o.anim->OverrideAnimationFrames(animation, data, uuid);
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

void AnimationComponent::AddToSyncList(const AnimationComponent::SyncItem& item)
{
  auto iter = std::find_if(syncList.begin(), syncList.end(), [item](auto in) {
    return std::tie(in.anim, in.node, in.point) == std::tie(item.anim, item.node, item.point); 
  });

  if (iter == syncList.end()) {
    syncList.push_back(item);
  }
}

void AnimationComponent::RemoveFromSyncList(const AnimationComponent::SyncItem& item)
{
  auto iter = std::find_if(syncList.begin(), syncList.end(), [item](auto in) {
    return std::tie(in.anim, in.node, in.point) == std::tie(item.anim, item.node, item.point);
  });

  if (iter != syncList.end()) {
    syncList.erase(iter);
  }
}

void AnimationComponent::SetInterruptCallback(const FrameFinishCallback& onInterrupt)
{
  animation.SetInterruptCallback(onInterrupt);
}

void AnimationComponent::SetFrame(const int index)
{
  animation.SetFrame(index, GetOwner()->getSprite());

  for (auto& s : syncList) {
    s.anim->SetFrame(index, s.node->getSprite());
    RefreshSyncItem(s);
  }
}

void AnimationComponent::Refresh()
{
  this->OnUpdate(0);
}

void AnimationComponent::RefreshSyncItem(AnimationComponent::SyncItem& item)
{
  // update node position in the animation
  auto baseOffset = GetPoint(item.point);
  auto& origin = character->getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  item.node->setPosition(baseOffset);
}
