#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "bnBackground.h"

using std::vector;
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;

#include "bnAnimation.h"

/*! \brief Moves diagonally and animates */
class LanBackground : public Background {
public:
  LanBackground();
  ~LanBackground();

  /**
   * @brief Texture wraps the frame and sets frame based on time
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);

private:

  //Animation
  float x, y;
  float progress;

  Animation animation;
  sf::Sprite dummy;
};
