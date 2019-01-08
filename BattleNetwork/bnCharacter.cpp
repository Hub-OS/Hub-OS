#include "bnCharacter.h"
#include "bnTile.h"
#include "bnField.h"

Character::Character(Rank _rank) :
  health(0),
  counterable(false),
  stunCooldown(0),
  name("unnamed"),
  rank(_rank) {
  burnCycle = sf::milliseconds(150);
  elapsedBurnTime = burnCycle.asSeconds();
}

Character::~Character() {
}

const Character::Rank Character::GetRank() const {
  return rank;
}

void Character::Update(float _elapsed) {
  elapsedBurnTime -= _elapsed;

  if (this->IsBattleActive()) {
    if (this->GetTile() && this->GetTile()->GetState() == TileState::LAVA) {
      if (elapsedBurnTime <= 0 && this->GetElement() != Element::FIRE) {
        elapsedBurnTime = burnCycle.asSeconds();
        this->SetHealth(this->GetHealth() - 1);
      }
    }
    else {
      elapsedBurnTime = 0;
    }
  }

  Entity::Update(_elapsed);
}

bool Character::CanMoveTo(Battle::Tile * next)
{
  return (Entity::CanMoveTo(next) && !next->ContainsEntityType<Character>());
}

vector<Drawable*> Character::GetMiscComponents() {
  assert(false && "GetMiscComponents shouldn't be called directly from Character");
  return vector<Drawable*>();
}

void Character::AddAnimation(string _state, FrameList _frameList, float _duration) {
  assert(false && "AddAnimation shouldn't be called directly from Character");
}

void Character::SetAnimation(string _state) {
  assert(false && "SetAnimation shouldn't be called directly from Character");
}

void Character::SetCounterFrame(int frame)
{
  assert(false && "SetCounterFrame shouldn't be called directly from Character");
}

void Character::OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave, bool doOnce)
{
  assert(false && "OnFrameCallback shouldn't be called directly from Character");
}

int Character::GetHealth() const {
  return health;
}

const float Character::GetHitHeight() const {
  //assert(false && "GetHitHeight shouldn't be called directly from Entity");
  return 0;
}

const bool Character::Hit(int damage) {
  this->SetHealth(this->GetHealth() - damage);

  if (this->GetHealth() < 0) {
    this->SetHealth(0);
  }

  return true;
}

int* Character::GetAnimOffset() {
  return nullptr;
}

void Character::SetHealth(const int _health) {
  health = _health;
  if (health < 0) health = 0;

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
