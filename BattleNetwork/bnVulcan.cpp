#include <random>
#include <time.h>

#include "bnVulcan.h"
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

  hit = false;
  progress = 0.0f;

  hitHeight = 20.0f;

  random = 0;

  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);

  texture = TEXTURES.GetTexture(TextureType::SPELL_BULLET_HIT);
  animationComponent->Setup("resources/spells/spell_bullet_hit.animation");
  animationComponent->Reload();
  animationComponent->SetAnimation("HIT");

  setScale(2.f, 2.f);

  AUDIO.Play(AudioType::GUN, AudioPriority::HIGHEST);

  auto props = GetHitboxProperties();
  props.flags = props.flags & ~Hit::recoil;
  props.damage = damage;
  this->SetHitboxProperties(props);

  contact = nullptr;
  spawnGuard = false;

  HighlightTile(Battle::Tile::Highlight::solid);
}

Vulcan::~Vulcan() {
}

void Vulcan::OnUpdate(float _elapsed) {
  if (spawnGuard) {
    field->AddEntity(*new GuardHit(field, contact), this->tile->GetX(), this->tile->GetY());
    spawnGuard = false;
    this->Delete();
    return;
  }

  if (hit) {
    animationComponent->Reload();
    animationComponent->SetAnimation("HIT");

    if (progress == 0.0f) {
      setPosition(tile->getPosition().x + random, tile->getPosition().y - hitHeight);
    }
    progress += 5 * _elapsed;
    this->setTexture(*texture);
    if (progress >= 1.f) {
      this->Delete();
    }
    return;
  }

  // Strike panel and leave
  if (GetDirection() == Direction::NONE) {
    this->Delete();
  }

  // If it did not hit a target this frame, move to next tile
  if (!hit) {
    if (GetTile()->GetX() == 6 && this->GetTeam() == Team::RED) { this->Delete(); }
    if (GetTile()->GetX() == 1 && this->GetTeam() == Team::BLUE) { this->Delete(); }

    // TODO: Vulcan moves too fast for the frame to attack the entities... See queuedSpells logic
    cooldown += _elapsed;
    if (cooldown >= COOLDOWN) {
      if (Move(GetDirection())) {
        AdoptNextTile();
        FinishMove();
        cooldown = 0;

        // Attack the current tile
        GetTile()->AffectEntities(this);
      }
      else {
        this->Delete();
      }
    }
  }
}

bool Vulcan::CanMoveTo(Battle::Tile* next) {
  return true;
}

void Vulcan::Attack(Character* _entity) {
  if (dynamic_cast<Gear*>(_entity)) {
    spawnGuard = true;
    contact = _entity;
    return;
  }

  if (_entity->Hit(this->GetHitboxProperties())) {
    hit = true;

    random = _entity->getLocalBounds().width / 2.0f;
    random *= rand() % 2 == 0 ? -1.0f : 1.0f;

    hitHeight = (float)(std::floor(_entity->GetHeight()));

    if (hitHeight > 0) {
      hitHeight = (float)(rand() % (int)hitHeight);
    }
  }

  if (hit) {
    AUDIO.Play(AudioType::HURT);
  }
}