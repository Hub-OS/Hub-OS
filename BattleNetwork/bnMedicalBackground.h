#pragma once
#include <SFML/Graphics.hpp>
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;
#include <vector>
using std::vector;

#include "bnAnimation.h"
#include "bnBackground.h"

/**
 * @class MedicalBackground
 * @author mav
 * @date 09/21/19
 * @brief pills animate
 */
class MedicalBackground : public Background
{
public:
  MedicalBackground();
  ~MedicalBackground();

  virtual void Update(float _elapsed);

private:
  float x, y;
  Animation animation;
  sf::Sprite dummy;
};
