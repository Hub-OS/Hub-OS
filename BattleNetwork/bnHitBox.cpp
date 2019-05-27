#include <random>
#include <time.h>

#include "bnHitBox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

HitBox::HitBox(Field* _field, Team _team, int _damage) : Spell() {
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
<<<<<<< HEAD
  cooldown = 0;
=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  damage = _damage;

  auto props = Hit::DefaultProperties;
  props.damage = _damage;
  this->SetHitboxProperties(props);

  EnableTileHighlight(false);
}

<<<<<<< HEAD
HitBox::~HitBox(void) {
=======
HitBox::~HitBox() {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void HitBox::Update(float _elapsed) {
  tile->AffectEntities(this);
  this->Delete();
}

bool HitBox::Move(Direction _direction) {
  return false;
}

void HitBox::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}
