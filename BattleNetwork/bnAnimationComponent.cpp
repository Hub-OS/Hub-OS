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
  character = this->GetOwnerAs<Character>();
}

AnimationComponent::~AnimationComponent() {
}

void AnimationComponent::OnUpdate(float _elapsed)
{
  // Since animations can be used on non-characters
  // we check if the owning entity is non-null 
  // we also check if it is a character, because stunned
  // characters do not update
  if (!GetOwner() || (character && character->IsStunned())) {
    return;
  }

  animation.Update(_elapsed, *GetOwner(), speed);
}

void AnimationComponent::SetPath(string _path)
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

const double AnimationComponent::GetPlaybackSpeed()
{
  return speed;
}

void AnimationComponent::SetAnimation(string state, std::function<void()> onFinish)
{
  // Animation changes cancel stun
  // REDACTION: Why did I write this? Changing animation shouldn't cancel stun!
  /*auto c = GetOwnerAs<Character>();
  if (c && c->IsStunned()) {
    c->stunCooldown = 0;
  }*/

  animation.SetAnimation(state);
  animation << onFinish;

  // TODO: why does this cancel callbacks?
  //animation.Refresh(*GetOwner());
}

void AnimationComponent::SetAnimation(string state, char playbackMode, std::function<void()> onFinish)
{
  animation.SetAnimation(state);
  animation << playbackMode << onFinish;

  // TODO: why does this cancel callbacks?
  //animation.Refresh(*GetOwner());
}

void AnimationComponent::SetPlaybackMode(char playbackMode)
{
	animation << playbackMode;
}

void AnimationComponent::AddCallback(int frame, std::function<void()> onFrame, std::function<void()> outFrame, bool doOnce) {
  animation << Animator::On(frame, onFrame, doOnce) << Animator::On(frame+1, outFrame, doOnce);
}

void AnimationComponent::CancelCallbacks()
{
  // We just want to cancel callbacks
  // Preserve some other state info
  auto mode = animation.GetMode();
  animation.RemoveCallbacks();
  animation << mode;
}

sf::Vector2f AnimationComponent::GetPoint(const std::string & pointName)
{
  return animation.GetPoint(pointName);
}

void AnimationComponent::OverrideAnimationFrames(const std::string& animation, std::list<OverrideFrame> data, std::string & uuid)
{
  this->animation.OverrideAnimationFrames(animation, data, uuid);

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

void AnimationComponent::SetFrame(const int index)
{
  animation.SetFrame(index, *GetOwner());
}
