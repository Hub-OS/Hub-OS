#pragma once
#include <SFML/Graphics.hpp>
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;
#include <vector>
using std::vector;

#include "bnBackground.h"

/**
 * @class SecretBackground
 * @author mav
 * @date 06/21/10
 * @brief secret area background
 */
class SecretBackground final : public Background
{
public:
  SecretBackground();
  ~SecretBackground();

  void Update(float _elapsed) override;

private:
  float x, y;
};
