#include <random>
#include <time.h>

#include "bnAirShot.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnProgsMan.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define COOLDOWN 40.0f/1000.0f
#define DAMAGE_COOLDOWN 50.0f/1000.0f

#define BULLET_ANIMATION_SPRITES 3
#define BULLET_ANIMATION_WIDTH 30
#define BULLET_ANIMATION_HEIGHT 27

AirShot::AirShot(Field* _field, Team _team, int _damage) : Spell(_field, _team) {
  this->SetPassthrough(true);

  hit = false;
  progress = 0.0f;
  hitHeight = 10.0f;
  random = rand() % 20 - 20;
  cooldown = 0.0f;

  SetDirection(Direction::RIGHT);

  damage = _damage;
}

AirShot::~AirShot() {
}

void AirShot::OnUpdate(float _elapsed) {
  GetTile()->AffectEntities(this);

  cooldown += _elapsed;
  if (cooldown >= COOLDOWN) {
    Move(GetDirection());
    cooldown = 0;
  }

}

void AirShot::Attack(Character* _entity) {
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    auto props = Hit::DefaultProperties;
    props.damage = damage;
    props.flags |= Hit::drag;
    props.drag = GetDirection();

    if(_entity->Hit(props)) {
      this->Delete();
    }

    hitHeight = _entity->GetHitHeight();
  }
}