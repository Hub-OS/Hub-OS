/*! \brief Virus background uses Background class to animate and scroll
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

class VirusBackground final : public Background
{
public:
  VirusBackground();
  ~VirusBackground();

  void Update(float _elapsed) override;
  void ScrollUp();
  void ScrollLeft();
  void SetScrollSpeed(const float speed);
private:
  float x, y;
  float progress;
  float speed;
  int lr{ 0 };
  int ud{ -1 };
};

