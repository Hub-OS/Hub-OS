#pragma once
#include "../bnResourceHandle.h"
#include "../bnSpriteProxyNode.h"
#include <SFML/Graphics.hpp>
#include <memory>

// cursor for a vertical list
class VerticalCursor : public ResourceHandle, public SpriteProxyNode {
public:
  VerticalCursor();

  void SetTarget(sf::Vector2f target);
  void Update(float elapsed);
private:
  sf::Vector2f targetPos;
  double totalElapsed{};
};
