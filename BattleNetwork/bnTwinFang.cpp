#include "bnTwinFang.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

TwinFang::TwinFang(Field* _field, Team _team, Type _type, int damage) : Spell(_field, _team), type(_type) {
  // Blades float over tiles 
  this->SetFloatShoe(true);
  this->SetAirShoe(true);

  switch(type) {
  case Type::ABOVE:
    spreadOffset = +40.0f;
    break;
  case Type::BELOW:
    spreadOffset = -40.0f;
    break;
  case Type::ABOVE_DUD:
    spreadOffset = +40.0f;
    break;
  case Type::BELOW_DUD:
    spreadOffset = -40.0f;
    break;
  }

  SetLayer(0);

  auto texture = TEXTURES.GetTexture(TextureType::SPELL_TWIN_FANG);
  setTexture(*texture);
  setScale(2.f, 2.f);

  // Twin fang move from tile to tile in 4 frames
  // Adjust by speed factor
  this->SetSlideTime(sf::seconds(1.0f / 15.0f));

  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->Setup("resources/spells/twin_fang.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT");

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::flinch;
  this->SetHitboxProperties(props);

  spreadOut = onEdgeOfMap = false;

  flickeroutTimer = 1; // in seconds
}

TwinFang::~TwinFang() {
}

void TwinFang::OnUpdate(float _elapsed) {
  auto height = 50.0f;

  if (type == Type::ABOVE_DUD) {
    height += 40.0f;
  }
  else if (type == Type::BELOW_DUD) {
    height -= 40.0f;
  }

  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y - height + spreadOffset);

  if (!spreadOut) {
    // quickly spread out before firing across the map
    if (spreadOffset > 0) {
      spreadOffset -= 160.0f*_elapsed;

      if (spreadOffset <= 0) {
        spreadOut = true;
      }
    }
    else if (spreadOffset < 0) {
      spreadOffset += 160.0f*_elapsed;

      if (spreadOffset >= 0) {
        spreadOut = true;
      }
    }
  }
  else {
    if (onEdgeOfMap) {
      if (int(flickeroutTimer * 1000) % 3 == 0) {
        this->Hide();
      }
      else {
        this->Reveal();
      }

      flickeroutTimer -= _elapsed;

      if (flickeroutTimer < 0) {
        this->Delete();
      }
    }
    else {
      // Always glide
      if (!IsSliding()) {
        this->SlideToTile(true);

        // Keep moving
        this->Move(this->GetDirection());

        if (this->GetNextTile() == nullptr) {
          // we hit the wall
          onEdgeOfMap = true;
        }

      }
    }
  }

  if (type != Type::ABOVE_DUD && type != Type::BELOW_DUD) {
    tile->AffectEntities(this);
  }
}

// This attack flies through the air
bool TwinFang::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void TwinFang::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}