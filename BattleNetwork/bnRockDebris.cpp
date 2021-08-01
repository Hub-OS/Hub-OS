#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnRockDebris.h"

#include <Swoosh/Ease.h>

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/cube/cube.animation"

RockDebris::RockDebris(RockDebris::Type type, double intensity) : 
  Artifact(), 
  type(type), 
  intensity(intensity), 
  duration(0.3), 
  progress(0)
{
  SetLayer(0);
  setTexture(Textures().GetTexture(TextureType::MISC_CUBE));
  setScale(2.f, 2.f);
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

void RockDebris::OnUpdate(double _elapsed) {
  progress += _elapsed;

  // Alpha is a path along a parabola 
  // This represents an object going up and then coming back down
  float alpha = swoosh::ease::wideParabola(progress, duration, 1.0);
  
  // Beta is a path along a line
  // This represents the horizontal distance away from the origin
  float beta = swoosh::ease::linear(progress, duration, 1.0);
 
  // Interpolate the position 
  float posX = (beta * (-(10.0*intensity)));
  
  // If intensity is 1.0, the peak will be -60.0 and the bottom will be 0
  float height = -(alpha * 60.0 * intensity);
  float posY = height;

  if (type == RockDebris::Type::RIGHT || type == RockDebris::Type::RIGHT_ICE) {
    // Same as before but in the opposite direction for right pieces
    posX = (beta * (10.0 * intensity));
  }

  Entity::drawOffset = { posX, posY };

  if (progress >= duration) {
      Delete();
  }
}

void RockDebris::OnDelete()
{
  Remove();
}

RockDebris::~RockDebris()
{
}
