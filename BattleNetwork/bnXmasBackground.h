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
 * @class XmasBackground
 * @author mav
 * @date 11/24/20
 * @brief Repurposed background from an excellent fangame
 */
class XmasBackground final  : public Background
{
public:
  XmasBackground();
  ~XmasBackground();

  void Update(float _elapsed) override;

private:
  float x, y;
  Animation animation;
  sf::Sprite dummy;
};