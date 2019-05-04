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
 * @file bnGridBackground.h
 * @brief Simple wire background that animates and doesn't move
 */
class GridBackground : public Background
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
  virtual void Update(float _elapsed);

private:
  float x, y; /*!< Unused */
  float progress; /*!< Used to progress animation and loop */
};

#pragma once
