#include "bnProgBomb.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnExplosion.h"
#include <cmath>
#include <Swoosh/Ease.h>

ProgBomb::ProgBomb(Field* _field, Team _team, sf::Vector2f startPos, float _duration) {
  SetLayer(0);
  cooldown = 0;
  damageCooldown = 0;
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_PROG_BOMB);
  setTexture(*texture);
  setScale(2.f, 2.f);

  progress = 0.0f;
  hitHeight = 0.0f;
  random = 0;

  auto props = Hit::DefaultProperties;
  props.damage = 40;
  this->SetHitboxProperties(props);

  arcDuration = _duration;
  arcProgress = 0;

  start = startPos;
  

  setOrigin(sf::Vector2f(19, 24) / 2.f);
  AUDIO.Play(AudioType::TOSS_ITEM);
}

ProgBomb::~ProgBomb(void) {
}

void ProgBomb::Update(float _elapsed) {
  arcProgress += _elapsed;

  int flash = (int)(arcProgress * 15);

  if (flash % 2 == 0) {
    this->EnableTileHighlight(true);
  }
  else {
    this->EnableTileHighlight(false);
  }

  double alpha = swoosh::ease::wideParabola(arcProgress, arcDuration, 1.0f);
  double beta  = swoosh::ease::linear(arcProgress, arcDuration, 1.0f);

  double posX = (beta * tile->getPosition().x) + ((1.0f - beta)*start.x);
  double height = -(alpha * 120.0);
  double posY = height + (beta * tile->getPosition().y) + ((1.0f - beta)*start.y);

  setPosition((float)posX, (float)posY);
  setRotation(-(arcProgress / arcDuration)*90.0f);

  // When at the end of the arc
  if (arcProgress >= arcDuration) {
    // update tile to target tile 
    tile->AffectEntities(this);
    Artifact* explosion = new Explosion(this->GetField(), this->GetTeam());
    this->GetField()->AddEntity(*explosion, this->tile->GetX(), this->tile->GetY());
    deleted = true;
  }
}

bool ProgBomb::Move(Direction _direction) {
  return true;
}

void ProgBomb::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
  deleted = true;
  return;
}
