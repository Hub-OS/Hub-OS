#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Wave::Wave(Field* _field, Team _team, double speed) : Spell() {
  SetLayer(0);
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_WAVE);
  this->speed = speed;

  //Components setup and load
  auto onFinish = [this]() {
    Move(direction);
    AUDIO.Play(AudioType::WAVE);
  };

  animation = Animation("resources/spells/spell_wave.animation");
  animation.SetAnimation("DEFAULT");
  animation << Animate::Mode::Loop << Animate::On(5, onFinish);

  auto props = Hit::DefaultProperties;
  props.damage = 10;
  this->SetHitboxProperties(props);

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

  int lr = (this->GetDirection() == Direction::LEFT) ? 1 : -1;
  setScale(2.f*(float)lr, 2.f);

  setPosition(tile->getPosition().x, tile->getPosition().y);
  
  animation.Update(_elapsed, *this);

  tile->AffectEntities(this);

  Entity::Update(_elapsed);
}

bool Wave::Move(Direction _direction) {
  tile->RemoveEntityByID(this->GetID());
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
  tile->AddEntity(*this);
  return true;
}

void Wave::Attack(Character* _entity) {
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    _entity->Hit(GetHitboxProperties());
  }
}
