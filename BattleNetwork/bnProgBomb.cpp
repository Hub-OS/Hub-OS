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
  props.flags |= Hit::flinch;
  SetHitboxProperties(props);

  arcDuration = _duration;
  arcProgress = 0;

  start = startPos;
  

  setOrigin(sf::Vector2f(19, 24) / 2.f);
  Audio().Play(AudioType::TOSS_ITEM);

  HighlightTile(Battle::Tile::Highlight::flash);
}

ProgBomb::~ProgBomb(void) {
}

void ProgBomb::OnUpdate(double _elapsed) {
  arcProgress += _elapsed;

  double alpha = double(swoosh::ease::wideParabola(arcProgress, arcDuration, 1.0));
  double beta = double(swoosh::ease::linear(arcProgress, arcDuration, 1.0));

  double posX = (beta * tile->getPosition().x) + ((1.0 - beta)*start.x);
  double height = -(alpha * 120.0);
  double posY = height + (beta * tile->getPosition().y) + ((1.0 - beta)*start.y);

  setPosition(static_cast<float>(posX), static_cast<float>(posY));
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
