#include "bnCardAction.h"
#include "battlescene/bnBattleSceneBase.h"

CardAction::CardAction(Character& user, const std::string& animation) : 
  user(user),
  animation(animation), 
  anim(user.GetFirstComponent<AnimationComponent>()),
  uuid(), 
  prevState(), 
  attachments(),
  ResourceHandle()
{
  if (anim) {
    prepareActionDelegate = [this] {
      for (auto& [nodeName, node] : attachments) {
        this->user.AddNode(&node.spriteProxy.get());
        node.AttachAllPendingNodes();
      }

      // use the current animation's arrangement, do not overload
      prevState = anim->GetAnimationString();
      anim->SetAnimation(this->animation, [this]() {

        if (this->IsLockoutOver()) {
          EndAction();
        }

        RecallPreviousState();
      });

      anim->SetInterruptCallback([this]() {
        this->animationIsOver = true;
        this->FreeAttachedNodes();
      });

      this->animationIsOver = false;
      anim->OnUpdate(0);
    };
  }
}

CardAction::~CardAction()
{
  stepList.clear();
  sequence.clear();

  if (started) {
    FreeAttachedNodes();
  }
}

void CardAction::AddStep(Step step)
{
  // Swooshlib needs pointers so we put these steps in a list and use that address
  auto iter = stepList.insert(stepList.begin(), new Step(step));
  sequence.add(*iter);
}

void CardAction::AddAnimAction(int frame, const FrameCallback& action) {
  anim->AddCallback(frame, action);
}

sf::Vector2f CardAction::CalculatePointOffset(const std::string& point) {
  assert(this->anim && "Character must have an animation component");

  return  this->anim->GetPoint(point) - this->anim->GetPoint("origin");
}

void CardAction::RecallPreviousState()
{
  FreeAttachedNodes();

  if (anim) {
    anim->SetAnimation(prevState);
    anim->SetPlaybackMode(Animator::Mode::Loop);
    OnAnimationEnd();
  }

  // "Animation" for sequences are only complete when the animation sequence is complete
  if (lockoutProps.type != CardAction::LockoutType::sequence) {
    animationIsOver = true;
  }
}

Character& CardAction::GetCharacter()
{
  return user;
}

void CardAction::OverrideAnimationFrames(std::list<OverrideFrame> frameData)
{
  if (anim) {
    prepareActionDelegate = [this, frameData]() {
      for (auto& [nodeName, node] : attachments) {
        this->GetCharacter().AddNode(&node.spriteProxy.get());
        node.AttachAllPendingNodes();
      }

      prevState = anim->GetAnimationString();;
      anim->OverrideAnimationFrames(animation, frameData, uuid);
      anim->SetAnimation(uuid, [this]() {
        anim->SetPlaybackMode(Animator::Mode::Loop);

        if (this->IsLockoutOver()) {
          EndAction();
        }

        RecallPreviousState();
      });

      anim->SetInterruptCallback([this]() {
        this->animationIsOver = true;
        this->FreeAttachedNodes();

        // we don't want to recall the previous state because we may be interrupted into a new state...
      });

      this->animationIsOver = false;
      anim->OnUpdate(0);
    };
  }
}

void CardAction::Execute()
{
  // prepare the animation behavior
  prepareActionDelegate();
  started = true;
  // run
  OnExecute();
  // Position any new nodes to owner
  Update(0);
}

void CardAction::EndAction()
{
  OnEndAction();
}

CardAction::Attachment& CardAction::AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node) {
  auto iter = attachments.insert(std::make_pair(point, Attachment{ std::ref(node), std::ref(parent) }));

  if (started) {
    this->GetCharacter().AddNode(&node);

    // inform any new attachments they can and should attach immediately
    iter->second.started = true;
  }

  return iter->second;
}

CardAction::Attachment& CardAction::AddAttachment(Character& character, const std::string& point, SpriteProxyNode& node) {
  auto animComp = character.GetFirstComponent<AnimationComponent>();
  assert(animComp && "character must have an animation component");

  if (started) {
    this->GetCharacter().AddNode(&node);
  }

  return AddAttachment(animComp->GetAnimationObject(), point, node);
}

