#include "bnRowHit.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

RowHit::RowHit(Field* _field, Team _team, int damage) : damage(damage), Spell(_field, _team) {
  SetLayer(0);

  auto texture = TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT);
  setTexture(*texture);
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    this->Delete();
  };

  auto onFrameTwo = [this]() {
    field->AddEntity(*new RowHit(field, this->GetTeam(), this->damage), this->tile->GetX() + 1, this->tile->GetY());
  };

  animation = Animation("resources/spells/spell_charged_bullet_hit.animation");
  animation.SetAnimation("HIT");
  animation << Animator::On(3, onFrameTwo, true) << onFinish;
  animation.Update(0, *this);
}

RowHit::~RowHit() {
}

void RowHit::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y - 20.0f);

  animation.Update(_elapsed, *this);

  tile->AffectEntities(this);
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
