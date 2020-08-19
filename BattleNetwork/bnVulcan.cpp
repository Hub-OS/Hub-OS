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

Vulcan::Vulcan(Field* _field, Team _team,int damage) : Spell(_field, _team) {
  SetPassthrough(true);
  SetLayer(0);

  AUDIO.Play(AudioType::GUN, AudioPriority::highest);

  auto props = GetHitboxProperties();
  props.flags = props.flags & ~Hit::recoil;
  props.damage = damage;
  SetHitboxProperties(props);

  HighlightTile(Battle::Tile::Highlight::none);
}

Vulcan::~Vulcan() {
}

void Vulcan::OnUpdate(float _elapsed) {
  // Strike panel and leave
  GetTile()->AffectEntities(this);

  if (Move(GetDirection())) {
    AdoptNextTile();
    FinishMove();
  }

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
    AUDIO.Play(AudioType::HURT);
    auto impact = new ParticleImpact(ParticleImpact::Type::VULCAN);
    impact->SetHeight(_entity->GetHeight());
    field->AddEntity(*impact, *_entity->GetTile());

    auto tile = _entity->GetTile();

    auto next = GetField()->GetAt(tile->GetX() + 1, tile->GetY());

    if (next) {
      impact = new ParticleImpact(ParticleImpact::Type::THIN);
      impact->SetHeight(_entity->GetHeight()/2.f);

      field->AddEntity(*impact, *next);

      Spell* hitbox = new Hitbox(field, GetTeam(), 0);
      hitbox->SetHitboxProperties(GetHitboxProperties());
      field->AddEntity(*hitbox, *next);
    }
    Delete();
  }
}