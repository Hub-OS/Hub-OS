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
<<<<<<< HEAD
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT);

  //Components setup and load
=======
  
  auto texture = TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT);
  setTexture(*texture);
  setScale(2.f, 2.f);

  //When the animation ends, delete this
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  auto onFinish = [this]() {
    this->Delete();
  };

<<<<<<< HEAD
  auto onFrameTwo = [this]() {
=======
  // On the 3rd frame, spawn another RowHit
  auto onFrameThree = [this]() {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    field->AddEntity(*new RowHit(field, this->GetTeam(), this->damage), this->tile->GetX() + 1, this->tile->GetY());
  };

  animation = Animation("resources/spells/spell_charged_bullet_hit.animation");
  animation.SetAnimation("HIT");
<<<<<<< HEAD
  animation << Animate::On(3, onFrameTwo, true) << onFinish;
=======
  animation << Animate::On(3, onFrameThree, true) << onFinish;
  
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  animation.Update(0, *this);

  EnableTileHighlight(false);
}

<<<<<<< HEAD
RowHit::~RowHit(void) {
}

void RowHit::Update(float _elapsed) {
  setTexture(*texture);
  setScale(2.f, 2.f);

=======
RowHit::~RowHit() {
}

void RowHit::Update(float _elapsed) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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
