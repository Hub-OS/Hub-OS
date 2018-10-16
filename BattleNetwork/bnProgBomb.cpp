#include "bnProgBomb.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <cmath>

#define GRAVITY 9.81f

ProgBomb::ProgBomb(Field* _field, Team _team, Battle::Tile* _target, float _duration) {
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

  //animation.addFrame(0.3f, IntRect(0, 0, 19, 41));

  arcDuration = _duration*1000.0f;
  arcProgress = 0;
  target = _target;

  setOrigin(sf::Vector2f(19, 24) / 2.f);
  AUDIO.Play(AudioType::TOSS_ITEM);
}

ProgBomb::~ProgBomb(void) {
}

void ProgBomb::PrepareThrowPath() {
  velX = velY = 0;
  posX = tile->getPosition().x;
  posY = tile->getPosition().y;

  // Calculate projectile variables on init 
  velX = (target->getPosition().x - posX) / (arcDuration/1000.0f);
  velY = -1.0f * ((float) fabs(target->getPosition().y + 0.5f * (GRAVITY * (arcDuration/1000.0f) * (arcDuration/1000.0f)) - posY)) / (arcDuration / 1000.0f);
  arcProgress = 0;
}

void ProgBomb::Update(float _elapsed) {
  arcProgress += _elapsed * 1000.0f; // convert from s to ms

  setPosition(tile->getPosition().x + 5.f, tile->getPosition().y);
  setRotation(-(arcProgress / arcDuration)*45.0f);

  /* Have a target tile in mind
  calculate the time to the tile
  follow arc path to the tile from start height over time
  at time x (where x is the end of the arc), attack tile
  explode
  */
  sf::Vector2f pos(getPosition());

  pos.x = (velX * (arcProgress/1000.0f)) + pos.x;
  pos.y += 0.5f * GRAVITY * ((arcProgress / 1000.0f) * (arcProgress / 1000.0f)) + (velY * (arcProgress / 1000.0f));

  setPosition(pos);

  // When at the end of the arc
  if (arcProgress >= arcDuration) {
    // update tile to target tile 
    target->AffectEntities(this);
    deleted = true;
  }
}

bool ProgBomb::Move(Direction _direction) {
  return true;
}

void ProgBomb::Attack(Entity* _entity) {
  Player* isPlayer = dynamic_cast<Player*>(_entity);
  if (isPlayer) {
    isPlayer->Hit(20);
    deleted = true;
    return;
  }
}

vector<Drawable*> ProgBomb::GetMiscComponents() {
  return vector<Drawable*>();
}
