<<<<<<< HEAD
=======
/*! \brief Undernet background uses Background class to animate and scroll
 */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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
<<<<<<< HEAD
  float progress;
  int colorIndex;
  std::vector<sf::Color> colors;
  float colorProgress;
  sf::Time colorDuration;
=======
  float progress; /**< Animation progress */
  int colorIndex; /**< The current color to flash */
  std::vector<sf::Color> colors; /**< In the game, the undernet flashes colors */
  float colorProgress; /**< Counter until next flash */
  sf::Time colorDuration; /**< Duration inbetween color clashes */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};
