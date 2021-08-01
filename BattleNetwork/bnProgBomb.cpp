#include "bnProgBomb.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnExplosion.h"
#include <cmath>
#include <Swoosh/Ease.h>

ProgBomb::ProgBomb(Team _team, sf::Vector2f startPos, float _duration) : Spell(_team) {
  SetLayer(0);

  setTexture(Textures().GetTexture(TextureType::SPELL_PROG_BOMB));
  setScale(2.f, 2.f);

  SetLayer(-1);

  random = 0;

  auto props = Hit::DefaultProperties;
  props.damage = 40;
  props.flags |= Hit::flash;
  SetHitboxProperties(props);

  arcDuration = _duration;
  arcProgress = 0;
  
  setOrigin(sf::Vector2f(19, 24) / 2.f);
  Audio().Play(AudioType::TOSS_ITEM);

  HighlightTile(Battle::Tile::Highlight::flash);
}

ProgBomb::~ProgBomb(void) {
}

void ProgBomb::OnSpawn(Battle::Tile& tile)
{
  start = tile.getPosition() - start;
}

void ProgBomb::OnUpdate(double _elapsed) {
  arcProgress += _elapsed;

  float alpha = double(swoosh::ease::wideParabola(arcProgress, arcDuration, 1.0));
  float beta = double(swoosh::ease::linear(arcProgress, arcDuration, 1.0));

  float posX = (1.0 - beta)*start.x;
  float height = -(alpha * 120.0);
  float posY = height  + ((1.0 - beta)*start.y);

  Entity::drawOffset = { posX, posY };
  setRotation(-static_cast<float>(arcProgress / arcDuration)*90.0f);
  Reveal();

  // When at the end of the arc
  if (arcProgress >= arcDuration) {
    // update tile to target tile 
    tile->AffectEntities(this);
    Artifact* explosion = new Explosion();
    GetField()->AddEntity(*explosion, tile->GetX(), tile->GetY());
    Delete();
  }
}

void ProgBomb::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
  Delete();
}

void ProgBomb::OnDelete()
{
  Remove();
}
