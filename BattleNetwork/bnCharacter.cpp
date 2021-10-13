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
  SetShader(smartShader);

  if (!smartShader.Get()) return;

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
  health(0),
  maxHealth(0),
  counterable(false),
  canTilePush(true),
  slideFromDrag(false),
  canShareTile(false),
  stunCooldown(0),
  rootCooldown(0),
  invincibilityCooldown(0),
  counterSlideDelta(0),
  name("unnamed"),
  rank(_rank),
  CounterHitPublisher(), 
  CardActionUsePublisher(),
  Entity() {

  if (sf::Shader* shader = Shaders().GetShader(ShaderType::BATTLE_CHARACTER)) {
    smartShader = shader;
    smartShader.SetUniform("texture", sf::Shader::CurrentTexture);
    smartShader.SetUniform("additiveMode", true);
    smartShader.SetUniform("swapPalette", false);
  }

  using namespace std::placeholders;
  auto cardHandler = std::bind(&Character::HandleCardEvent, this, _1, _2);
  actionQueue.RegisterType<CardEvent>(ActionTypes::card, cardHandler);

  auto peekHandler = std::bind(&Character::HandlePeekEvent, this, _1, _2);
  actionQueue.RegisterType<PeekCardEvent>(ActionTypes::peek_card, peekHandler);

  RegisterStatusCallback(Hit::bubble, [this] {
    actionQueue.ClearQueue(ActionQueue::CleanupType::allow_interrupts);
    CreateComponent<BubbleTrap>(this);
    
    // TODO: take out this ugly hack
    if (auto ai = dynamic_cast<Player*>(this)) {
      ai->ChangeState<BubbleState<Player>>(); 
    }
  });
}

Character::~Character() {
  // Defense items need to be manually deleted where they are created
  defenses.clear();

  for (std::shared_ptr<CardAction> action : asyncActions) {
    action->EndAction();
  }

  asyncActions.clear();
}

void Character::OnHit()
{
}

void Character::OnBattleStop()
{
  asyncActions.clear();
}

bool Character::IsStunned()
{
  return stunCooldown > 0;
}

bool Character::IsRooted()
{
    return rootCooldown > 0;
}

const Character::Rank Character::GetRank() const {
  return rank;
}

void Character::ShareTileSpace(bool enabled)
{
  canShareTile = enabled;

}

const bool Character::CanShareTileSpace() const
{
  return canShareTile;
}

void Character::EnableTilePush(bool enabled)
{
  canTilePush = enabled;
}

const bool Character::CanTilePush() const {
  return canTilePush;
}

const bool Character::IsLockoutAnimationComplete()
{
  if (currCardAction) {
    return currCardAction->IsAnimationOver();
  }

  return true;
}