void CardAction::Update(double _elapsed)
{
  for (auto& [nodeName, node] : attachments) {
    // update the node's animation
    node.Update(_elapsed);

    // update the node's position
    auto baseOffset = node.GetParentAnim().GetPoint(nodeName);
    const auto& origin = user.getSprite().getOrigin();
    baseOffset = baseOffset - origin;

    node.SetOffset(baseOffset);
  }

  if (!started) return;

  if (lockoutProps.type == CardAction::LockoutType::sequence) {
    sequence.update(_elapsed);
    if (sequence.isEmpty()) {
      animationIsOver = true; // animation for sequence is complete
      EndAction();
    }
  }
  else if(animationIsOver) /* lockoutProps.type == CardAction::LockoutType::async */ {
    lockoutProps.cooldown -= _elapsed;
    lockoutProps.cooldown = std::max(lockoutProps.cooldown, 0.0);

    if (IsLockoutOver()) {
      EndAction();
    }
  }
}

void CardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const {
};

void CardAction::PreventCounters()
{
  preventCounters = true;
}

void CardAction::SetLockout(const CardAction::LockoutProperties& props)
{
  this->lockoutProps = props;
}

void CardAction::SetLockoutGroup(const CardAction::LockoutGroup& group)
{
  lockoutProps.group = group;
}

void CardAction::FreeAttachedNodes() {
  for (auto& [nodeName, node] : attachments) {
    this->GetCharacter().RemoveNode(&node.spriteProxy.get());
  }

  attachments.clear();

  // some animation callbacks will expect to have some attachments,
  // erase the animation callbacks
  this->anim->CancelCallbacks();
}

const CardAction::LockoutGroup CardAction::GetLockoutGroup() const
{
  return lockoutProps.group;
}

const CardAction::LockoutType CardAction::GetLockoutType() const
{
  return lockoutProps.type;
}

const std::string& CardAction::GetAnimState() const
{
  return this->animation;
}

const bool CardAction::IsAnimationOver() const
{
  return IsLockoutOver();
}

const bool CardAction::IsLockoutOver() const {
  if (lockoutProps.type == CardAction::LockoutType::animation) {
    return animationIsOver;
  }

  return lockoutProps.cooldown <= 0 && animationIsOver;
}

const bool CardAction::CanExecute() const
{
  return started == false;
}

//////////////////////////////////////////////////
//                Attachment Impl               //
//////////////////////////////////////////////////

CardAction::Attachment::Attachment(SpriteProxyNode& parentNode, Animation& parentAnim) :
  spriteProxy(parentNode), parentAnim(parentAnim)
{
}

CardAction::Attachment::~Attachment()
{
}

CardAction::Attachment& CardAction::Attachment::UseAnimation(Animation& anim)
{
  this->myAnim = &anim;
  return *this;
}

void CardAction::Attachment::Update(double elapsed)
{
  if (this->myAnim) {
    this->myAnim->Update(elapsed, spriteProxy.get().getSprite());
  }

  for (auto& [nodeName, node] : attachments) {
    // update the node's animation
    node.Update(elapsed);

    // update the node's position
    auto baseOffset = node.GetParentAnim().GetPoint(nodeName);
    const auto& origin = spriteProxy.get().getSprite().getOrigin();
    baseOffset = baseOffset - origin;

    node.SetOffset(baseOffset);
  }
}

void CardAction::Attachment::SetOffset(const sf::Vector2f& pos)
{
  this->spriteProxy.get().setPosition(pos);
}

void CardAction::Attachment::AttachAllPendingNodes()
{
  for (auto& [nodeName, node] : attachments) {
    this->spriteProxy.get().AddNode(&node.spriteProxy.get());
  }

  started = true;
}

Animation& CardAction::Attachment::GetParentAnim()
{
  return this->parentAnim;
}

CardAction::Attachment& CardAction::Attachment::AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node) {
  auto iter = attachments.insert(std::make_pair(point, Attachment( std::ref(node), std::ref(parent) )));

  if (started) {
    this->spriteProxy.get().AddNode(&node);

    // inform any new attachments they can and should attach immediately
    iter->second.started = true;
  }

  return iter->second;
}