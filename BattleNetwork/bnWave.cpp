#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define WAVE_ANIMATION_SPRITES 5
#define WAVE_ANIMATION_WIDTH 41
#define WAVE_ANIMATION_HEIGHT 46

Wave::Wave(Field* _field, Team _team, double speed) : Spell() {
  SetLayer(0);
  cooldown = 0;
  damageCooldown = 0;
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_WAVE);
  this->speed = speed;
  for (int x = 0; x < WAVE_ANIMATION_SPRITES; x++) {
    animation.Add(0.3f, IntRect(WAVE_ANIMATION_WIDTH*x, 0, WAVE_ANIMATION_WIDTH, WAVE_ANIMATION_HEIGHT));
  }

  //Components setup and load
  auto onFinish = [this]() {
    Move(direction);
    AUDIO.Play(AudioType::WAVE);
    cooldown = 0;
    progress = 0.0f; 
  };

  animator << onFinish;

  progress = 0.0f;
  hitHeight = 0.0f;
  random = 0;

  AUDIO.Play(AudioType::WAVE);

  EnableTileHighlight(true);
}

Wave::~Wave(void) {
}

void Wave::Update(float _elapsed) {
  if (!tile->IsWalkable()) {
    deleted = true;
    Entity::Update(_elapsed);
    return;
  }

  setTexture(*texture);
  setScale(2.f, 2.f);
  setPosition(tile->getPosition().x, tile->getPosition().y-this->getLocalBounds().height);
  progress += 3 * _elapsed;
  
  animator(progress*(float)speed, *this, animation);

  tile->AffectEntities(this);

  Entity::Update(_elapsed);
}

bool Wave::Move(Direction _direction) {
  tile->RemoveEntity(this);
  Battle::Tile* next = nullptr;
  if (_direction == Direction::LEFT) {
    if (tile->GetX() - 1 > 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
      SetTile(next);
    } else {
      deleted = true;
      return false;
    }
  } else if (_direction == Direction::RIGHT) {
    if (tile->GetX() + 1 <= (int)field->GetWidth()) {
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

void Wave::Attack(Entity* _entity) {
  Player* isPlayer = dynamic_cast<Player*>(_entity);
  if (isPlayer) {
    bool hit = isPlayer->Hit(10);
  }
}

vector<Drawable*> Wave::GetMiscComponents() {
  return vector<Drawable*>();
}