void Character::Update(double _elapsed) {
  ResolveFrameBattleDamage(); 

  // reset base color
  // NOTE: that black works with additive color mode
  setColor(sf::Color(0, 0, 0, getColor().a));

  if (!hit) {
    if (invincibilityCooldown > 0) {
      unsigned frame = from_seconds(invincibilityCooldown).count() % 4;
      if (frame < 2) {
        Hide();
      }
      else {
        Reveal();
      }

      invincibilityCooldown -= _elapsed;

      if (invincibilityCooldown <= 0) {
        Reveal();
      }
    }
  }

  if(rootCooldown > 0.0) {
    rootCooldown -= _elapsed;

    if (rootCooldown <= 0.0 || invincibilityCooldown > 0 || IsPassthrough()) {
      rootCooldown = 0.0;
    }
  }

  if(stunCooldown > 0.0) {
    stunCooldown -= _elapsed;

    if (stunCooldown <= 0.0) {
      stunCooldown = 0.0;
    }
  }
  else if(stunCooldown <= 0.0) {
    this->OnUpdate(_elapsed);
  }

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
        currCardAction->Execute(this);
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

  // If the counterSlideOffset has changed from 0, it's due to the character
  // being deleted on a counter frame. Begin animating the counter-delete slide
  if (counterSlideOffset.x != 0 || counterSlideOffset.y != 0) {
    counterSlideDelta += static_cast<float>(_elapsed);
    
    auto delta = swoosh::ease::linear(counterSlideDelta, 0.10f, 1.0f);
    auto offset = delta * counterSlideOffset;

    // Add this offset onto our offsets
    setPosition(tile->getPosition().x + offset.x, tile->getPosition().y + offset.y);
  }

  if (health <= 0 || IsDeleted()) {
    // Ensure entity is deleted if health is zero
    Delete();

    // Ensure health is zero if marked for immediate deletion
    health = 0;

    // Ensure status effects do not play out
    stunCooldown = 0;
    rootCooldown = 0;
    invincibilityCooldown = 0;
  }

  // If drag status is over, reset the flag
  if (!IsSliding() && slideFromDrag) slideFromDrag = false;

  RefreshShader();

  hit = false; // reset our hit flag
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

  std::vector<SceneNode*> copies = childNodes;
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
        if (asSpriteProxyNode = dynamic_cast<SpriteProxyNode*>(currNode)) {
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
  auto occupied = [this](Entity* in) {
    Character* c = dynamic_cast<Character*>(in);

    return c && c != this && !c->CanShareTileSpace();
  };

  bool result = (Entity::CanMoveTo(next) && next->FindEntities(occupied).size() == 0);
  result = result && !next->IsEdgeTile();

  return result;
}

const bool Character::Hit(Hit::Properties props) {

  const auto original = props;

  if (GetHealth() <= 0) {
    return false;
  }

  if (IsJumping()) {
    return false;
  }

  if ((props.flags & Hit::shake) == Hit::shake) {
    CreateComponent<ShakingEffect>(this);
  }
  
  for (auto&& defense : defenses) {
    props = defense->FilterStatuses(props);
  }

  for (auto linkedCharacter : shareHit) {
    linkedCharacter->Hit(props);
  }

  // If the character itself is also super-effective,
  // double the damage independently from tile damage
  bool isSuperEffective = IsSuperEffective(props.element);

  // super effective damage is x2
  if (isSuperEffective) {
    props.damage *= 2;
  }

  SetHealth(GetHealth() - props.damage);

  if (IsTimeFrozen()) {
    props.flags |= Hit::no_counter;
  }

  // Add to status queue for state resolution
  statusQueue.push(CombatHitProps{ original, props });

  if ((props.flags & Hit::impact) == Hit::impact) {
    this->hit = true; // flash white immediately
  }

  return true;
}

void Character::RegisterStatusCallback(const Hit::Flags& flag, const StatusCallback& callback)
{
  statusCallbackHash[flag] = callback;
}

const bool Character::UnknownTeamResolveCollision(const Spell& other) const
{
  return true; // by default unknown vs unknown spells attack eachother
}

const bool Character::HasCollision(const Hit::Properties & props)
{
  if (IsJumping() && GetJumpHeight() > 10.f) return false;

  // Pierce status hits even when passthrough or flinched
  if ((props.flags & Hit::pierce) != Hit::pierce) {
    if (invincibilityCooldown > 0 || IsPassthrough()) return false;
  }

  return true;
}

int Character::GetHealth() const {
  return health;
}

const int Character::GetMaxHealth() const
{
  return maxHealth;
}

const bool Character::CanAttack() const
{
  return !currCardAction;
}

void Character::ResolveFrameBattleDamage()
{
  if(statusQueue.empty()) return;

  Character* frameCounterAggressor = nullptr;
  bool frameStunCancel = false;
  Hit::Drag postDragEffect{};

  std::queue<CombatHitProps> append;

  while (!statusQueue.empty() && !IsSliding()) {
    CombatHitProps props = statusQueue.front();
    statusQueue.pop();

    // a re-usable thunk for custom status effects
    auto flagCheckThunk = [props, this](const Hit::Flags& toCheck) {
      if ((props.filtered.flags & toCheck) == toCheck) {
        if (auto& func = statusCallbackHash[toCheck]) {
          func();
        }
      }
    };

    int tileDamage = 0;

    // Calculate elemental damage if the tile the character is on is super effective to it
    if (props.filtered.element == Element::fire
      && GetTile()->GetState() == TileState::grass) {
      tileDamage = props.filtered.damage;
      GetTile()->SetState(TileState::normal);
    }
    else if (props.filtered.element == Element::elec
      && GetTile()->GetState() == TileState::ice) {
      tileDamage = props.filtered.damage;
    }

    {
      // Only register counter if:
      // 1. Hit type is impact
      // 2. The hitbox is allowed to counter
      // 3. The character is on a counter frame
      // 4. Hit properties has an aggressor
      // This will set the counter aggressor to be the first non-impact hit and not check again this frame
      if (IsCountered() && (props.filtered.flags & Hit::impact) == Hit::impact && !frameCounterAggressor) {
        if ((props.hitbox.flags & Hit::no_counter) == 0 && props.filtered.aggressor) {
          frameCounterAggressor = field->GetCharacter(props.filtered.aggressor);
        }

        flagCheckThunk(Hit::impact);
      }

      // Requeue drag if already sliding by drag or in the middle of a move
      if ((props.filtered.flags & Hit::drag) == Hit::drag) {
        if (IsSliding()) {
          append.push({ props.hitbox, { 0, Hit::drag, Element::none, 0, props.filtered.drag } });
        }
        else {
          // requeue counter hits, if any (frameCounterAggressor is null when no counter was present)
          if (frameCounterAggressor) {
            append.push({ props.hitbox, { 0, Hit::impact, Element::none, frameCounterAggressor->GetID() } });
            frameCounterAggressor = nullptr;
          }

          // requeue drag if count is > 0
          if(props.filtered.drag.count > 0) {
            // Apply drag effect post status resolution
            postDragEffect.dir = props.filtered.drag.dir;
            postDragEffect.count = props.filtered.drag.count - 1u;
          }
        }

        flagCheckThunk(Hit::drag);

        // exclude this from the next processing step
        props.filtered.flags &= ~Hit::drag;
      }

      /**
      While an attack that only flinches will not cancel stun, 
      an attack that both flinches and flashes will cancel stun. 
      This applies if the entity doesn't have SuperArmor installed. 
      If they do have armor, stun isn't cancelled.
      
      This effect is requeued for another frame if currently dragging
      */
      if ((props.filtered.flags & Hit::stun) == Hit::stun) {
        if (postDragEffect.dir != Direction::none) {
          // requeue these statuses if in the middle of a slide
          append.push({ props.hitbox, { 0, props.filtered.flags } });
        }
        else {
          // TODO: this is a specific (and expensive) check. Is there a way to prioritize this defense rule?
          /*for (auto&& d : this->defenses) {
            hasSuperArmor = hasSuperArmor || dynamic_cast<DefenseSuperArmor*>(d);
          }*/

          // assume some defense rule strips out flinch, prevent abuse of stun
          bool cancelsStun = (props.filtered.flags & Hit::flinch) == 0 && (props.hitbox.flags & Hit::flinch) == Hit::flinch;

          if ((props.filtered.flags & Hit::flash) == Hit::flash && cancelsStun) {
            // cancel stun
            stunCooldown = 0.0;
          }
          else {
            // refresh stun
            stunCooldown = 3.0;
            flagCheckThunk(Hit::stun);
          }

          actionQueue.ClearQueue(ActionQueue::CleanupType::allow_interrupts);
        }
      }

      // exclude this from the next processing step
      props.filtered.flags &= ~Hit::stun;

      // Flash can be queued if dragging this frame
      if ((props.filtered.flags & Hit::flash) == Hit::flash) {
        if (postDragEffect.dir != Direction::none) {
          append.push({ props.hitbox, { 0, props.filtered.flags } });
        }
        else {
          invincibilityCooldown = 2.0; // used as a `flash` status time
          flagCheckThunk(Hit::flash);
        }
      }

      // exclude this from the next processing step
      props.filtered.flags &= ~Hit::flash;

      // Flinch is canceled if retangibility is applied
      if ((props.filtered.flags & Hit::retangible) == Hit::retangible) {
        invincibilityCooldown = 0.0;

        flagCheckThunk(Hit::retangible);
      }

      // exclude this from the next processing step
      props.filtered.flags &= ~Hit::retangible;

      if ((props.filtered.flags & Hit::bubble) == Hit::bubble) {
        flagCheckThunk(Hit::bubble);
      }

      // exclude this from the next processing step 
      props.filtered.flags &= ~Hit::bubble;

      if ((props.filtered.flags & Hit::root) == Hit::root) {
          rootCooldown = 2.0;
          flagCheckThunk(Hit::root);
      }

      // exclude this from the next processing step 
      props.filtered.flags &= ~Hit::root;

      /*
      flags already accounted for:
      - impact
      - stun
      - flash
      - drag
      - retangible
      - bubble
      - root

      Now check if the rest were triggered and invoke the
      corresponding status callbacks
      */
      flagCheckThunk(Hit::breaking);
      flagCheckThunk(Hit::freeze);
      flagCheckThunk(Hit::pierce);
      flagCheckThunk(Hit::shake);
      flagCheckThunk(Hit::flinch);


      if (props.filtered.damage) {
        OnHit();
        SetHealth(GetHealth() - tileDamage);
        if (GetHealth() == 0) {
          postDragEffect.dir = Direction::none; // Cancel slide post-status if blowing up
        }
        HitPublisher::Broadcast(*this, props.filtered);
      }
    }
  } // end while-loop

  if (!append.empty()) {
    statusQueue = append;
  }

  if (postDragEffect.dir != Direction::none) {
    // enemies and objects on opposing side of field are granted immunity from drag
    if (Teammate(GetTile()->GetTeam())) {
      actionQueue.ClearQueue(ActionQueue::CleanupType::allow_interrupts);
      slideFromDrag = true;
      Battle::Tile* dest = GetTile() + postDragEffect.dir;

      if (CanMoveTo(dest)) {
        // Enqueue a move action at the top of our priorities
        actionQueue.Add(MoveEvent{ frames(4), frames(0), frames(0), 0, dest, {}, true }, ActionOrder::immediate, ActionDiscardOp::until_resolve);

        // Re-queue the drag status to be re-considered in our next combat checks
        statusQueue.push({ {}, { 0, Hit::drag, Element::none, 0, postDragEffect } });
      }

      // cancel stun
      stunCooldown = 0;
    }
  }

  if (GetHealth() == 0) {
    while(statusQueue.size() > 0) {
      statusQueue.pop();
    }

    //FinishMove(); // cancels slide. TODO: obstacles do not use this but characters do!

    if(frameCounterAggressor) {
      // Slide entity back a few pixels
      counterSlideOffset = sf::Vector2f(50.f, 0.0f);
      CounterHitPublisher::Broadcast(*this, *frameCounterAggressor);
    }
  } else if (frameCounterAggressor) {
    CounterHitPublisher::Broadcast(*this, *frameCounterAggressor);
    ToggleCounter(false);
    Stun(2.5); // 150 frames @ 60 fps = 2.5 seconds
  }
}

void Character::OnUpdate(double elapsed)
{
  /* needed to be implemented for virtual inheritence */
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

void Character::SetHealth(const int _health) {
  health = _health;

  if (maxHealth == 0) {
    maxHealth = health;
  }

  if (health > maxHealth) health = maxHealth;
  if (health < 0) health = 0;
}

void Character::AdoptTile(Battle::Tile * tile)
{
  if (!IsMoving()) {
    setPosition(tile->getPosition());
  }

  tile->AddEntity(*this);
}

void Character::ToggleCounter(bool on)
{
  counterable = on;
}

void Character::Stun(double maxCooldown)
{
  stunCooldown = maxCooldown;
}

void Character::Root(double maxCooldown)
{
    rootCooldown = maxCooldown;
}

bool Character::IsCountered()
{
  return (counterable && stunCooldown <= 0);
}

void Character::SetName(std::string name)
{
  Character::name = name;
}

const std::string Character::GetName() const
{
  return name;
}

void Character::AddDefenseRule(DefenseRule * rule)
{
  if (!rule) return;

  auto iter = std::find_if(defenses.begin(), defenses.end(), [rule](DefenseRule* other) { return rule->GetPriorityLevel() == other->GetPriorityLevel(); });

  if (rule && iter == defenses.end()) {
    defenses.push_back(rule);
    std::sort(defenses.begin(), defenses.end(), [](DefenseRule* first, DefenseRule* second) { return first->GetPriorityLevel() < second->GetPriorityLevel(); });
  }
  else {
    (*iter)->replaced = true; // Flag that this defense rule may be valid ptr, but is no longer in use
    (*iter)->OnReplace();
    RemoveDefenseRule(*iter); // will invalidate the iterator

    // call again, adding new rule this time
    AddDefenseRule(rule);
  }
}

void Character::RemoveDefenseRule(DefenseRule * rule)
{
  auto iter = std::find_if(defenses.begin(), defenses.end(), [&rule](DefenseRule * in) { return in == rule; });

  if(iter != defenses.end())
    defenses.erase(iter);
}

void Character::DefenseCheck(DefenseFrameStateJudge& judge, Spell& in, const DefenseOrder& filter)
{
  std::vector<DefenseRule*> copy = defenses;

  for (int i = 0; i < copy.size(); i++) {
    if (copy[i]->GetDefenseOrder() == filter) {
      DefenseRule* defenseRule = copy[i];
      judge.SetDefenseContext(defenseRule);
      defenseRule->CanBlock(judge, in, *this);
    }
  }
}

void Character::SharedHitboxDamage(Character * to)
{
  auto iter = std::find(shareHit.begin(), shareHit.end(), to);

  if (to && iter == shareHit.end()) {
    shareHit.push_back(to);
  }
}

void Character::CancelSharedHitboxDamage(Character * to)
{
  auto iter = std::remove_if(shareHit.begin(), shareHit.end(), [&to](Character * in) { return in == to; });

  if(iter != shareHit.end())
    shareHit.erase(iter);
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
    publisher->HandlePeekEvent(this);
  }

  actionQueue.Pop();
}

Animation* Character::GetAnimationFromComponent()
{
  if (auto anim = GetFirstComponent<AnimationComponent>()) {
    return &anim->GetAnimationObject();
  }

  return nullptr;
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
