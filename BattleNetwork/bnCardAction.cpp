#include "bnCardAction.h"

CardAction::CardAction(Character* owner, std::string animation, SpriteProxyNode** attachment, std::string nodeName)
  : anim(nullptr), animation(animation), nodeName(nodeName), uuid(), prevState(), attachment(attachment), UIComponent(owner)
{
  anim = owner->GetFirstComponent<AnimationComponent>();

  if (anim) {
    prepareActionDelegate = [this, owner, animation]() {
      // use the current animation's arrangement, do not overload
      prevState = anim->GetAnimationString();;
      anim->SetAnimation(animation, [this]() {
        //Logger::Log("normal callback fired");
        RecallPreviousState();

        if (this->IsLockoutOver()) {
          EndAction();
        }
      });

      anim->SetInterruptCallback([this]() {
        this->animationIsOver = true;
      });

      anim->OnUpdate(0);
      OnUpdate(0); // position to owner...

    };
  }
}

void CardAction::AddAction(int frame, std::function<void()> action)
{
  anim->AddCallback(frame, action, true);
}

void CardAction::RecallPreviousState()
{
  if (anim) {
    anim->SetAnimation(prevState);
    anim->SetPlaybackMode(Animator::Mode::Loop);
    OnAnimationEnd();
  }

  animationIsOver = true;
}

Character* CardAction::GetOwner()
{
  return GetOwnerAs<Character>();
}

void CardAction::OverrideAnimationFrames(std::list<OverrideFrame> frameData)
{
  if (anim) {
    prepareActionDelegate = [this, frameData]() {
      prevState = anim->GetAnimationString();

      anim->OverrideAnimationFrames(animation, frameData, uuid);
      anim->SetAnimation(uuid, [this]() {
        //Logger::Log("custom callback fired");

        anim->SetPlaybackMode(Animator::Mode::Loop);
        RecallPreviousState();

        if (this->IsLockoutOver()) {
          EndAction();
        }
      });
      anim->SetInterruptCallback([this]() {
        this->animationIsOver = true;
      });
      anim->OnUpdate(0);
      OnUpdate(0); // position to owner...
    };
  }
}

CardAction::~CardAction()
{
}

void CardAction::OnExecute()
{
  // prepare the animation behavior
  prepareActionDelegate();

  // run
  Execute();
}

void CardAction::OnUpdate(float _elapsed)
{
  if (!GetOwner() || !GetOwner()->GetTile()) return;

  if (attachment) {
    // update node position in the animation
    auto baseOffset = anim->GetPoint(nodeName);
    auto origin = GetOwner()->getSprite().getOrigin();
    baseOffset = baseOffset - origin;

    (*attachment)->setPosition(baseOffset);
  }

  lockoutProps.cooldown -= _elapsed;
  lockoutProps.cooldown = std::max(lockoutProps.cooldown, 0.0);
 
  if(IsLockoutOver()){
    EndAction();
  }
}

void CardAction::Inject(BattleScene&)
{
  // do nothing
}

void CardAction::SetLockout(const ActionLockoutProperties& props)
{
  this->lockoutProps = props;
}

void CardAction::SetLockoutGroup(const ActionLockoutGroup& group)
{
  lockoutProps.group = group;
}

const ActionLockoutGroup CardAction::GetLockoutGroup() const
{
  return lockoutProps.group;
}

const ActionLockoutType CardAction::GetLockoutType() const
{
  return lockoutProps.type;
}

const bool CardAction::IsAnimationOver() const
{
  return this->animationIsOver;
}

const bool CardAction::IsLockoutOver() const {
  if (lockoutProps.type == ActionLockoutType::animation)
    return animationIsOver;

  return lockoutProps.cooldown <= 0;
}
