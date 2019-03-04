#pragma once
#include <SFML/Graphics.hpp>
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;
#include <vector>
using std::vector;

#include "bnBackground.h"

class LanBackground : public Background {
public:
  LanBackground(void);
  ~LanBackground(void);

  virtual void Update(float _elapsed);

private:

  //Animation
  float x, y;
  float progress;
};
