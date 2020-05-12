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
  Animation animation;
  char letter;
  sf::IntRect texcoords;

  void ApplyStyle();

public:
  Font(const Style& style);
  ~Font();

  const Style& GetStyle() const;
  void SetLetter(char letter);
  const sf::Texture& GetTexture() const;
  const sf::IntRect GetTextureCoords() const;
  const float GetLetterHeight() const;
  const float GetLetterWidth() const;
  const float GetWhiteSpaceWidth() const;
};

