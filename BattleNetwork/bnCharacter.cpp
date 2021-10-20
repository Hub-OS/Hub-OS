#include <Swoosh/Ease.h>

#include "bnCharacter.h"
#include "bnSelectedCardsUI.h"
#include "bnDefenseRule.h"
#include "bnDefenseSuperArmor.h"
#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnElementalDamage.h"
#include "bnShaderResourceManager.h"
#include "bnAnimationComponent.h"
#include "bnShakingEffect.h"
#include "bnBubbleTrap.h"
#include "bnBubbleState.h"
#include "bnPlayer.h"
#include "bnCardAction.h"
#include "bnCardToActions.h"

void Character::RefreshShader()
{
  auto field = this->field.lock();

  if (!field) {
    return;
  }

  SetShader(smartShader);

  if (!smartShader.HasShader()) return;

  // state checks
  unsigned stunFrame = from_seconds(stunCooldown).count() % 4;
  unsigned rootFrame = from_seconds(rootCooldown).count() % 4;
  counterFrameFlag = counterFrameFlag % 4;
  counterFrameFlag++;

  bool iframes = invincibilityCooldown > 0;

  vector<float> states = {
    static_cast<float>(this->hit || GetHealth() <= 0),                  // WHITEOUT
    static_cast<float>(rootCooldown > 0. && (iframes || rootCooldown)), // BLACKOUT
    static_cast<float>(stunCooldown > 0. && (iframes || stunFrame))     // HIGHLIGHT
  };

  smartShader.SetUniform("states", states);
  
  bool enabled = states[0] || states[1];

  if (enabled) return;

  if (counterable && field->DoesRevealCounterFrames() && counterFrameFlag < 2) {
    // Highlight when the character can be countered
    setColor(sf::Color(55, 55, 255, getColor().a));
  }
}

Character::Character(Rank _rank) :
  rank(_rank),
  CardActionUsePublisher(),
  Entity() {

  if (sf::Shader* shader = Shaders().GetShader(ShaderType::BATTLE_CHARACTER)) {
    smartShader = shader;
    smartShader.SetUniform("texture", sf::Shader::CurrentTexture);
    smartShader.SetUniform("additiveMode", true);
    smartShader.SetUniform("swapPalette", false);
    baseColor = sf::Color(0, 0, 0, 0);
  }

  EnableTilePush(true);

  using namespace std::placeholders;
  auto cardHandler = std::bind(&Character::HandleCardEvent, this, _1, _2);
  actionQueue.RegisterType<CardEvent>(ActionTypes::card, cardHandler);

  auto peekHandler = std::bind(&Character::HandlePeekEvent, this, _1, _2);
  actionQueue.RegisterType<PeekCardEvent>(ActionTypes::peek_card, peekHandler);

  RegisterStatusCallback(Hit::bubble, [this] {
    actionQueue.ClearQueue(ActionQueue::CleanupType::allow_interrupts);
    CreateComponent<BubbleTrap>(weak_from_this());
    
    // TODO: take out this ugly hack
    if (auto ai = dynamic_cast<Player*>(this)) {
      ai->ChangeState<BubbleState<Player>>(); 
    }
  });
}

Character::~Character() {
  for (auto& action : asyncActions) {
    action->EndAction();
  }
}

void Character::OnBattleStop()
{
  asyncActions.clear();
}

const Character::Rank Character::GetRank() const {
  return rank;
}

const bool Character::IsLockoutAnimationComplete()
{
  if (currCardAction) {
    return currCardAction->IsAnimationOver();
  }

  return true;
}

void Character::Update(double _elapsed) {
  Entity::Update(_elapsed);

  // process async actions
  std::vector<std::shared_ptr<CardAction>> asyncCopy = asyncActions;
  for (std::shared_ptr<CardAction> action : asyncCopy) {
    action->Update(_elapsed);

    if (action->IsLockoutOver()) {
      auto iter = std::find(asyncActions.begin(), asyncActions.end(), action);
      if (iter != asyncActions.end()) {
        asyncActions.erase(iter);
      }
    }
  }

  // If we have an attack from the action queue...
  if (currCardAction) {

    // if we have yet to invoke this attack...
    if (currCardAction->CanExecute() && IsActionable()) {

      // reduce the artificial delay
      cardActionStartDelay -= from_seconds(_elapsed);

      // execute when delay is over
      if (this->cardActionStartDelay <= frames(0)) {
        for(auto anim : this->GetComponents<AnimationComponent>()) {
          anim->CancelCallbacks();
        }
        MakeActionable();
        auto characterPtr = shared_from_base<Character>();
        currCardAction->Execute(characterPtr);
      }
    }

    // update will exit early if not executed (configured) 
    currCardAction->Update(_elapsed);

    // once the animation is complete, 
    // cleanup the attack and pop the action queue
    if (currCardAction->IsAnimationOver()) {
      currCardAction->EndAction();

      if (currCardAction->GetLockoutType() == CardAction::LockoutType::async) {
        asyncActions.push_back(currCardAction);
      }

      currCardAction = nullptr;
      actionQueue.Pop();
    }
  }

  RefreshShader();
}

