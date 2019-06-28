#include "bnCharacter.h"
#include "bnDefenseRule.h"
#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnExplosion.h"
#include "bnElementalDamage.h"
#include "bnShaderResourceManager.h

Character::Character(Rank _rank) : 
  health(0),
  maxHealth(0),
  counterable(false),
  canTilePush(true),
  slideFromDrag(false),
  canShareTile(false),
  stunCooldown(0),
  name("unnamed"),
  rank(_rank),
  invokeDeletion(false),
  hit(false),
  CounterHitPublisher() {

  burnCycle = sf::milliseconds(150);
  elapsedBurnTime = burnCycle.asSeconds();
  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);
}

Character::~Character() {
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
  return this->canShareTile;
}

void Character::ToggleTilePush(bool enabled)
{
  this->canTilePush = enabled;
}

const bool Character::CanTilePush() const {
  return this->canTilePush;
}

void Character::Update(float _elapsed) {
  if (!hit) {
    this->SetShader(nullptr);
  }
  else {
    SetShader(whiteout);
  }

  if ((((int)(stunCooldown * 15))) % 2 == 0) {
    this->SetShader(stun);
  }
  else {
    this->SetShader(nullptr);
  }

  elapsedBurnTime -= _elapsed;

  if (this->IsBattleActive() && !this->HasFloatShoe()) {
    if (this->GetTile()) {
      if (this->GetTile()->GetState() == TileState::POISON) {
        if (elapsedBurnTime <= 0) {
          if (this->Hit(Hit::Properties({ 1, 0x00, Element::NONE, 0, nullptr }))) {
            elapsedBurnTime = burnCycle.asSeconds();
          }
        }
      }
      else {
        elapsedBurnTime = 0;
      }

      if (this->GetTile()->GetState() == TileState::LAVA) {
        if (this->Hit(Hit::Properties({ 50, Hit::pierce, Element::FIRE, 0, nullptr }))) {
          Field* field = GetField();
          Artifact* explosion = new Explosion(field, this->GetTeam(), 1);
          field->AddEntity(*explosion, tile->GetX(), tile->GetY());
          this->GetTile()->SetState(TileState::NORMAL);
        }
      }
    }
  }

  if(this->stunCooldown > 0) {
    this->stunCooldown--;
  } else {
    this->OnUpdate(_elapsed);
  }

  //if (this->GetHealth() <= 0 && this->isSliding) {
  //  this->isSliding = false;
  //}

  Entity::Update(_elapsed);
}

bool Character::CanMoveTo(Battle::Tile * next)
{
  auto occupied = [this](Entity* in) {
    Character* c = dynamic_cast<Character*>(in);

    return c && c != this && !c->CanShareTileSpace();
  };

  return (Entity::CanMoveTo(next) && next->FindEntities(occupied).size() == 0);
}

const bool Character::Hit(Hit::Properties props) {
  if(this->IsPassthrough()) return false;

  // Add to status queue for state resolution
  this->statusQueue.push(props);

  return true;
}

int Character::GetHealth() const {
  return health;
}

const int Character::GetMaxHealth() const
{
  return this->maxHealth;
}

