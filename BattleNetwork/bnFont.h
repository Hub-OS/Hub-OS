#pragma once
#include "bnSceneNode.h"
#include "bnAnimation.h"
#include "bnResourceHandle.h"
#include "bnTextureResourceManager.h"

#include <memory>

class Font : ResourceHandle
{
public:
  enum class Style : int {
    thick = 0,
    small = 1,
    tiny = 2,
    wide = 3,
    thin = 4,
    gradient = 5
  } style;

private:
  Animation animation{};
  char letter{ 'A' };
  sf::IntRect texcoords{};
  sf::IntRect letterATexcoords{};
  sf::Vector2f origin{};
  void ApplyStyle();

public:
  Font(const Style& style);
  ~Font();

  const Style& GetStyle() const;
  void SetLetter(char letter);
  const sf::Texture& GetTexture() const;
  const sf::IntRect GetTextureCoords() const;
  const sf::Vector2f GetOrigin() const;
  const float GetLetterHeight() const;
  const float GetLetterWidth() const;
  const float GetWhiteSpaceWidth() const;
  const float GetLineHeight() const;
};

