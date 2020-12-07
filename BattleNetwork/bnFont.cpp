#include "bnFont.h"

void Font::ApplyStyle()
{
  std::string animName;

  switch (style) {
  case Style::thick: 
    animName = "THICK_";
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
  case Style::thin:
    animName = "THIN_";
  }

  std::string letterStr(1, letter);
  std::transform(letterStr.begin(), letterStr.end(), letterStr.begin(), ::toupper);

  if (letter != '"') {
    // WIDE font does not have lower case values
    if (::islower(letter) && style != Style::wide) {
      letterStr = "LOWER_" + letterStr;
    }

    animName = animName + letterStr;
  }
  else {
    animName = animName + "QUOTE";
  }

  auto list = animation.GetFrameList(animName);
  
  if (list.IsEmpty()) {
    list = animation.GetFrameList("SMALL_A");
  }
   
  texcoords = list.GetFrame(0).subregion;
}

Font::Font(const Style & style) 
  : style(style), letter('A'), animation("resources/fonts/fonts_compressed.animation")
{
  ApplyStyle();
  letterATexcoords = texcoords;
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

const sf::IntRect Font::GetTextureCoords() const
{
  return texcoords;
}

const float Font::GetLetterWidth() const
{
  return static_cast<float>(GetTextureCoords().width);
}

const float Font::GetLetterHeight() const
{
  return static_cast<float>(GetTextureCoords().height)+2.0f; // +2 for letter 'j'
}

const float Font::GetWhiteSpaceWidth() const
{
  // these values are based on the letter 'A' since I didn't add whitespace font entries...
  return static_cast<float>(letterATexcoords.width);
}

const float Font::GetLineHeight() const {
  // values are based on the letter 'A' since .animation doesn't have meta data for this type of use case
  return static_cast<float>(letterATexcoords.height);
}