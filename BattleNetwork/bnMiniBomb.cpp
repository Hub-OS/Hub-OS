#include "bnMiniBomb.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnExplosion.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

MiniBomb::MiniBomb(Field* _field, Team _team, sf::Vector2f startPos, float _duration, int damage) : Spell(_field, _team) {
  SetLayer(0);
  cooldown = 0;
  damageCooldown = 0;

  auto texture = TEXTURES.GetTexture(TextureType::SPELL_MINI_BOMB);
  setTexture(*texture);
  setScale(2.f, 2.f);

  SetLayer(-1);

  random = 0;

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::flinch;
  this->SetHitboxProperties(props);

  arcDuration = _duration;
  arcProgress = 0;

  start = startPos;

  swoosh::game::setOrigin(*this, 0.5, 0.5);
  
  AUDIO.Play(AudioType::TOSS_ITEM);
}

MiniBomb::~MiniBomb(void) {
}

void MiniBomb::OnUpdate(float _elapsed) {
  arcProgress += _elapsed;

  int flash = (int)(arcProgress * 15);

  if (flash % 2 == 0) {
    this->EnableTileHighlight(true);
  }
  else {
    this->EnableTileHighlight(false);
  }

  double alpha = double(swoosh::ease::wideParabola(arcProgress, arcDuration, 1.0f));
  double beta = double(swoosh::ease::linear(arcProgress, arcDuration, 1.0f));

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
    this->Delete();
  }
}

bool MiniBomb::Move(Direction _direction) {
  return true;
}

void MiniBomb::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
  this->Delete();
}
