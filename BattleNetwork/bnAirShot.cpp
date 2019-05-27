#include <random>
#include <time.h>

#include "bnAirShot.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnProgsMan.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define COOLDOWN 40.0f/1000.0f
#define DAMAGE_COOLDOWN 50.0f/1000.0f

#define BULLET_ANIMATION_SPRITES 3
#define BULLET_ANIMATION_WIDTH 30
#define BULLET_ANIMATION_HEIGHT 27

AirShot::AirShot(Field* _field, Team _team, int _damage) {
  SetPassthrough(true);

  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  progress = 0.0f;
  hitHeight = 10.0f;
  srand((unsigned int)time(nullptr));
  random = rand() % 20 - 20;
  cooldown = 0.0f;

  damage = _damage;
}

AirShot::~AirShot() {
}

void AirShot::Update(float _elapsed) {
  if (hit) {
      deleted = true;
      Entity::Update(_elapsed);
    return;
  }

  tile->AffectEntities(this);

  cooldown += _elapsed;
  if (cooldown >= COOLDOWN) {
    Move(direction);
    cooldown = 0;
  }

  Entity::Update(_elapsed);
}

bool AirShot::Move(Direction _direction) {
  tile->RemoveEntityByID(this->GetID());
  Battle::Tile* next = nullptr;
  if (_direction == Direction::UP) {
    if (tile->GetY() - 1 > 0) {
      next = field->GetAt(tile->GetX(), tile->GetY() - 1);
      SetTile(next);
    }
  }
  else if (_direction == Direction::LEFT) {
    if (tile->GetX() - 1 > 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
      SetTile(next);
    }
    else {
      deleted = true;
      return false;
    }
  }
  else if (_direction == Direction::DOWN) {
    if (tile->GetY() + 1 <= (int)field->GetHeight()) {
      next = field->GetAt(tile->GetX(), tile->GetY() + 1);
      SetTile(next);
    }
  }
  else if (_direction == Direction::RIGHT) {
    if (tile->GetX() < (int)field->GetWidth()) {
      next = field->GetAt(tile->GetX() + 1, tile->GetY());
      SetTile(next);
    }
    else {
      deleted = true;
      return false;
    }
  }
  tile->AddEntity(*this);
  return true;
}

void AirShot::Attack(Character* _entity) {
  if (hit || deleted) {
    return;
  }

  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    auto props = Hit::DefaultProperties;
    props.damage = damage;
    _entity->Hit(props);
    hitHeight = _entity->GetHitHeight();

    if (!_entity->IsPassthrough()) {
      hit = true;

      _entity->SlideToTile(true);
      _entity->Move(direction);
    }
  }
}