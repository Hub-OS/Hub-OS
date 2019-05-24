#include "bnRowHit.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

RowHit::RowHit(Field* _field, Team _team, int damage) : damage(damage), Spell() {
  SetLayer(0);
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  
  auto texture = TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT);
  setTexture(*texture);
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    this->Delete();
  };

  // On the 3rd frame, spawn another RowHit
  auto onFrameThree = [this]() {
    field->AddEntity(*new RowHit(field, this->GetTeam(), this->damage), this->tile->GetX() + 1, this->tile->GetY());
  };

  animation = Animation("resources/spells/spell_charged_bullet_hit.animation");
  animation.SetAnimation("HIT");
  animation << Animate::On(3, onFrameThree, true) << onFinish;
  
  animation.Update(0, *this);

  EnableTileHighlight(false);
}

RowHit::~RowHit() {
}

void RowHit::Update(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y - 20.0f);

  animation.Update(_elapsed, *this);

  tile->AffectEntities(this);

  Entity::Update(_elapsed);
}

bool RowHit::Move(Direction _direction) {
  return false;
}

void RowHit::Attack(Character* _entity) {
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    auto props = Hit::DefaultProperties;
    props.damage = damage;
    _entity->Hit(props);
  }
}
