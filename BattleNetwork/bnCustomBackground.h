#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

#include "bnBackground.h"
#include "bnAnimation.h"

/*! \brief Simple scrolling animated background */
class CustomBackground final : public IBackground<CustomBackground> {
public:
  CustomBackground(const std::shared_ptr<sf::Texture>& texture, const Animation& animation, sf::Vector2f velocity);

  /**
   * @brief Texture wraps the frame and sets frame based on time
   * @param _elapsed in seconds
   */
  void Update(double _elapsed) override;

private:

  // Animation
  sf::Vector2f position;
  float progress;

  sf::Vector2f velocity;
  Animation animation;
  sf::Sprite dummy;
};
