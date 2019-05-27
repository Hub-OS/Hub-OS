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
<<<<<<< HEAD
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
=======
  
  auto texture = TEXTURES.GetTexture(TextureType::SPELL_NINJA_STAR);
  setTexture(*texture);
  
  // Swoosh util sets the texture origin to 50% x and 80% y
  swoosh::game::setOrigin(*this, 0.5, 0.8);
  
  setScale(2.f, 2.f);

  progress = 0.0f;

  // Blue team starts from the right side of the screen
  if (GetTeam() == Team::BLUE) {
    start = sf::Vector2f(480.0f, 0.0f);
  }
  // Red ream starts from the left side of the screen
  else if (GetTeam() == Team::RED) {
    start = sf::Vector2f(0.0f, 0.0f);
  }
  // Otherwise, unsupported
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  else {
    this->Delete();
  }

  AUDIO.Play(AudioType::TOSS_ITEM_LITE);

  auto props = Hit::DefaultProperties;
<<<<<<< HEAD
=======
  
  // Do 100 units of impact damage
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  props.damage = 100;
  props.flags |= Hit::impact;
  this->SetHitboxProperties(props);
}

<<<<<<< HEAD
NinjaStar::~NinjaStar(void) {
=======
NinjaStar::~NinjaStar() {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void NinjaStar::Update(float _elapsed) {
  double beta = swoosh::ease::linear(progress, duration, 1.0);

<<<<<<< HEAD
=======
  // interpolate from top of screen to the target tile spot
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  double posX = (beta * tile->getPosition().x) + ((1.0f - beta)*start.x);
  double posY = (beta * tile->getPosition().y) + ((1.0f - beta)*start.y);

  setPosition((float)posX, (float)posY);

<<<<<<< HEAD
  // When at the end of the arc
  if (progress >= duration) {
    // update tile to target tile 
    tile->AffectEntities(this);
  }
  
=======
  // When at the end of the line
  if (progress >= duration) {
    // deal damage
    tile->AffectEntities(this);
  }
  
  // Let the star linger around for a bit before deleting
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  if (progress > duration*2.0) {
    this->Delete();
  }

  progress += _elapsed;
<<<<<<< HEAD

}

bool NinjaStar::Move(Direction _direction) {
  return true;
=======
}

bool NinjaStar::Move(Direction _direction) {
  return false;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void NinjaStar::Attack(Character* _entity) {
    _entity->Hit(GetHitboxProperties());
}
