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
 * @class GridBackground
 * @author mav
 * @date 01/05/19
 * @brief Simple wire background that animates and doesn't move
 */
class GridBackground final : public IBackground<GridBackground>
{
public:
  /**
   * @brief Loads animation data
   */
  GridBackground();
  ~GridBackground();

  /**
   * @brief Loops through the animation
   * @param _elapsed in seconds
   */
  void Update(double _elapsed) override;

private:
  float x, y; /*!< Unused */
  double progress; /*!< Used to progress animation and loop */
};

#pragma once
