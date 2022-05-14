#pragma once
#include <memory>
#include <functional>
#include <SFML/Graphics.hpp>
#include "bnCard.h"
#include "bnAnimation.h"

struct PlayerSpecialButton {
  using CallbackFn = std::function<bool(std::vector<Battle::Card::Properties*>)>;
  std::shared_ptr<sf::Texture> texture;
  std::shared_ptr<sf::Texture> preview;
  Animation anim;
  CallbackFn onUse{ nullptr };
  unsigned int maxUses{}, uses{};

  static const PlayerSpecialButton UltraFormVariant;
};