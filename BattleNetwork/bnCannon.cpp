#include <random>
#include <time.h>

#include "bnCannon.h"
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

Cannon::Cannon(Field* _field, Team _team, int _damage) {
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

  damage = _damage;
  //TODO: make new sprite animation for charged bullet
  texture = TEXTURES.GetTexture(TextureType::SPELL_BULLET_HIT);
 
  setScale(2.f, 2.f);
  for (int x = 0; x < BULLET_ANIMATION_SPRITES; x++) {
    animation.Add(0.3f, IntRect(BULLET_ANIMATION_WIDTH*x, 0, BULLET_ANIMATION_WIDTH, BULLET_ANIMATION_HEIGHT));
  }
}

Cannon::~Cannon(void) {
}

void Cannon::Update(float _elapsed) {
  if (hit) {
    if (progress == 0.0f) {
      setTexture(*texture);
      setPosition(tile->getPosition().x + random, tile->getPosition().y - hitHeight);
    }
    progress += 3 * _elapsed;
    animator(fmin(progress, 1.0f), *this, animation);
    if (progress >= 1.f) {
      deleted = true;
      Entity::Update(_elapsed);
    }
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

bool Cannon::Move(Direction _direction) {
  tile->RemoveEntity(this);
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
  tile->AddEntity(this);
  return true;
}

void Cannon::Attack(Entity* _entity) {
  if (hit || deleted) {
    return;
  }

  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    _entity->Hit(damage);
    hitHeight = _entity->GetHitHeight();

    if (!_entity->IsPassthrough()) {
      hit = true;
    }

    Character* isCharacter = dynamic_cast<Character*>(_entity);

    if (isCharacter && isCharacter->IsCountered()) {
      AUDIO.Play(AudioType::COUNTER, AudioPriority::LOWEST);
      isCharacter->Stun(1);

      if (isCharacter->GetHealth() == 0) {
        // Slide entity back a few pixels
        isCharacter->setPosition(isCharacter->getPosition().x + 50.f, isCharacter->getPosition().y);
      }
    }
  }
}

vector<Drawable*> Cannon::GetMiscComponents() {
  return vector<Drawable*>();
}
