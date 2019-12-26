#include <random>
#include <time.h>

#include "bnHitBox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

HitBox::HitBox(Field* _field, Team _team, int _damage) : Spell(_field, _team) {
  hit = false;
  damage = _damage;

  auto props = Hit::DefaultProperties;
  props.damage = _damage;
  this->SetHitboxProperties(props);

  callback = 0;
}

HitBox::~HitBox() {
}

void HitBox::OnUpdate(float _elapsed) {
  tile->AffectEntities(this);
  this->Delete();
}

bool HitBox::Move(Direction _direction) {
  return false;
}

void HitBox::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties()) && callback) {
    callback(_entity);
  }
}

void HitBox::AddCallback(decltype(callback) callback)
{
  this->callback = callback;
}
