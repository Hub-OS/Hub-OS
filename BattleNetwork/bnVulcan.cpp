#include <random>
#include <time.h>

#include "bnVulcan.h"
#include "bnParticleImpact.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnHitbox.h"
#include "bnGear.h" 

#define COOLDOWN 40.0f/1000.0f

Vulcan::Vulcan(Team _team, int damage) : Spell(_team) {
  SetPassthrough(true);
  SetLayer(0);

  Audio().Play(AudioType::GUN, AudioPriority::highest);

  auto props = GetHitboxProperties();
  props.flags = props.flags & ~Hit::flash;
  props.damage = damage;
  SetHitboxProperties(props);

  HighlightTile(Battle::Tile::Highlight::none);
}

Vulcan::~Vulcan() {
}

void Vulcan::OnUpdate(double _elapsed) {
  // Strike panel and leave
  GetTile()->AffectEntities(this);

  Teleport(GetTile() + GetMoveDirection());

  if (GetTile()->IsEdgeTile()) {
    Delete();
  }
}

bool Vulcan::CanMoveTo(Battle::Tile* next) {
  return true;
}

void Vulcan::OnDelete()
{
  Remove();
}

void Vulcan::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    Audio().Play(AudioType::HURT);
    auto impact = new ParticleImpact(ParticleImpact::Type::vulcan);
    impact->SetHeight(_entity->GetHeight());
    field->AddEntity(*impact, *_entity->GetTile());

    auto tile = _entity->GetTile();

    auto next = GetField()->GetAt(tile->GetX() + 1, tile->GetY());

    if (next) {
      Spell* hitbox = new Hitbox(GetTeam(), 0);
      hitbox->SetHitboxProperties(GetHitboxProperties());
      field->AddEntity(*hitbox, *next);
    }
    Delete();
  }
}