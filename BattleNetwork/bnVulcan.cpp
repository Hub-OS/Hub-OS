#include <random>
#include <time.h>

#include "bnVulcan.h"
#include "bnParticleImpact.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnGuardHit.h"
#include "bnGear.h" 

#define COOLDOWN 40.0f/1000.0f

Vulcan::Vulcan(Field* _field, Team _team,int damage) :Spell(_field, _team) {
  SetPassthrough(true);
  SetLayer(0);

  cooldown = 0;

  hitHeight = 20.0f;

  random = 0;

  AUDIO.Play(AudioType::GUN, AudioPriority::HIGHEST);

  auto props = GetHitboxProperties();
  props.flags = props.flags & ~Hit::recoil;
  props.damage = damage;
  this->SetHitboxProperties(props);

  HighlightTile(Battle::Tile::Highlight::none);
}

Vulcan::~Vulcan() {
}

void Vulcan::OnUpdate(float _elapsed) {
  // Strike panel and leave
  GetTile()->AffectEntities(this);

  if (Move(GetDirection())) {
    this->AdoptNextTile();
    FinishMove();
  }

  if (GetTile()->IsEdgeTile()) {
    //this->Delete();
  }
}

bool Vulcan::CanMoveTo(Battle::Tile* next) {
  return true;
}

void Vulcan::Attack(Character* _entity) {
  if (dynamic_cast<Gear*>(_entity)) {
    field->AddEntity(*new GuardHit(field, _entity), *_entity->GetTile());
    this->Delete();
    return;
  }

  if (_entity->Hit(this->GetHitboxProperties())) {
    AUDIO.Play(AudioType::HURT);
    auto impact = new ParticleImpact(ParticleImpact::Type::YELLOW);
    impact->SetHeight(_entity->GetHeight());
    field->AddEntity(*impact, *_entity->GetTile());
    this->Delete();
  }
}