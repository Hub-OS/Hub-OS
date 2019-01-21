#include "bnCharacter.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnExplosion.h"

Character::Character(Rank _rank) : 
  health(0),
  counterable(false),
  stunCooldown(0),
  name("unnamed"),
  rank(_rank),
  CounterHitPublisher() {
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

  if (this->IsBattleActive() && !this->HasFloatShoe()) {
    if (this->GetTile()) {
      if (this->GetTile()->GetState() == TileState::POISON) {
        if (elapsedBurnTime <= 0) {
          if (this->Hit(1, Entity::HitProperties({ false, false, false, true, 0, nullptr }))) {
            elapsedBurnTime = burnCycle.asSeconds();
          }
        }
      }
      else {
        elapsedBurnTime = 0;
      }

      if (this->GetTile()->GetState() == TileState::LAVA) {
        if (this->Hit(50, Entity::HitProperties({ true, false, false, false, 0, nullptr }))) {
          Field* field = GetField();
          Entity* explosion = new Explosion(field, this->GetTeam(), 1);
          field->OwnEntity(explosion, tile->GetX(), tile->GetY());
          this->GetTile()->SetState(TileState::NORMAL);
        }
      }
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

const bool Character::Hit(int damage, HitProperties props) {
  int previousHealth = health;

  (health - damage < 0) ? this->SetHealth(0) : this->SetHealth(health - damage);
   

  if (this->IsCountered() && props.recoil) {
    this->Stun(3.0);

    if (this->GetHealth() == 0) {
      // Slide entity back a few pixels
      this->tileOffset = sf::Vector2f(50.f, 0.0f);
    }

    this->Broadcast(*this, *props.aggressor);
  }

  return (health != previousHealth);
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
