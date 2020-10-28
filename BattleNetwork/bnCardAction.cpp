#include "bnCardAction.h"
#include "battlescene/bnBattleSceneBase.h"

CardAction::CardAction(Character& user, const std::string& animation)
  : 
  user(user),
  animation(animation), 
  anim(user.GetFirstComponent<AnimationComponent>()),
  uuid(), 
  prevState(), 
  attachments(), 
  SceneNode(),
  Component(&user, Component::lifetimes::battlestep)
{
  if (anim) {
    prepareActionDelegate = [this, animation]() {
      for (auto& [nodeName, node] : attachments) {
        this->GetOwner()->AddNode(&node.spriteProxy.get());
      }

      // use the current animation's arrangement, do not overload
      prevState = anim->GetAnimationString();;
      anim->SetAnimation(animation, [this]() {

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
      OnUpdate(0); // position to owner...

    };
  }
}

CardAction::~CardAction()
{
  sequence.clear();

  FreeAttachedNodes();
}

void CardAction::AddStep(Step* step)
{
  sequence.add(step);
}

void CardAction::AddAnimAction(int frame, const FrameCallback& action) {
  anim->AddCallback(frame, action);
}

sf::Vector2f CardAction::CalculatePointOffset(const std::string& point) {
  assert(this->anim && "Character must have an animation component");

  return  this->anim->GetPoint("origin") - this->anim->GetPoint(point);
}

void CardAction::RecallPreviousState()
{
  FreeAttachedNodes();

  if (anim) {
    anim->SetAnimation(prevState);
    anim->SetPlaybackMode(Animator::Mode::Loop);
    OnAnimationEnd();
  }

  animationIsOver = true;
}

Character* CardAction::GetOwner()
{
  return &user;
}

void CardAction::OverrideAnimationFrames(std::list<OverrideFrame> frameData)
{
  if (anim) {
    prepareActionDelegate = [this, frameData]() {
      for (auto& [nodeName, node] : attachments) {
        this->GetOwner()->AddNode(&node.spriteProxy.get());
      }
      prevState = anim->GetAnimationString();;
      anim->OverrideAnimationFrames(animation, frameData, uuid);
      anim->SetAnimation(uuid, [this]() {
        //Logger::Log("custom callback fired");

        anim->SetPlaybackMode(Animator::Mode::Loop);

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
      OnUpdate(0); // position to owner...
    };
  }
}

void CardAction::OnExecute()
{
  // prepare the animation behavior
  prepareActionDelegate();

  // run
  Execute();
}

CardAction::Attachment& CardAction::AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node) {
  auto iter = attachments.insert(std::make_pair(point, Attachment{ std::ref(node), std::ref(parent) }));
  return iter->second;
}

CardAction::Attachment& CardAction::AddAttachment(Character& character, const std::string& point, SpriteProxyNode& node) {
  auto animComp = character.GetFirstComponent<AnimationComponent>();
  assert(animComp && "character must have an animation component");

  return AddAttachment(animComp->GetAnimationObj(), point, node);
}

void CardAction::OnUpdate(float _elapsed)
{

  for (auto& [nodeName, node] : attachments) {
    // update the node's animation
    node.Update(_elapsed);

    // update the node's position
    auto baseOffset = node.GetParentAnim().GetPoint(nodeName);
    auto origin = user.getSprite().getOrigin();
    baseOffset = baseOffset - origin;

    node.SetOffset(baseOffset);
  }

  if (lockoutProps.type == ActionLockoutType::sequence) {
    sequence.update(_elapsed);
    if (sequence.isEmpty()) {
      EndAction();
    }
  }
  else {
    lockoutProps.cooldown -= _elapsed;
    lockoutProps.cooldown = std::max(lockoutProps.cooldown, 0.0);

    if (IsLockoutOver()) {
      EndAction();
    }
  }
}

void CardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  SceneNode::draw(target, states);
};

void CardAction::Inject(BattleSceneBase& scene)
{
  scene.Inject(this);
}

void CardAction::SetLockout(const ActionLockoutProperties& props)
{
  this->lockoutProps = props;
}

void CardAction::SetLockoutGroup(const ActionLockoutGroup& group)
{
  lockoutProps.group = group;
}

void CardAction::FreeAttachedNodes() {
  for (auto& [nodeName, node] : attachments) {
    this->GetOwner()->RemoveNode(&node.spriteProxy.get());
  }

  attachments.clear();
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
  if (lockoutProps.type == ActionLockoutType::animation) {
    return animationIsOver;
  }

  return lockoutProps.cooldown <= 0;
}

//////////////////////////////////////////////////
//                Attachment Impl               //
//////////////////////////////////////////////////

void CardAction::Attachment::PrepareAnimation(const Animation& anim)
{
  this->myAnim = anim;
}

void CardAction::Attachment::Update(double elapsed)
{
  this->myAnim.Update(elapsed, spriteProxy.get().getSprite());
}

void CardAction::Attachment::SetOffset(const sf::Vector2f& pos)
{
  this->spriteProxy.get().setPosition(pos);
}

Animation& CardAction::Attachment::GetParentAnim()
{
  return this->parentAnim;
}
