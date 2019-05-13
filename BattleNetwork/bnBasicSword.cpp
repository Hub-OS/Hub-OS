#include <random>
#include <time.h>

#include "bnBasicSword.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnProgsMan.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define COOLDOWN 40.0f/1000.f
#define DAMAGE_COOLDOWN 50.0f/1000.f

#define BULLET_ANIMATION_SPRITES 3
#define BULLET_ANIMATION_WIDTH 30
#define BULLET_ANIMATION_HEIGHT 27

BasicSword::BasicSword(Field* _field, Team _team, int _damage) {
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  srand((unsigned int)time(nullptr));
  cooldown = 0;
  damage = _damage;

  EnableTileHighlight(true);
}

BasicSword::~BasicSword(void) {
}

void BasicSword::Update(float _elapsed) {
  if (cooldown >= COOLDOWN || hit) {
    deleted = true;
    Entity::Update(_elapsed);
    return;
  }

  tile->AffectEntities(this);

  cooldown += _elapsed;
}

bool BasicSword::Move(Direction _direction) {
  return false;
}

void BasicSword::Attack(Character* _entity) {

  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    auto props = Hit::DefaultProperties;
    props.damage = damage;
    _entity->Hit(props);
    hitHeight = _entity->GetHitHeight();
    hit = true;
  }
}
