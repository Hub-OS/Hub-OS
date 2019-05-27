#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRockDebris.h"

#include <Swoosh/Ease.h>

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/cube/cube.animation"

RockDebris::RockDebris(RockDebris::Type type, double intensity) : type(type), intensity(intensity), duration(0.5*intensity), progress(0)
{
  SetLayer(0);
  this->setTexture(*TEXTURES.GetTexture(TextureType::MISC_CUBE));
  this->setScale(2.f, 2.f);
  rightRock = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  if (type == RockDebris::Type::LEFT_ICE || type == RockDebris::Type::RIGHT_ICE) {
    animation.SetAnimation("DEBRIS");

    animation.SetFrame((int)type-2, *this);
  }
  else {
    animation.SetAnimation("DEBRIS");

    animation.SetFrame((int)type, *this);
  }
}

void RockDebris::Update(float _elapsed) {
  progress += _elapsed;

<<<<<<< HEAD
  double alpha = swoosh::ease::wideParabola(progress, duration, 1.0);
  double beta = swoosh::ease::linear(progress, duration, 1.0);

  double posX = (beta * (tile->getPosition().x-(10.0*intensity))) + ((1.0 - beta)*tile->getPosition().x);
  double height = -(alpha * 60.0 * intensity);
  double posY = height + (beta * tile->getPosition().y) + ((1.0f - beta)*tile->getPosition().y);

=======
  // Alpha is a path along a parabola 
  // This represents an object going up and then coming back down
  double alpha = swoosh::ease::wideParabola(progress, duration, 1.0);
  
  // Beta is a path along a line
  // This represents the horizontal distance away from the origin
  double beta = swoosh::ease::linear(progress, duration, 1.0);
 
  // Interpolate the position 
  double posX = (beta * (tile->getPosition().x-(10.0*intensity))) + ((1.0 - beta)*tile->getPosition().x);
  
  // If intensity is 1.0, the peak will be -60.0 and the bottom will be 0
  double height = -(alpha * 60.0 * intensity);
  double posY = height + (beta * tile->getPosition().y) + ((1.0f - beta)*tile->getPosition().y);

  // Set the position of the left debris
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  if (type == RockDebris::Type::LEFT || type == RockDebris::Type::LEFT_ICE) {
    this->setPosition((float)posX, (float)posY);
  }

<<<<<<< HEAD
=======
  // Same as before but in the opposite direction for right pieces
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  posX = (beta * (tile->getPosition().x + (10.0*intensity))) + ((1.0 - beta)*tile->getPosition().x);

  if (type == RockDebris::Type::RIGHT || type == RockDebris::Type::RIGHT_ICE) {
    this->setPosition((float)posX, (float)posY);
  }

<<<<<<< HEAD

=======
  // Rotate by intensity to look sort of authentic
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  setRotation((float)(-std::min(1.0,(progress / duration))*90.0 * intensity));

  if (type == RockDebris::Type::RIGHT || type == RockDebris::Type::RIGHT_ICE) {
    setRotation(getRotation()*-1.0f);
  }

  // std::cout << "beta: " << beta << std::endl;

<<<<<<< HEAD
=======
  // After progress >= 1.0, the rock is on the ground
  // Wait for a total of duration*2.0 before removing the rock
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  if (progress >= duration*2.0f) {
    //progress = 0;

    //if (--intensity == 0) {
      this->Delete();
    //}
  } 

  Entity::Update(_elapsed);
}

RockDebris::~RockDebris()
{
}
