#pragma once
#include <SFML/Graphics.hpp>
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;
#include <vector>
using std::vector;

#include "bnBackground.h"

class GridBackground : public Background
{
public:
  GridBackground();
  ~GridBackground();

  virtual void Update(float _elapsed);

private:
  float x, y;
  float progress;
};

#pragma once
