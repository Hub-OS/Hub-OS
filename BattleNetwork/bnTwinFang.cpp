#include "bnTwinFang.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

TwinFang::TwinFang(Team _team,Type _type, int damage) : 
  Spell(_team), 
  type(_type) {
  // Blades float over tiles 
  SetFloatShoe(true);
  SetAirShoe(true);

  switch(type) {
  case Type::ABOVE:
    spreadOffset = +40.0f;
    break;
  case Type::BELOW:
    spreadOffset = -40.0f;
    break;
  }

  SetLayer(0);

  setTexture(Textures().GetTexture(TextureType::SPELL_TWIN_FANG));
  setScale(2.f, 2.f);

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/spells/twin_fang.animation");
  animation->Load();
  animation->SetAnimation("DEFAULT");

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::flash;
  SetHitboxProperties(props);

  spreadOut = onEdgeOfMap = false;

  flickeroutTimer = 1; // in seconds
}

TwinFang::~TwinFang() {
}

void TwinFang::OnUpdate(double _elapsed) {
  auto height = 50.0f;

  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y - height + spreadOffset);

  if (!spreadOut) {
    // quickly spread out before firing across the map
    if (spreadOffset > 0) {
      spreadOffset -= 160.0f*static_cast<float>(_elapsed);

      if (spreadOffset <= 0) {
        spreadOut = true;
      }
    }
    else if (spreadOffset < 0) {
      spreadOffset += 160.0f*static_cast<float>(_elapsed);

      if (spreadOffset >= 0) {
        spreadOut = true;
      }
    }
  }
  else {
    if (onEdgeOfMap) {
      if (static_cast<int>(flickeroutTimer * 1000) % 3 == 0) {
        Hide();
      }
      else {
        Reveal();
      }

      flickeroutTimer -= _elapsed;

      if (flickeroutTimer < 0) {
        Delete();
      }
    }
    else {
      if (!IsSliding()) {
        if (!CanMoveTo(GetTile() + GetDirection())) {
          onEdgeOfMap = true;
        }
        else {
          Slide(GetTile() + GetDirection(), frames(4), frames(0));
        }
      }
    }
  }
  
  tile->AffectEntities(this);
}

// This attack flies through the air
bool TwinFang::CanMoveTo(Battle::Tile* tile) {
  return (bool)tile;
}

void TwinFang::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}

void TwinFang::OnDelete()
{
  Remove();
}

void TwinFang::OnSpawn(Battle::Tile& startTile)
{
  if (GetDirection() == Direction::left) {
    setScale(-1.f, 1.0);
  }
}