void Character::ResolveFrameBattleDamage()
{
  if(this->statusQueue.empty()) return;

  Character* frameCounterAggressor = nullptr;
  bool frameElementalDmg = false;
  bool frameSlideCancel = false;
  bool frameStunCancel = false;
  Direction postDragDir = Direction::NONE;

  Hit::Properties& props = this->statusQueue.front();
  
  while(props.flags == Hit::none) {
    this->statusQueue.pop();
    if (this->statusQueue.empty()) return;

    props = this->statusQueue.front();

    double tileDamage = 0;

    // Calculate elemental damage if the tile the character is on is super effective to it
    if (props.element == Element::FIRE
        && GetTile()->GetState() == TileState::GRASS
        && !(this->HasAirShoe() || this->HasFloatShoe())) {
      tileDamage = props.damage;
    } else if (props.element == Element::ELEC
               && GetTile()->GetState() == TileState::ICE
               && !(this->HasAirShoe() || this->HasFloatShoe())) {
      tileDamage = props.damage;
    }

    // If the character itself is also super-effective,
    // double the damage independently from tile damage
    bool isSuperEffective = IsSuperEffective(props.element);

    // Show ! super effective symbol on the field
    if (isSuperEffective || tileDamage) {
      // Additional damage bonus if super effective against the attack too
      if (isSuperEffective) {
        props.damage *= 2;
      }

      frameElementalDmg = true;
    }

    props.damage += tileDamage; // append tile damage

    // Pass on hit properties to the user-defined handler
    if (this->OnHit(props)) {
      // Only register counter if:
      // 1. Hit type is impact
      // 2. The character is on a counter frame
      // 3. Hit properties has an aggressor
      // This will set the counter aggressor to be the first non-impact hit and not check again this frame
      if (this->IsCountered() && (props.flags & Hit::impact) == Hit::impact && !frameCounterAggressor) {
        if (props.aggressor) {
          frameCounterAggressor = props.aggressor;
        }
      }

      if(this->IsSliding() && (props.flags & Hit::impact) == Hit::impact && !this->slideFromDrag) {
        frameSlideCancel = true;
      }

      // Requeue drag if already sliding by drag
      if((props.flags & Hit::drag) == Hit::drag) {
        if(this->slideFromDrag) {
          this->statusQueue.push({0, Hit::drag, Element::NONE, 0.0, nullptr, props.drag});
        } else {
          // Apply directional slide later
          postDragDir = props.drag;
        }
      }

      // Stun can be canceled by non-stun hits or queued if dragging
      if((props.flags & Hit::stun) == Hit::stun) {
        if(postDragDir != Direction::NONE) {
          this->statusQueue.push({0, Hit::stun, Element::NONE, props.secs});
        } else {
          this->stunCooldown = props.secs;
        }
      } else if(this->stunCooldown > 0) {
        // cancel
        this->stunCooldown = 0;
      }
    }

    this->SetHealth(health - props.damage);
  }

  if(frameSlideCancel) {
    this->AdoptNextTile();
    this->SlideToTile(false); // override ICE check in AdoptNextTile routine
  }

  if(postDragDir != Direction::NONE) {
    // enemies and objects on opposing side of field are granted immunity from drag
    if(Teammate(this->GetTile()->GetTeam())) {
      this->SlideToTile(true);
      this->slideFromDrag = true;
      this->Move(postDragDir);
    }
  } else if(this->slideFromDrag){
    // No dragging status in this frame, no sliding
    this->slideFromDrag = false;
  }

  if(frameCounterAggressor) {
    this->Broadcast(*this, *frameCounterAggressor);
    this->ToggleCounter(false);
    this->Stun(3.0);
  }

  if(frameElementalDmg) {
    Artifact *seSymbol = new ElementalDamage(field);
    field->AddEntity(*seSymbol, tile->GetX(), tile->GetY());
  }

  if (this->GetHealth() == 0 && !this->invokeDeletion) {
    this->OnDelete();
    this->invokeDeletion = true;

    if(frameCounterAggressor) {
      // Slide entity back a few pixels
      this->tileOffset = sf::Vector2f(50.f, 0.0f);
    }
  }
}

void Character::SetHealth(const int _health) {
  health = _health;

  if (maxHealth == 0) {
    maxHealth = health;
  }

  if (health < 0) health = 0;
  if (health > maxHealth) health = maxHealth;
}

void Character::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!isSliding) {
    this->setPosition(tile->getPosition());
  }
}

void Character::TryDelete() {
  deleted = (health <= 0);
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
  this->name = name;
}

const std::string Character::GetName() const
{
  return name;
}

void Character::AddDefenseRule(DefenseRule * rule)
{
  if (rule) {
    defenses.push_back(rule);
    std::sort(defenses.begin(), defenses.end(), [](DefenseRule* first, DefenseRule* second) { return first->GetPriorityLevel() < second->GetPriorityLevel(); });
  }
}

void Character::RemoveDefenseRule(DefenseRule * rule)
{
  auto iter = std::remove_if(defenses.begin(), defenses.end(), [&rule](DefenseRule * in) { return in == rule; });
  defenses.erase(iter);
}

const bool Character::CheckDefenses(Spell* in)
{
  for (int i = 0; i < defenses.size(); i++) {
    if (defenses[i]->Check(in, this)) {
      return true;
    }
  }

  return false;
}
