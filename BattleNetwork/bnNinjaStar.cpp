#include "bnNinjaStar.h"
#include "bnTile.h"

#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

NinjaStar::NinjaStar(Field* _field, Team _team, float _duration) : duration(_duration), Spell(_field, _team) {
  SetLayer(0);;
  
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_NINJA_STAR));
  
  // Swoosh util sets the texture origin to 50% x and 80% y
  swoosh::game::setOrigin(getSprite(), 0.5, 0.8);
  
  setScale(2.f, 2.f);

  progress = 0.0f;

  // Blue team starts from the right side of the screen
  if (GetTeam() == Team::blue) {
    start = sf::Vector2f(480.0f, 0.0f);
  }
  // Red ream starts from the left side of the screen
  else if (GetTeam() == Team::red) {
    start = sf::Vector2f(0.0f, 0.0f);
  }
  // Otherwise, unsupported
  else {
    Delete();
  }

  AUDIO.Play(AudioType::TOSS_ITEM_LITE);

  auto props = Hit::DefaultProperties;
  
  // Do 100 units of impact damage
  props.damage = 100;
  props.flags |= Hit::impact;
  SetHitboxProperties(props);
}

NinjaStar::~NinjaStar() {
}

void NinjaStar::OnUpdate(float _elapsed) {
  double beta = swoosh::ease::linear(progress, duration, 1.0);

  // interpolate from top of screen to the target tile spot
  double posX = (beta * tile->getPosition().x) + ((1.0f - beta)*start.x);
  double posY = (beta * tile->getPosition().y) + ((1.0f - beta)*start.y);

  setPosition((float)posX, (float)posY);

  // When at the end of the line
  if (progress >= duration) {
    // deal damage
    tile->AffectEntities(this);
  }
  
  // Let the star linger around for a bit before deleting
  if (progress > duration*2.0) {
    Delete();
  }

  progress += _elapsed;
}

bool NinjaStar::Move(Direction _direction) {
  return false;
}

void NinjaStar::Attack(Character* _entity) {
    _entity->Hit(GetHitboxProperties());
}

void NinjaStar::OnDelete()
{
  Remove();
}
