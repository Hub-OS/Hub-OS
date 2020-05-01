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

BasicSword::BasicSword(Field* _field, Team _team, int _damage) : Spell(_field, _team){
  hit = false;
  cooldown = 0;

  this->HighlightTile(Battle::Tile::Highlight::solid);

  auto  props = GetHitboxProperties();;
  props.damage = _damage;
  props.flags |= Hit::flinch;
  this->SetHitboxProperties(props);
}

BasicSword::~BasicSword(void) {
}

void BasicSword::OnUpdate(float _elapsed) {
  if (cooldown >= COOLDOWN) {
    this->Delete();
    return;
  }

  tile->AffectEntities(this);

  cooldown += _elapsed;
}

bool BasicSword::Move(Direction _direction) {
  return false;
}

void BasicSword::Attack(Character* _entity) {
  hit = hit || _entity->Hit(GetHitboxProperties());
  hitHeight = _entity->GetHeight();
}

void BasicSword::OnDelete()
{
}