void Character::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  // NOTE: This function does not call the parent implementation
  //       This function is a special behavior for battle characters to
  //       color their attached nodes correctly in-game

  if (!SpriteProxyNode::show) return;

  // combine the parent transform with the node's one
  sf::Transform combinedTransform = getTransform();

  states.transform *= combinedTransform;

  std::vector<SceneNode*> copies;
  copies.reserve(childNodes.size() + 1);

  for (auto& child : childNodes) {
    copies.push_back(child.get());
  }

  copies.push_back((SceneNode*)this);

  std::sort(copies.begin(), copies.end(), [](SceneNode* a, SceneNode* b) { return (a->GetLayer() > b->GetLayer()); });

  // draw its children
  for (std::size_t i = 0; i < copies.size(); i++) {
    SceneNode* currNode = copies[i];

    if (!currNode) continue;

    // If it's time to draw our scene node, we draw the proxy sprite
    if (currNode == this) {
      sf::Shader* s = smartShader.Get();

      if (s) {
        states.shader = s;
      }

      target.draw(getSpriteConst(), states);
    }
    else {
      SpriteProxyNode* asSpriteProxyNode{ nullptr };
      SmartShader temp(smartShader);
      sf::Color tempColor = sf::Color::White;
      if (currNode->HasTag(Player::FORM_NODE_TAG)) {
        asSpriteProxyNode = dynamic_cast<SpriteProxyNode*>(currNode);

        if (asSpriteProxyNode) {
          smartShader.SetUniform("swapPalette", false);
          tempColor = asSpriteProxyNode->getColor();
          asSpriteProxyNode->setColor(sf::Color(0, 0, 0, getColor().a));
        }
      }

      // Apply and return shader if applicable
      sf::Shader* s = smartShader.Get();

      if (s) {
        states.shader = s;
      }

      currNode->draw(target, states);

      // revert color
      if (asSpriteProxyNode) {
        asSpriteProxyNode->setColor(tempColor);
      }

      // revert uniforms from this pass
      smartShader = temp;
    }
  }
}

bool Character::CanMoveTo(Battle::Tile * next)
{
  auto occupied = [this](std::shared_ptr<Entity> in) {
    auto c = dynamic_cast<Character*>(in.get());

    return c && c != this && !c->CanShareTileSpace();
  };

  bool result = (Entity::CanMoveTo(next) && next->FindEntities(occupied).size() == 0);
  result = result && !next->IsEdgeTile();

  return result;
}

const bool Character::CanAttack() const
{
  return !currCardAction;
}

void Character::MakeActionable()
{
  // impl. defined
}

bool Character::IsActionable() const
{
  return true; // impl. defined
}

const std::vector<std::shared_ptr<CardAction>> Character::AsyncActionList() const
{
  return asyncActions;
}

std::shared_ptr<CardAction> Character::CurrentCardAction()
{
  return currCardAction;
}

void Character::SetName(std::string name)
{
  Character::name = name;
}

const std::string Character::GetName() const
{
  return name;
}

void Character::AddAction(const CardEvent& event, const ActionOrder& order)
{
  actionQueue.Add(event, order, ActionDiscardOp::until_resolve);
}

void Character::AddAction(const PeekCardEvent& event, const ActionOrder& order)
{
  actionQueue.Add(event, order, ActionDiscardOp::until_eof);
}

void Character::HandleCardEvent(const CardEvent& event, const ActionQueue::ExecutionType& exec)
{
  if (currCardAction == nullptr) {
    if (event.action->GetMetaData().timeFreeze) {
      CardActionUsePublisher::Broadcast(event.action, CurrentTime::AsMilli());
      actionQueue.Pop();
    }
    else {
      currCardAction = event.action;
    }
  }

  if (exec == ActionQueue::ExecutionType::interrupt) {
    currCardAction->EndAction();
    currCardAction = nullptr;
  }
}

void Character::HandlePeekEvent(const PeekCardEvent& event, const ActionQueue::ExecutionType& exec)
{
  SelectedCardsUI* publisher = event.publisher;
  if (publisher) {
    auto characterPtr = shared_from_base<Character>();
    publisher->HandlePeekEvent(characterPtr);
  }

  actionQueue.Pop();
}

void Character::SetPalette(const std::shared_ptr<sf::Texture>& palette)
{
  if (palette.get() == nullptr) {
    smartShader.SetUniform("swapPalette", false);
    return;
  }

  this->palette = palette;
  smartShader.SetUniform("swapPalette", true);
  smartShader.SetUniform("palette", *this->palette.get());
}

std::shared_ptr<sf::Texture> Character::GetPalette()
{
  return palette;
}

void Character::StoreBasePalette(const std::shared_ptr<sf::Texture>& palette)
{
  basePalette = palette;
}

std::shared_ptr<sf::Texture> Character::GetBasePalette()
{
  return basePalette;
}
