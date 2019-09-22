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
 * @class RobotBackground
 * @author mav
 * @date 09/21/19
 * @brief robot animates and scrolls upward
 */
class RobotBackground : public Background
{
public:
  RobotBackground();
  ~RobotBackground();

  virtual void Update(float _elapsed);

private:
  float x, y;
  Animation animation;
  sf::Sprite dummy;
};
