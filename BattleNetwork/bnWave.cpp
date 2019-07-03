#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSharedHitBox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Wave::Wave(Field* _field, Team _team, double speed) : Spell(_field, _team) {
  SetLayer(0);


  setTexture(*TEXTURES.GetTexture(TextureType::SPELL_WAVE));
  this->speed = speed;

  //Components setup and load
  auto onFinish = [this]() {
    if (Move(GetDirection())) {
      AUDIO.Play(AudioType::WAVE);
    }
  };

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);

  animation->Setup("resources/spells/spell_wave.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT", Animate::Mode::Loop, onFinish);

  auto props = Hit::DefaultProperties;
  props.damage = 10;
  this->SetHitboxProperties(props);

  AUDIO.Play(AudioType::WAVE);

  EnableTileHighlight(true);
}

Wave::~Wave() {
}

void Wave::OnUpdate(float _elapsed) {
  int lr = (this->GetDirection() == Direction::LEFT) ? 1 : -1;
  setScale(2.f*(float)lr, 2.f);

  setPosition(tile->getPosition().x, tile->getPosition().y);

  if (!this->IsDeleted()) {
    tile->AffectEntities(this);
  }
}

bool Wave::Move(Direction _direction) {
  // Drop a shared hitbox when moving

  //SharedHitBox* shb = new SharedHitBox(this, 1.0f/60.0f);
  //GetField()->AddEntity(*shb, tile->GetX(), tile->GetY());
  
  tile->RemoveEntityByID(this->GetID());
  Battle::Tile* next = nullptr;

  if (_direction == Direction::LEFT) {
    if (tile->GetX() - 1 > 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
    }
  } else if (_direction == Direction::RIGHT) {
    if (tile->GetX() + 1 <= (int)field->GetWidth()) {
      next = field->GetAt(tile->GetX() + 1, tile->GetY());
    }
  }

  if (next && next->IsWalkable()) {
    next->AddEntity(*this);
    
    return true;
  }

  // If our next tile pointer is invalid, we cannot move
  // and must mark ourselves for deletion
  tile->RemoveEntityByID(this->GetID());
  this->Delete();
  return false;
}

void Wave::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}
