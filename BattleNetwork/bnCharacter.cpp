#include "bnCharacter.h"
#include "bnDefenseRule.h"
#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnElementalDamage.h"
#include "bnShaderResourceManager.h"
#include "bnAnimationComponent.h"
#include "bnShakingEffect.h"
#include "bnCardAction.h"

#include <Swoosh/Ease.h>

Character::Character(Rank _rank) :
  health(0),
  maxHealth(0),
  counterable(false),
  canTilePush(true),
  slideFromDrag(false),
  canShareTile(false),
  stunCooldown(0),
  invincibilityCooldown(0),
  counterSlideDelta(0),
  name("unnamed"),
  rank(_rank),
  hit(false),
  CounterHitPublisher(), Entity() {

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);
}

Character::~Character() {
  // Defense items need to be manually deleted where they are created
  defenses.clear();
}

bool Character::IsStunned()
{
  return stunCooldown > 0;
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

void Character::Update(float _elapsed) {
  ResolveFrameBattleDamage();

  // Prevent animations from updating and AI from moving around
  // If during time freeze battle states
  if (IsTimeFrozen()) return;

  setColor(sf::Color(255, 255, 255, getColor().a));

  sf::Vector2f shakeOffset;

  double prevThisFrameStun = stunCooldown;

  if (!hit) {
      if (stunCooldown && (((int)(stunCooldown * 15))) % 2 == 0) {
          SetShader(stun);
      }
      else if(GetHealth() > 0) {
          SetShader(nullptr);

          if (counterable) {
            setColor(sf::Color(255, 55, 55, getColor().a));
            SetShader(SHADERS.GetShader(ShaderType::ADDITIVE));
          }
      }

      if (invincibilityCooldown > 0) {
          // This just blinks every 15 ms
          if ((((int)(invincibilityCooldown * 15))) % 2 == 0) {
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
  else {
      SetShader(whiteout);
  }

  if (prevThisFrameStun <= 0.0) {
    // HACKY: If we are stunned this frame, let AI update step once
    // to turn into their respective hit state animations
    // TODO at some sort of hooks instead for Characters
    OnUpdate(_elapsed);
  } else if (stunCooldown > 0.0) {
    stunCooldown -= _elapsed;

    if (stunCooldown <= 0.0) {
      stunCooldown = 0.0;
    }
  }

  Entity::Update(_elapsed);

  if (queuedAction && currentAction == nullptr && !GetNextTile()) {
    currentAction = queuedAction;
    queuedAction = nullptr;
    currentAction->OnExecute();
  }

  if (currentAction) {
    currentAction->OnUpdate(_elapsed);
  }

  // If the counterSlideOffset has changed from 0, it's due to the character
  // being deleted on a counter frame. Begin animating the counter-delete slide
  if (counterSlideOffset.x != 0 || counterSlideOffset.y != 0) {
    counterSlideDelta += _elapsed;
    
    auto delta = swoosh::ease::linear(counterSlideDelta, 0.10f, 1.0f);
    auto offset = delta * counterSlideOffset;

    // Add this offset onto our offsets
    setPosition(tile->getPosition().x + offset.x, tile->getPosition().y + offset.y);
  }

  setPosition(getPosition() + shakeOffset);

  hit = false;

  if (health <= 0 || IsDeleted()) {
      health = 0;
  }

  if (health == 0) {
      Delete();
  }

  // If drag status is over, reset the flag
  if (!IsSliding() && slideFromDrag) slideFromDrag = false;
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

  if (GetHealth() <= 0) return false;

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
  bool isSuperEffective = IsSuperEffective(props.element) && (props.flags & Hit::impact) != Hit::impact;

  // Show ! super effective symbol on the field
  if (isSuperEffective) {
    props.damage *= 2;

    Artifact *seSymbol = new ElementalDamage(field);
    seSymbol->SetLayer(-100);
    field->AddEntity(*seSymbol, tile->GetX(), tile->GetY());
  }

  SetHealth(GetHealth() - props.damage);

  // Add to status queue for state resolution
  statusQueue.push(props);

  return true;
}

const bool Character::HasCollision(const Hit::Properties & props)
{
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

void Character::ResolveFrameBattleDamage()
{
  if(statusQueue.empty()) return;

  Character* frameCounterAggressor = nullptr;
  bool frameStunCancel = false;
  Direction postDragDir = Direction::none;

  std::queue<Hit::Properties> append;

  while(!statusQueue.empty() && !IsSliding()) {
    Hit::Properties& props = statusQueue.front();
    statusQueue.pop();

    int tileDamage = 0;

    // Calculate elemental damage if the tile the character is on is super effective to it
    if (props.element == Element::fire
      && GetTile()->GetState() == TileState::grass
      && !(HasAirShoe() || HasFloatShoe())) {
      tileDamage = props.damage;
      GetTile()->SetState(TileState::normal);
    }
    else if (props.element == Element::elec
      && GetTile()->GetState() == TileState::ice
      && !(HasAirShoe() || HasFloatShoe())) {
      tileDamage = props.damage;
      GetTile()->SetState(TileState::normal);
    }

    // Pass on hit properties to the user-defined handler
    if (OnHit(props)) {
      // Only register counter if:
      // 1. Hit type is impact
      // 2. The character is on a counter frame
      // 3. Hit properties has an aggressor
      // This will set the counter aggressor to be the first non-impact hit and not check again this frame
      if (IsCountered() && (props.flags & Hit::impact) == Hit::impact && !frameCounterAggressor) {
        if (props.aggressor) {
          frameCounterAggressor = props.aggressor;
        }
      }

      // Requeue drag if already sliding by drag or in the middle of a move
      if ((props.flags & Hit::drag) == Hit::drag) {
        if (slideFromDrag || GetNextTile()) {
          append.push({ 0, Hit::drag, Element::none, nullptr, props.drag });
        }
        else {
          // Apply directional slide in a moment
          postDragDir = props.drag;

          // requeue counter hits
          append.push({ 0, Hit::impact, Element::none, frameCounterAggressor, Direction::none });
          frameCounterAggressor = nullptr;
        }

        // exclude this from the next processing step
        props.drag = Direction::none;
        props.flags &= ~Hit::drag;
      }

      bool hadStun = false;

      // Stun can be canceled by non-stun hits or queued if dragging
      if ((props.flags & Hit::stun) == Hit::stun) {
        if (postDragDir != Direction::none) {
          // requeue these statuses if in the middle of a slide
          append.push({ 0, props.flags, Element::none, nullptr, Direction::none });
        }
        else {
          // refresh stun
          stunCooldown = 3.0;
          hadStun = true;
        }
      }

      // exclude this from the next processing step
      props.flags &= ~Hit::stun;

      // Flinch is ignored if already flinching or stunned (super armor equivalent)
      // and can be queued if dragging this frame
      if ((props.flags & Hit::flinch) == Hit::flinch && !hadStun) {
        if (postDragDir != Direction::none) {
          append.push({ 0, props.flags, Element::none, nullptr, Direction::none });
        }
        else {
          if (invincibilityCooldown <= 0.0) {
            invincibilityCooldown = 3.0;
          }
        }
      }

      // exclude this from the next processing step
      props.flags &= ~Hit::flinch;

      // Flinch is canceled if retangibility is applied
      if ((props.flags & Hit::retangible) == Hit::retangible) {
        invincibilityCooldown = 0.0;
      }

      // exclude this from the next processing step
      props.flags &= ~Hit::retangible;

      hit = hit || props.damage;
    }

    if (hit) {
      SetHealth(GetHealth() - tileDamage);

      if (GetHealth() == 0) {
        postDragDir = Direction::none; // Cancel slide post-status if blowing up
      }
    }
  }

  if (!append.empty()) {
    statusQueue = append;
  }

  if (postDragDir != Direction::none) {
    // enemies and objects on opposing side of field are granted immunity from drag
    if (Teammate(GetTile()->GetTeam())) {
      SlideToTile(true);
      slideFromDrag = true;
      Move(postDragDir);

      // cancel stun
      stunCooldown = 0;
    }
  }

  if (frameCounterAggressor) {
    Broadcast(*this, *frameCounterAggressor);
    ToggleCounter(false);
    Stun(2.5); // 150 frames @ 60 fps = 2.5 seconds
  }

  if (GetHealth() == 0) {

    while(statusQueue.size() > 0) {
      statusQueue.pop();
    }

    stunCooldown = 0;
    invincibilityCooldown = 0;

    SlideToTile(false); // cancel slide

    if(frameCounterAggressor) {
      // Slide entity back a few pixels
      counterSlideOffset = sf::Vector2f(50.f, 0.0f);
    }
  }
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
  tile->AddEntity(*this);

  if (!IsSliding()) {
    setPosition(tile->getPosition());
  }
}

void Character::ToggleCounter(bool on)
{
  counterable = on;
}

void Character::Stun(double maxCooldown)
{
  stunCooldown = maxCooldown;
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
    RemoveDefenseRule(*iter);

    // call again, adding this time
    AddDefenseRule(rule);
  }
}

void Character::RemoveDefenseRule(DefenseRule * rule)
{
  auto iter = std::remove_if(defenses.begin(), defenses.end(), [&rule](DefenseRule * in) { return in == rule; });

  if(iter != defenses.end())
    defenses.erase(iter);
}

void Character::DefenseCheck(DefenseFrameStateJudge& judge, Spell& in, const DefenseOrder& filter)
{
  for (int i = 0; i < defenses.size(); i++) {
    if (defenses[i]->GetDefenseOrder() == filter) {
      DefenseRule* defenseRule = defenses[i];
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

void Character::QueueAction(CardAction * action)
{
  if (queuedAction) delete queuedAction;
  queuedAction = action;
}

void Character::EndCurrentAction()
{
  if (currentAction) delete currentAction;
  currentAction = nullptr;
}
