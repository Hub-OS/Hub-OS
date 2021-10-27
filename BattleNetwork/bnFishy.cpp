#include "bnFishy.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Fishy::Fishy(Team _team, double speed) : Obstacle(team) {
  SetLayer(0);
  hit = false;
  
  setTexture(Textures().LoadTextureFromFile("resources/spells/fishy_temp.png"));
  setScale(2.f, 2.f);

  Fishy::speed = speed;

  SetHealth(1);

  Audio().Play(AudioType::TOSS_ITEM_LITE, AudioPriority::lowest);

  Hit::Properties props;
  props.damage = 80;
  props.flags |= Hit::flinch | Hit::flash;
  SetHitboxProperties(props);
  SetFloatShoe(true);

  Entity::drawOffset = { -40.0f, -120.0f };
}

Fishy::~Fishy() {
}

void Fishy::OnUpdate(double _elapsed) {
  if (GetTile()->GetX() == 6) {
    Delete();
  }

  // Keep moving
  if (!IsSliding()) {
    Slide(GetTile() + GetMoveDirection(), frames(8), frames(0));
  }

  tile->AffectEntities(this);
}

bool Fishy::CanMoveTo(Battle::Tile* tile) {
  return true;
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