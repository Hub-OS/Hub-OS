#pragma once
#include "bnSceneNode.h"
#include "bnAnimation.h"
#include "bnResourceHandle.h"
#include "bnTextureResourceManager.h"

#include <cstdint>
#include <memory>
#include <array>

class Font : ResourceHandle
{
public:
  enum class Style : int {
    thick = 0,
    small = 1,
    tiny = 2,
    wide = 3,
    thin = 4,
    gradient = 5,
    gradient_gold = 6,
    gradient_green = 7,
    gradient_orange = 8, // a more-contrast gold
    gradient_tall = 9,
    battle = 10,
    size // don't use!
  } style;

private:
  static constexpr size_t style_sz = static_cast<size_t>(Style::size);
  static bool animationsLoaded;
  static std::array<Animation, style_sz> animationArray;

  uint32_t letter{ 'A' };
  sf::IntRect texcoords{};
  sf::IntRect letterATexcoords{};
  sf::Vector2f origin{};
  void ApplyStyle();
  const bool HasLowerCase(const Style& style);
public:
  Font(const Style& style);
  ~Font();

  const Style& GetStyle() const;
  void SetLetter(uint32_t letter);
  const sf::Texture& GetTexture() const;
  const sf::IntRect GetTextureCoords() const;
  const sf::Vector2f GetOrigin() const;
  const float GetLetterHeight() const;
  const float GetLetterWidth() const;
  const float GetWhiteSpaceWidth() const;
  const float GetLineHeight() const;
};

