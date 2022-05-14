#pragma once
#include <memory>
#include <functional>
#include <SFML/Graphics.hpp>
#include "bnCard.h"

struct PlayerSpecialButton {
  std::shared_ptr<sf::Texture> texture;
  unsigned int uses{}, maxUses{};
  std::function<bool(const std::vector<Battle::Card*>& input)> onUse{ nullptr };

  static const PlayerSpecialButton UltraFormVariant;
};