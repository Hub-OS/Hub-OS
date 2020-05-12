#include "bnText.h"

void Text::AddLetterQuad(sf::Vector2f position, const sf::Color & color, char letter) const
{
  float padding = 0.0; //was 1.0f

  font.SetLetter(letter);
  auto bounds = font.GetBounds();

  const sf::Texture& texture = font.GetTexture();
  float width = texture.getSize().x;
  float height = texture.getSize().y;
  
  float left = bounds.left - padding;
  float top = bounds.top - padding;
  float right = bounds.left + bounds.width + padding;
  float bottom = bounds.top + bounds.height + padding;

  float u = static_cast<float>(bounds.left / width);
  float v = static_cast<float>(bounds.top / height);

  auto uvRect = sf::FloatRect(u, v, bounds.width/width, bounds.height/height);

  float u1 = uvRect.left - padding;
  float v1 = uvRect.top  - padding;
  float u2 = uvRect.left + uvRect.width  + padding;
  float v2 = uvRect.top  + uvRect.height + padding;

  vertices.append(sf::Vertex(sf::Vector2f(position.x + left * top, position.y + top), color, sf::Vector2f(u1, v1)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + right* top, position.y + top), color, sf::Vector2f(u2, v1)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + left * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + left * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + right* top, position.y + top), color, sf::Vector2f(u2, v1)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + right* bottom, position.y + bottom), color, sf::Vector2f(u2, v2)));
}

void Text::UpdateGeometry() const
{
  if (!geometryDirty) return;

  vertices.clear();
  bounds = sf::FloatRect();

  if (message.empty()) return; // nothing to draw

  // Precompute the variables needed by the algorithm
  float whitespaceWidth = font.GetWhiteSpaceWidth();
  float letterSpacing = (whitespaceWidth / 3.f) * (Text::letterSpacing - 1.f);
  whitespaceWidth += letterSpacing;
  float lineSpacing = font.GetLetterHeight() * Text::lineSpacing;
  float x = 0.f;
  float y = 1.0f; // static_cast<float>(m_characterSize);

  float minX = 1.0f; // static_cast<float>(m_characterSize);
  float minY = 1.0f; // static_cast<float>(m_characterSize);
  float maxX = 0.f;
  float maxY = 0.f;

  for (auto&& letter : message) {

    // Handle special characters
    if ((letter == L' ') || (letter == L'\n') || (letter == L'\t'))
    {
      // Update the current bounds (min coordinates)
      minX = std::min(minX, x);
      minY = std::min(minY, y);

      switch (letter)
      {
      case L' ':  x += whitespaceWidth;     break;
      case L'\t': x += whitespaceWidth * 4; break;
      case L'\n': y += lineSpacing; x = 0;  break;
      }

      // Update the current bounds (max coordinates)
      maxX = std::max(maxX, x);
      maxY = std::max(maxY, y);

      // Next glyph, no need to create a quad for whitespace
      continue;
    }

    AddLetterQuad(sf::Vector2f(x, y), color, letter);

    x += font.GetLetterWidth() + letterSpacing;

    // Update bound values
    auto letterBounds = font.GetBounds();
    float left = letterBounds.left;
    float top = letterBounds.top;
    float right = letterBounds.left + letterBounds.width;
    float bottom = letterBounds.top + letterBounds.height;

    minX = std::min(minX, x + left * bottom);
    maxX = std::max(maxX, x + right * top);
    minY = std::min(minY, y + top);
    maxY = std::max(maxY, y + bottom);
  }

  // Update the bounding rectangle
  bounds.left = minX;
  bounds.top = minY;
  bounds.width = maxX - minX;
  bounds.height = maxY - minY;

  geometryDirty = false;
}

Text::Text(const std::string message, Font & font) : font(font), message(message), geometryDirty(true)
{
  letterSpacing = lineSpacing = 1.0f;
}

Text::Text(const Text & rhs) : font(rhs.font)
{
  letterSpacing = rhs.letterSpacing;
  lineSpacing = rhs.lineSpacing;
  message = rhs.message;
  color = rhs.color;
  bounds = rhs.bounds;
  vertices = rhs.vertices;
  geometryDirty = rhs.geometryDirty;
}

Text::~Text()
{
}

void Text::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
  UpdateGeometry();

  states.transform *= getTransform();
  states.texture = &font.GetTexture();

  target.draw(vertices, states);
}

void Text::SetString(const std::string message)
{
  geometryDirty |= Text::message == message;
  Text::message = message;
}

void Text::SetString(char c)
{
  geometryDirty |= Text::message == std::to_string(c);
  Text::message = std::to_string(c);
}

void Text::SetColor(const sf::Color & color)
{
  geometryDirty |= Text::color == color;

  Text::color = color;

  // Change vertex colors directly, no need to update whole geometry
  // (if geometry is updated anyway, we can skip this step)
  if (!geometryDirty)
  {
    for (std::size_t i = 0; i < vertices.getVertexCount(); ++i)
      vertices[i].color = Text::color;
  }
}

void Text::SetLetterSpacing(float spacing)
{
  geometryDirty |= letterSpacing == spacing;

  letterSpacing = spacing;
}

void Text::SetLineSpacing(float spacing)
{
  geometryDirty |= lineSpacing == spacing;

  lineSpacing = spacing;
}

const std::string & Text::GetString() const
{
  return message;
}

const Font & Text::GetFont() const
{
  return font;
}

const Font::Style & Text::GetStyle() const
{
  return font.GetStyle();
}

sf::FloatRect Text::GetLocalBounds() const
{
  UpdateGeometry();

  return bounds;
}

sf::FloatRect Text::GetWorldBounds() const
{
  return getTransform().transformRect(GetLocalBounds());
}