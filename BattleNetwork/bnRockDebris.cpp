#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnRockDebris.h"

#include <Swoosh/Ease.h>

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/cube/cube.animation"

RockDebris::RockDebris(RockDebris::Type type, double intensity) : Artifact(nullptr), type(type), intensity(intensity), duration(0.5), progress(0)
{
  SetLayer(0);
  this->setTexture(TEXTURES.GetTexture(TextureType::MISC_CUBE));
  this->setScale(2.f, 2.f);
  rightRock = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  if (type == RockDebris::Type::LEFT_ICE || type == RockDebris::Type::RIGHT_ICE) {
    animation.SetAnimation("ICE_DEBRIS");

    animation.SetFrame((int)type-2, getSprite());
  }
  else {
    animation.SetAnimation("DEBRIS");

    animation.SetFrame((int)type, getSprite());
  }
}

void RockDebris::OnUpdate(float _elapsed) {
  progress += _elapsed;

  // Alpha is a path along a parabola 
  // This represents an object going up and then coming back down
  double alpha = swoosh::ease::wideParabola(progress, duration, 1.0);
  
  // Beta is a path along a line
  // This represents the horizontal distance away from the origin
  double beta = swoosh::ease::linear(progress, duration, 1.0);
 
  // Interpolate the position 
  double posX = (beta * (GetTile()->getPosition().x-(10.0*intensity))) + ((1.0 - beta)*GetTile()->getPosition().x);
  
  // If intensity is 1.0, the peak will be -60.0 and the bottom will be 0
  double height = -(alpha * 60.0 * intensity);
  double posY = height + (beta * GetTile()->getPosition().y) + ((1.0f - beta)*GetTile()->getPosition().y);

  // Set the position of the left debris
  if (type == RockDebris::Type::LEFT || type == RockDebris::Type::LEFT_ICE) {
    this->setPosition((float)posX, (float)posY);
  }

  // Same as before but in the opposite direction for right pieces
  posX = (beta * (GetTile()->getPosition().x + (10.0*intensity))) + ((1.0 - beta)*GetTile()->getPosition().x);

  if (type == RockDebris::Type::RIGHT || type == RockDebris::Type::RIGHT_ICE) {
    this->setPosition((float)posX, (float)posY);
  }

  if (progress >= duration) {
      this->Delete();
  }
}

RockDebris::~RockDebris()
{
}
