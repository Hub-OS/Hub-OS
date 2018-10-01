#pragma once
#include <SFML/Graphics.hpp>
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;
#include <vector>
using std::vector;

#include "bnBackground.h"

class GraveyardBackground : public Background
{
public:
  GraveyardBackground();
  ~GraveyardBackground();

  virtual void Update(float _elapsed);

private:
  float x, y;
  float progress;
};

