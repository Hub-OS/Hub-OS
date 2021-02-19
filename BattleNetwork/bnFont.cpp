#include "bnFont.h"

std::array<Animation, Font::style_sz> Font::animationArray{};
bool Font::animationsLoaded = false;

Font::Font(const Style& style) :
  style(style),
  letter('A')
{
  if (!Font::animationsLoaded) {
    Font::animationArray.fill(Animation("resources/fonts/fonts_compressed.animation"));

    for (auto& a : animationArray) {
      a.Load();
    }

    Font::animationsLoaded = true;
  }

  ApplyStyle();
  letterATexcoords = texcoords;
}

Font::~Font()
{
}

void Font::ApplyStyle()
{
  auto& animation = animationArray[static_cast<size_t>(style)];
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
    break;
  case Style::gradient:
    animName = "GRADIENT_";
    break;
  case Style::gradient_gold:
    animName = "GRADIENT_GOLD_";
    break;
  case Style::gradient_green:
    animName = "GRADIENT_GREEN_";
    break;
  case Style::gradient_tall:
    animName = "GRADIENT_TALL_";
    break;
  default:
    animName = "SMALL_";
    break;
  }

  std::string letterStr(1, letter);
  std::transform(letterStr.begin(), letterStr.end(), letterStr.begin(), ::toupper);

  if (letter != '"') {
    // some font cannot be lower-cased
    if (::islower(letter) && HasLowerCase(style)) {
      letterStr = "LOWER_" + letterStr;
    }

    animName = animName + letterStr;
  }
  else {
    animName = animName + "QUOTE";
  }

  FrameList list = animation.GetFrameList(animName);
  
  if (list.IsEmpty()) {
    list = animation.GetFrameList("SMALL_A");
  }
  
  auto& frame = list.GetFrame(0);
  texcoords = frame.subregion;
  origin = frame.origin;
}

const bool Font::HasLowerCase(const Style& style)
{
  switch (style) {
  case Style::thick:
    return true;
    break;
  case Style::small:
    return true;
    break;
  case Style::tiny:
    return true;
    break;
  case Style::wide:
    return false;
    break;
  case Style::thin:
    return true;
    break;
  case Style::gradient:
    return false;
    break;
  case Style::gradient_gold:
    return false;
    break;
  case Style::gradient_green:
    return false;
    break;
  case Style::gradient_tall:
    return false;
    break;
  default:
    break;
  }

  return true;
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

const sf::Vector2f Font::GetOrigin() const
{
  return origin;
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