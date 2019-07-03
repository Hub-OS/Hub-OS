#include <random>
#include <time.h>

#include "bnBuster.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnProgsMan.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnGuardHit.h"
#include "bnGear.h" 

#define COOLDOWN 40.0f/1000.0f

Buster::Buster(Field* _field, Team _team, bool _charged) : isCharged(_charged), Spell(_field, _team) {
  SetPassthrough(true);
  SetLayer(0);

  cooldown = 0;

  hit = false;
  progress = 0.0f;

  if (isCharged) {
    hitHeight = 20.0f;
  }
  else {
    hitHeight = 0.0f;
  }

  random = 0;

  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);

  if (_charged) {
    damage = 10;
    texture = TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT);
    animationComponent->Setup("resources/spells/spell_charged_bullet_hit.animation");
    animationComponent->Reload();
    animationComponent->SetAnimation("HIT");
  } else {
    damage = 1;
    texture = TEXTURES.GetTexture(TextureType::SPELL_BULLET_HIT);
    animationComponent->Setup("resources/spells/spell_bullet_hit.animation");
    animationComponent->Reload();
    animationComponent->SetAnimation("HIT");
  }
  setScale(2.f, 2.f);

  AUDIO.Play(AudioType::BUSTER_PEA, AudioPriority::HIGH);

  contact = nullptr;
  spawnGuard = false;
}

Buster::~Buster() {
}

void Buster::OnUpdate(float _elapsed) {
  if (spawnGuard) {
    field->AddEntity(*new GuardHit(field, contact), this->tile->GetX(), this->tile->GetY());
    spawnGuard = false;
    this->Delete();
    return;
  }

  if (hit) {
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

  GetTile()->AffectEntities(this);

  cooldown += _elapsed;
  if (cooldown >= COOLDOWN) {
    Move(GetDirection());
    cooldown = 0;
  }

}

bool Buster::Move(Direction _direction) {
  tile->RemoveEntityByID(this->GetID());
  Battle::Tile* next = nullptr;
  if (_direction == Direction::UP) {
    if (tile->GetY() - 1 > 0) {
      next = field->GetAt(tile->GetX(), tile->GetY() - 1);
      SetTile(next);
    }
  } else if (_direction == Direction::LEFT) {
    if (tile->GetX() - 1 > 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
      SetTile(next);
    } else {
      this->Delete();
      return false;
    }
  } else if (_direction == Direction::DOWN) {
    if (tile->GetY() + 1 <= (int)field->GetHeight()) {
      next = field->GetAt(tile->GetX(), tile->GetY() + 1);
      SetTile(next);
    }
  } else if (_direction == Direction::RIGHT) {
    if (tile->GetX() < (int)field->GetWidth()) {
      next = field->GetAt(tile->GetX() + 1, tile->GetY());
      SetTile(next);
    } else {
      this->Delete();
      return false;
    }
  }
  tile->AddEntity(*this);
  return true;
}

void Buster::Attack(Character* _entity) {
  if (hit) return;

  if (dynamic_cast<Gear*>(_entity)) {
    spawnGuard = true;
    contact = _entity;
    return;
  }

  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    auto props = Hit::DefaultProperties;
    props.flags = props.flags & ~Hit::recoil;
    props.damage = damage;

    _entity->Hit(props);

    if (!_entity->IsPassthrough()) {
      hit = true;  

      if (!isCharged) {
        random = _entity->getLocalBounds().width / 2.0f;
        random *= rand() % 2 == 0 ? -1.0f : 1.0f;

        hitHeight = (float)(std::floor(_entity->GetHitHeight()));

        if (hitHeight > 0) {
          hitHeight = (float)(rand() % (int)hitHeight);
        }
      }
    }
  }

  if (hit) {
    AUDIO.Play(AudioType::HURT);
  }
}