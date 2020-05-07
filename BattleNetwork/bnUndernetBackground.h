/*! \brief Undernet background uses Background class to animate and scroll
 */

#pragma once
#include <SFML/Graphics.hpp>
using sf::Texture;
using sf::Sprite;
using sf::IntRect;
using sf::Drawable;
#include <vector>
using std::vector;

#include "bnBackground.h"

class UndernetBackground final : public Background
{
public:
  UndernetBackground();
  ~UndernetBackground();

  void Update(float _elapsed) override;

private:
  float progress; /**< Animation progress */
  int colorIndex; /**< The current color to flash */
  std::vector<sf::Color> colors; /**< In the game, the undernet flashes colors */
  float colorProgress; /**< Counter until next flash */
  sf::Time colorDuration; /**< Duration inbetween color clashes */
};
