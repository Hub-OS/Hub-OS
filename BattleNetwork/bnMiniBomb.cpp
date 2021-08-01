#include "bnMiniBomb.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnExplosion.h"
#include "bnMobMoveEffect.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

MiniBomb::MiniBomb(Team _team,sf::Vector2f startPos, float _duration, int damage) : Spell(_team) {
  SetLayer(0);

  HighlightTile(Battle::Tile::Highlight::flash);
  setTexture(Textures().GetTexture(TextureType::SPELL_MINI_BOMB));
  setScale(2.f, 2.f);

  SetLayer(-1);

  random = 0;

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::flash;
  SetHitboxProperties(props);

  arcDuration = _duration;
  arcProgress = 0;

  start = startPos;
  Entity::drawOffset = start;

  swoosh::game::setOrigin(getSprite(), 0.5, 0.5);
  
  Audio().Play(AudioType::TOSS_ITEM);
}

MiniBomb::~MiniBomb(void) {
}

void MiniBomb::OnUpdate(double _elapsed) {
  arcProgress += _elapsed;

  float alpha = (float)swoosh::ease::wideParabola(arcProgress, arcDuration, 1.0);
  float beta = (float)swoosh::ease::linear(arcProgress, arcDuration, 1.0);

  float posX = (1.0f - beta)*start.x;
  float height = -(alpha * 120.0);
  float posY = height + ((1.0f - beta)*start.y);

  Entity::drawOffset = { posX, posY };
  setRotation(-static_cast<float>(arcProgress / arcDuration)*90.0f);

  // When at the end of the arc
  if (arcProgress >= arcDuration) {
    // update tile to target tile 
    if (tile->IsWalkable()) {
      tile->AffectEntities(this);
      Artifact* explosion = new Explosion();
      GetField()->AddEntity(*explosion, tile->GetX(), tile->GetY());
      Delete();
    }
    else {
      auto fx = new MobMoveEffect();
      GetField()->AddEntity(*fx, *GetTile());
      Delete();
    }
  }
}

void MiniBomb::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
  Delete();
}

void MiniBomb::OnDelete()
{
    Remove();
}
