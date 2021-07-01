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

AirShot::AirShot(Team _team,int _damage) : Spell(_team) {
  SetPassthrough(true);
  SetFloatShoe(true);

  hit = false;
  progress = 0.0f;
  hitHeight = 10.0f;
  random = rand() % 20 - 20;
  cooldown = 0.0f;

  SetDirection(Direction::right);

  damage = _damage;

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::drag;
  props.element = Element::wind;
  props.drag = { Direction::right, 9u };
  SetHitboxProperties(props);
}

AirShot::~AirShot() {
}

void AirShot::OnUpdate(double _elapsed) {
  GetTile()->AffectEntities(this);

  cooldown += _elapsed;
  if (cooldown >= COOLDOWN) {
    if (GetTile()->IsEdgeTile()) { Delete(); }

    auto facing = GetFacing();
    Battle::Tile* dest = GetTile() + GetFacing();

    if (CanMoveTo(dest)) {
      Teleport(dest);
    }

    cooldown = 0;
  }
}

void AirShot::Attack(Character* _entity) {
  // TODO: Spells should auto-update drag flags based on direction?
  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::drag;
  props.drag = { GetFacing(), 9u };
  SetHitboxProperties(props);

  if(_entity->Hit(GetHitboxProperties())) {
    Remove();
  }

  hitHeight = _entity->GetHeight();
}

bool AirShot::CanMoveTo(Battle::Tile * next)
{
  return true;
}

void AirShot::OnDelete()
{
}
