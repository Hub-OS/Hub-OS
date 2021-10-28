#include "bnVerticalCursor.h"
#include "../bnTextureResourceManager.h"
#include <Swoosh/Ease.h>
#include <cmath>

VerticalCursor::VerticalCursor() {
  setTexture(Textures().LoadFromFile("resources/ui/select_cursor.png"));
  setOrigin(0, getTextureRect().height / 2.0f);
}

void VerticalCursor::SetTarget(sf::Vector2f newTarget) {
  targetPos = newTarget;
}

void VerticalCursor::Update(float elapsed) {
  totalElapsed += (double)elapsed;

  auto bounce = std::sin((float)totalElapsed * 15.0f);
  auto x = targetPos.x + bounce;
  auto y = swoosh::ease::interpolate(elapsed * 12.5f, getPosition().y, targetPos.y);

  setPosition(x, y);
}
