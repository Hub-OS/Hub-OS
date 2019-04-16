#include "bnNinjaStar.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

NinjaStar::NinjaStar(Field* _field, Team _team, float _duration) : duration(_duration), Spell() {
  SetLayer(0);
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_NINJA_STAR);
  setTexture(*texture);
  swoosh::game::setOrigin(*this, 0.5, 0.8);
  setScale(2.f, 2.f);

  progress = 0.0f;
  hitHeight = 0.0f;

  if (GetTeam() == Team::BLUE) {
    start = sf::Vector2f(480.0f, 0.0f);
  }
  else if (GetTeam() == Team::RED) {
    start = sf::Vector2f(0.0f, 0.0f);
  }
  else {
    this->Delete();
  }

  AUDIO.Play(AudioType::TOSS_ITEM_LITE);

  auto props = Hit::DefaultProperties;
  props.damage = 100;
  props.flags |= Hit::impact;
  this->SetHitboxProperties(props);
}

NinjaStar::~NinjaStar(void) {
}

void NinjaStar::Update(float _elapsed) {
  double beta = swoosh::ease::linear(progress, duration, 1.0);

  double posX = (beta * tile->getPosition().x) + ((1.0f - beta)*start.x);
  double posY = (beta * tile->getPosition().y) + ((1.0f - beta)*start.y);

  setPosition((float)posX, (float)posY);

  // When at the end of the arc
  if (progress >= duration) {
    // update tile to target tile 
    tile->AffectEntities(this);
  }
  
  if (progress > duration*2.0) {
    this->Delete();
  }

  progress += _elapsed;

}

bool NinjaStar::Move(Direction _direction) {
  return true;
}

void NinjaStar::Attack(Character* _entity) {
    _entity->Hit(GetHitboxProperties());
}
