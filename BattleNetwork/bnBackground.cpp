#include "bnBackground.h"


/**
  * @brief Constructs background with screen width and height. Fills the screen.
  * @param ref texture to fill
  * @param width of screen
  * @param height of screen
  */
Background::Background(const std::shared_ptr<sf::Texture>& ref, int width, int height) :
  offset(0, 0),
  textureRect(0, 0, width, height),
  width(width),
  height(height),
  texture(ref)
{
  if (texture) {
    texture = ref;
    texture->setRepeated(true);
    sf::Vector2u textureSize = texture->getSize();
    FillScreen(textureSize);
  }

  vertices.setPrimitiveType(sf::Triangles);

  textureWrap = Shaders().GetShader(ShaderType::TEXEL_TEXTURE_WRAP);
}

void Background::RecalculateColors() {
  for (int i = 0; i < vertices.getVertexCount(); i++) {
    sf::Color color = Background::color;
    color.r = (sf::Uint8)(color.r * mix);
    color.g = (sf::Uint8)(color.g * mix);
    color.b = (sf::Uint8)(color.b * mix);
    color.a = (sf::Uint8)(255*opacity);
    vertices[i].color = color;
  }
}

/**
  * @brief Wraps the texture area seemlessly to simulate scrolling
  * @param _amount in normalized values [0,1]
  */
void Background::Wrap(sf::Vector2f _amount) {
  offset = _amount;
  offset.x = std::fmod(offset.x, 1.f);
  offset.y = std::fmod(offset.y, 1.f);

}
 
/**
  * @brief Offsets the texture area to do animated backgrounds from spritesheets
  * @param _offset in pixels from 0 -> textureSize
  */
void Background::TextureOffset(sf::Vector2f _offset) {
  textureRect.left = (int)_offset.x;
  textureRect.top  = (int)_offset.y;
}

void Background::TextureOffset(const sf::IntRect& _offset) {
  textureRect.left = (int)_offset.left;
  textureRect.top = (int)_offset.top;
}

/**
  * @brief Given the single texture's size creates geometry to fill the screen
  * @param textureSize size of the texture you want to fill the screen with
  */
void Background::FillScreen(sf::Vector2u textureSize) {
  // How many times can the texture fit in (width,height)?
  unsigned occuranceX = (unsigned)std::ceil(((float)width / (float)textureSize.x));
  unsigned occuranceY = (unsigned)std::ceil(((float)height / (float)textureSize.y));

  occuranceX = std::max(occuranceX, (unsigned)1);
  occuranceY = std::max(occuranceY, (unsigned)1);

  vertices.resize(static_cast<size_t>(occuranceX * occuranceY * 6));

  textureRect = sf::IntRect(0, 0, textureSize.x, textureSize.y);

  for (unsigned int i = 0; i < occuranceX; ++i) {
    for (unsigned int j = 0; j < occuranceY; ++j) {
      // get a pointer to the current tile's quad
      sf::Vertex* quad = &vertices[static_cast<size_t>(i + j * occuranceX) * size_t(6)];

      // define its 4 corners
      quad[0].position = sf::Vector2f((float)(i * textureSize.x * 2), (float)((j + 1) * textureSize.y * 2));
      quad[1].position = sf::Vector2f((float)(i * textureSize.x * 2), (float)(j * textureSize.y * 2));
      quad[2].position = sf::Vector2f((float)((i + 1) * textureSize.x * 2), (float)((j + 1) * textureSize.y * 2));

      quad[3].position = sf::Vector2f((float)(i * textureSize.x * 2), (float)(j * textureSize.y * 2));
      quad[4].position = sf::Vector2f((float)((i + 1) * textureSize.x * 2), (float)((j + 1) * textureSize.y * 2));
      quad[5].position = sf::Vector2f((float)((i + 1) * textureSize.x * 2), (float)(j * textureSize.y * 2));

      // define its 4 texture coordinates
      quad[0].texCoords = sf::Vector2f(0, (float)textureSize.y);
      quad[1].texCoords = sf::Vector2f(0, 0);
      quad[2].texCoords = sf::Vector2f((float)textureSize.x, (float)textureSize.y);

      quad[3].texCoords = sf::Vector2f(0, 0);
      quad[4].texCoords = sf::Vector2f((float)textureSize.x, (float)textureSize.y);
      quad[5].texCoords = sf::Vector2f((float)textureSize.x, 0);
    }

    textureRect = sf::IntRect(0, 0, textureSize.x, textureSize.y);
  }
}

/**
  * @brief Draw the animated background with applied values
  * @param target
  * @param states
  */
void Background::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  // apply the transform
  states.transform *= getTransform();

  if (texture) {
    // apply the tileset texture
    states.texture = &(*texture);

    sf::Vector2u size = texture->getSize();

    if (textureWrap) {
      textureWrap->setUniform("x", (float)textureRect.left / (float)size.x);
      textureWrap->setUniform("y", (float)textureRect.top / (float)size.y);
      textureWrap->setUniform("w", (float)textureRect.width / (float)size.x);
      textureWrap->setUniform("h", (float)textureRect.height / (float)size.y);
      textureWrap->setUniform("offsetx", (float)(offset.x));
      textureWrap->setUniform("offsety", (float)(offset.y));
    }

    states.shader = textureWrap;
  }

  // draw the vertex array
  target.draw(vertices, states);
}

/**
  * @brief Apply color values to the background
  * @see bnUndernetBackground.h
  * @param color
  */
void Background::SetColor(sf::Color color) {
  Background::color = color;
  RecalculateColors();
}

void Background::SetMix(float tint) {
  Background::mix = tint;
  RecalculateColors();
}

void Background::SetOpacity(float opacity) {
  Background::opacity = opacity;
  RecalculateColors();
}

sf::Vector2f Background::GetOffset() {
  auto out = offset;
  out.x *= (float)textureRect.width;
  out.y *= (float)textureRect.height;
  return out;
}

/**
  * @brief Set a background offset. May get overridden by the background within the Update function
  * @param offset
  */
void Background::SetOffset(sf::Vector2f offset) {
  if (!texture) return;

  sf::Vector2u size = texture->getSize();
  offset.x /= (float)textureRect.width;
  offset.y /= (float)textureRect.height;
  Wrap(offset);
}

