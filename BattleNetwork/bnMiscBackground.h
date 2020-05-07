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
 * @class MiscBackground
 * @author mav
 * @date 09/21/19
 * @brief misc bn4 background
 */
class MiscBackground final : public Background
{
public:
  MiscBackground();
  ~MiscBackground();

  void Update(float _elapsed) override;

private:
  float x, y;
  Animation animation;
  sf::Sprite dummy;
};
