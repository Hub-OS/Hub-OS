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
 * @class ACDCBackground
 * @author mav
 * @date 09/21/19
 * @brief ACDC background scrolls diagonally
 */
class ACDCBackground final  : public IBackground<ACDCBackground>
{
public:
  ACDCBackground();
  ~ACDCBackground();

  void Update(double _elapsed) override;

private:
  float x, y;
  Animation animation;
  sf::Sprite dummy;
};