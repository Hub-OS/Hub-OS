#include "bnCharacter.h"
#include "bnDefenseRule.h"
#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnExplosion.h"
#include "bnElementalDamage.h"
#include "bnShaderResourceManager.h"

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

void Character::EnableTilePush(bool enabled)
{
  this->canTilePush = enabled;
}

const bool Character::CanTilePush() const {
  return this->canTilePush;
}

void Character::Update(float _elapsed) {
  this->ResolveFrameBattleDamage();

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
    this->stunCooldown-=_elapsed;
  } else {
    this->stunCooldown = 0;
    this->OnUpdate(_elapsed);
  }

  //if (this->GetHealth() <= 0 && this->isSliding) {
  //  this->isSliding = false;
  //}

  Entity::Update(_elapsed);

  if (!hit) {
      if (stunCooldown && (((int)(stunCooldown * 15))) % 2 == 0) {
          this->SetShader(stun);
      }
      else if(!this->IsDeleted()) {
          this->SetShader(nullptr);
      }
  }
  else {
      SetShader(whiteout);
  }

  hit = false;
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
  // If the character itself is also super-effective,
  // double the damage independently from tile damage
  bool isSuperEffective = IsSuperEffective(props.element);

  // Show ! super effective symbol on the field
  if (isSuperEffective) {
    // Additional damage bonus if super effective against the attack too
    if (isSuperEffective) {
      props.damage *= 2;
    }

    Artifact *seSymbol = new ElementalDamage(field);
    field->AddEntity(*seSymbol, tile->GetX(), tile->GetY());
  }

  if (!this->IsPassthrough()) {
    this->SetHealth(GetHealth() - props.damage);

    for (auto c : shareHit) {
      c->Hit(props);
    }
  }
  else {
    return false;
  }

  // Add to status queue for state resolution
  this->statusQueue.push(props);

  Logger::Log("pushing states");

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
  bool frameStunCancel = false;
  Direction postDragDir = Direction::NONE;

  std::queue<Hit::Properties> append;

  while(!this->statusQueue.empty()) {
      Hit::Properties& props = this->statusQueue.front();
    this->statusQueue.pop();

    int tileDamage = 0;

    // Calculate elemental damage if the tile the character is on is super effective to it
    if (props.element == Element::FIRE
      && GetTile()->GetState() == TileState::GRASS
      && !(this->HasAirShoe() || this->HasFloatShoe())) {
      tileDamage = props.damage;
    }
    else if (props.element == Element::ELEC
      && GetTile()->GetState() == TileState::ICE
      && !(this->HasAirShoe() || this->HasFloatShoe())) {
      tileDamage = props.damage;
    }

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
        this->CancelSlide();
      }

      // Requeue drag if already sliding by drag
      if((props.flags & Hit::drag) == Hit::drag) {
        if(this->slideFromDrag) {
          append.push({0, Hit::drag, Element::NONE, 0.0, nullptr, props.drag});
        } else {
          // Apply directional slide later
          postDragDir = props.drag;
        }
      }

      // Stun can be canceled by non-stun hits or queued if dragging
      if((props.flags & Hit::stun) == Hit::stun) {
        if(postDragDir != Direction::NONE) {
          append.push({0, Hit::stun, Element::NONE, props.secs});
        } else {
          this->stunCooldown = props.secs;
        }
      } else if(this->stunCooldown > 0) {
        // cancel
        this->stunCooldown = 0;
      }

      hit = hit || true;
    }

    if (hit) {
      if (this->GetHealth() == 0) {
        this->stunCooldown = 0;
      }
    }
  }

  if(!append.empty()) {
      this->statusQueue = append;
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

  if (!IsSliding()) {
    this->setPosition(tile->getPosition());
  }
}

void Character::TryDelete() {
    if (this->GetHealth() == 0 && !this->invokeDeletion) {
        this->OnDelete();
        this->invokeDeletion = true;
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
  this->name = name;
}

const std::string Character::GetName() const
{
  return name;
}

void Character::AddDefenseRule(DefenseRule * rule)
{
  auto iter = std::find(defenses.begin(), defenses.end(), rule);

  if (rule && iter == defenses.end()) {
    defenses.push_back(rule);
    std::sort(defenses.begin(), defenses.end(), [](DefenseRule* first, DefenseRule* second) { return first->GetPriorityLevel() < second->GetPriorityLevel(); });
  }
}

void Character::RemoveDefenseRule(DefenseRule * rule)
{
  auto iter = std::remove_if(defenses.begin(), defenses.end(), [&rule](DefenseRule * in) { return in == rule; });

  if(iter != defenses.end())
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

void Character::ShareHitboxDamage(Character * to)
{
  auto iter = std::find(shareHit.begin(), shareHit.end(), to);

  if (to && iter == shareHit.end()) {
    shareHit.push_back(to);
  }
}

void Character::CancelShareHitboxDamage(Character * to)
{
  auto iter = std::remove_if(shareHit.begin(), shareHit.end(), [&to](Character * in) { return in == to; });

  if(iter != shareHit.end())
    shareHit.erase(iter);
}
