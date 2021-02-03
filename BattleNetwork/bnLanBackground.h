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
class LanBackground final : public IBackground<LanBackground> {
public:
  LanBackground();
  ~LanBackground();

  /**
   * @brief Texture wraps the frame and sets frame based on time
   * @param _elapsed in seconds
   */
  void Update(double _elapsed) override;

private:

  //Animation
  float x, y;
  float progress;

  Animation animation;
  sf::Sprite dummy;
};
