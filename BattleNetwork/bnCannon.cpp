#include <random>
#include <time.h>

#include "bnCannon.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnProgsMan.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define COOLDOWN 40.0f/1000.0f

Cannon::Cannon(Field* _field, Team _team, int _damage) : Spell(_field, _team){
  SetPassthrough(true);

  hit = false;
  progress = 0.0f;

  random = rand() % 20 - 20;

  if(_team == Team::RED) {
    SetDirection(Direction::RIGHT);
  } else {
    SetDirection(Direction::LEFT);
  }
  damage = _damage;

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::flinch;
  SetHitboxProperties(props);
}

Cannon::~Cannon() {
}

void Cannon::OnUpdate(float _elapsed) {
  if (hit) {
      this->Delete();
    return;
  }

  tile->AffectEntities(this);

  cooldown += _elapsed;
  if (cooldown >= COOLDOWN) {
    Move(GetDirection());
    this->AdoptNextTile();
    cooldown = 0;
  }
}

bool Cannon::CanMoveTo(Battle::Tile * next)
{
  return true;
}

void Cannon::Attack(Character* _entity) {
  if (hit) {
    return;
  }

  _entity->Hit(GetHitboxProperties());
  hitHeight = _entity->GetHitHeight();

  if (!_entity->IsPassthrough()) {
    hit = true;
  }
}

