#pragma once
#include "bnSceneNode.h"
#include "bnAnimation.h"
#include "bnTextureResourceManager.h"

#include <memory>

class Font
{
public:
  enum class Style : int {
    big = 0,
    small = 1,
    tiny = 2,
    wide = 3
  } style;

private:
  static Animation animation;
  char letter;
  sf::IntRect bounds;

  void ApplyStyle();

public:
  Font(const Style& style);
  ~Font();

  const Style& GetStyle() const;
  void SetLetter(char letter);
  const sf::Texture& GetTexture() const;
  const sf::IntRect GetBounds() const;
  const float GetLetterHeight() const;
  const float GetLetterWidth() const;
  const float GetWhiteSpaceWidth() const;
};

