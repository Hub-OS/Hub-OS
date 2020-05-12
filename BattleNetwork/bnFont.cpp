#include "bnFont.h"

void Font::ApplyStyle()
{
  std::string animName;

  switch (style) {
  case Style::big: 
    animName = "BIG_";
    break;
  case Style::small:
    animName = "SMALL_";
    break;
  case Style::tiny: 
    animName = "TINY_";
    break;
  case Style::wide:
    animName = "WIDE_";
    break;
  }

  std::string letterStr(1, letter);
  std::transform(letterStr.begin(), letterStr.end(), letterStr.begin(), ::toupper);

  if (letter != '"') {
    // WIDE font does not have lower case values
    if (::islower(letter) && style != Style::wide) {
      letterStr = "LOWER_" + letterStr;
    }

    animName += letterStr;
  }
  else {
    animName += "QUOTE";
  }

  animation.SetAnimation(animName);

  bounds = animation.GetFrameList(animation.GetAnimationString()).GetFrame(0).subregion;
}

Font::Font(const Style & style) : style(style), letter('A')
{
}

Font::~Font()
{
}

const Font::Style & Font::GetStyle() const
{
  return style;
}

void Font::SetLetter(char letter)
{
  auto prev = Font::letter;
  Font::letter = letter;

  if(letter != prev)
    ApplyStyle();
}

const sf::Texture & Font::GetTexture() const
{
  static std::shared_ptr<sf::Texture> texture = LOAD_TEXTURE(FONT);
  return *texture;
}

const sf::IntRect Font::GetBounds() const
{
  return bounds;
}

const float Font::GetLetterWidth() const
{
  return GetBounds().width;
}

const float Font::GetLetterHeight() const
{
  return GetBounds().height;
}

const float Font::GetWhiteSpaceWidth() const
{
  // these values are based on the letter 'A' since I didn't add whitespace font entries...

  float width = 6.0f;

  switch (style) {
  case Style::big:
    width = 6;
    break;
  case Style::small:
    width = 5;
    break;
  case Style::tiny:
    width = 5;
    break;
  case Style::wide:
    width = 7;
    break;
  }

  return width;
}

Animation Font::animation = Animation("resources/fonts/fonts_atlas.animation");