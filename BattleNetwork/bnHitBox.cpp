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

  EnableTileHighlight(false);
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
  _entity->Hit(GetHitboxProperties());
}
