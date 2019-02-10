#pragma once
#include <SFML/Graphics.hpp>
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;
#include <vector>
using std::vector;

#include "bnBackground.h"

class UndernetBackground : public Background
{
public:
  UndernetBackground();
  ~UndernetBackground();

  virtual void Update(float _elapsed);

private:
  float progress;
  int colorIndex;
  std::vector<sf::Color> colors;
  float colorProgress;
  sf::Time colorDuration;
};
