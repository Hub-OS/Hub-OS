#include "bnFishy.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Fishy::Fishy(Field* _field, Team _team, double speed) : Obstacle(field, team) {
  SetLayer(0);
  field = _field;
  hit = false;
  
  setTexture(TEXTURES.LoadTextureFromFile("resources/spells/fishy_temp.png"));
  setScale(2.f, 2.f);
  // why do we need to do this??
  // The super constructor is failing to set this value
  // TODO: find out why
  team = _team;

  Fishy::speed = speed;

  SetSlideTime(sf::seconds(0.1f));
  SetHealth(1);

  Audio().Play(AudioType::TOSS_ITEM_LITE, AudioPriority::lowest);

  Hit::Properties props;
  props.damage = 80;
  props.flags |= Hit::recoil | Hit::flinch;
  SetHitboxProperties(props);

  SetFloatShoe(true);
}

Fishy::~Fishy() {
}

void Fishy::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x - 40.0f, tile->getPosition().y + tileOffset.y - 120.0f);

  if (GetTile()->GetX() == 6) {
    Delete();
  }

  // Keep moving
  if (!IsSliding()) {
    SlideToTile(true);
    Move(GetDirection());
  }

  tile->AffectEntities(this);
}

bool Fishy::CanMoveTo(Battle::Tile* tile) {
  return true;
}


const bool Fishy::OnHit(const Hit::Properties props) {
  return true; // fishy blocks everything
}

void Fishy::OnDelete()
{
    Remove();
}

void Fishy::Attack(Character* _entity) {
  if (!hit) {

    hit = _entity->Hit(GetHitboxProperties());
    if (hit) {
      // end this
      Delete();
    }
  }
}