#include <random>
#include <time.h>

#include "bnBuster.h"
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

Buster::Buster(Field* _field, Team _team, bool _charged) {
  SetPassthrough(true);

  cooldown = 0;
  damageCooldown = 0;
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  progress = 0.0f;
  hitHeight = 0.0f;
  srand((unsigned int)time(nullptr));
  random = rand() % 20 - 20;
  if (_charged) {
    damage = 10;
    //TODO: make new sprite animation for charged bullet
    texture = TEXTURES.GetTexture(TextureType::SPELL_BULLET_HIT);
  } else {
    damage = 1;
    texture = TEXTURES.GetTexture(TextureType::SPELL_BULLET_HIT);
  }
  setScale(2.f, 2.f);
  for (int x = 0; x < BULLET_ANIMATION_SPRITES; x++) {
    animation.Add(0.3f, IntRect(BULLET_ANIMATION_WIDTH*x, 0, BULLET_ANIMATION_WIDTH, BULLET_ANIMATION_HEIGHT));
  }
}

Buster::~Buster(void) {
}

void Buster::Update(float _elapsed) {
  if (hit) {
    if (progress == 0.0f) {
      setTexture(*texture);
      setPosition(tile->getPosition().x + random, tile->getPosition().y - hitHeight);
    }
    progress += 5 * _elapsed;
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

bool Buster::Move(Direction _direction) {
  tile->RemoveEntity(this);
  Battle::Tile* next = nullptr;
  if (_direction == Direction::UP) {
    if (tile->GetY() - 1 > 0) {
      next = field->GetAt(tile->GetX(), tile->GetY() - 1);
      SetTile(next);
    }
  } else if (_direction == Direction::LEFT) {
    if (tile->GetX() - 1 > 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
      SetTile(next);
    } else {
      deleted = true;
      return false;
    }
  } else if (_direction == Direction::DOWN) {
    if (tile->GetY() + 1 <= (int)field->GetHeight()) {
      next = field->GetAt(tile->GetX(), tile->GetY() + 1);
      SetTile(next);
    }
  } else if (_direction == Direction::RIGHT) {
    if (tile->GetX() < (int)field->GetWidth()) {
      next = field->GetAt(tile->GetX() + 1, tile->GetY());
      SetTile(next);
    } else {
      deleted = true;
      return false;
    }
  }
  tile->AddEntity(this);
  return true;
}

void Buster::Attack(Entity* _entity) {
  if (hit || deleted) return;

  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    auto props = Entity::DefaultHitProperties;
    props.recoil = false;

    _entity->Hit(damage, props);
    hitHeight = _entity->GetHitHeight();

    if (!_entity->IsPassthrough()) {
      hit = true;
    }
  }

  if (hit) {
    AUDIO.Play(AudioType::HURT);
  }
}

vector<Drawable*> Buster::GetMiscComponents() {
  return vector<Drawable*>();
}
