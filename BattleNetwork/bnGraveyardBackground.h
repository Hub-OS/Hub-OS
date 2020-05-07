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
 * @class GraveyardBackground
 * @author mav
 * @date 01/05/19
 * @brief spooky cloud background that animates and moves
 */
class GraveyardBackground final : public Background
{
public:
  GraveyardBackground();
  ~GraveyardBackground();

  void Update(float _elapsed) override;

private:
  float x, y;
  float progress;
};

