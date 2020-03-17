#pragma once

#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnSmartShader.h"

#include <cmath>

/**
 * @class Background
 * @author mav
 * @date 13/05/19
 * @brief Backgrounds must fill the entire screen by repeating the texture
 * 
 * Some are animated and need to support texture offsets while filling
 * the screen
 */

class Background : public sf::Drawable, public sf::Transformable
{
protected:
  /**
   * @brief Wraps the texture area seemlessly to simulate scrolling
   * @param _amount in normalized values [0,1]
   */
  void Wrap(sf::Vector2f _amount) {
    offset = _amount;

    offset.x = std::fmod(offset.x, 1.f);
    offset.y = std::fmod(offset.y, 1.f);

  }
 
  /**
   * @brief Offsets the texture area to do animated backgrounds from spritesheets
   * @param _offset in pixels from 0 -> textureSize
   */
  void TextureOffset(sf::Vector2f _offset) {
    textureRect.left = (int)_offset.x;
    textureRect.top  = (int)_offset.y;
  }

  void TextureOffset(const sf::IntRect& _offset) {
    textureRect.left = (int)_offset.left;
    textureRect.top = (int)_offset.top;
  }

  /**
   * @brief Given the single texture's size creates geometry to fill the screen
   * @param textureSize size of the texture you want to fill the screen with
   */
  void FillScreen(sf::Vector2u textureSize) {
    // How many times can the texture fit in (width,height)?
    unsigned occuranceX = (unsigned)std::ceil(((float)width / (float)textureSize.x));
    unsigned occuranceY = (unsigned)std::ceil(((float)height / (float)textureSize.y));

    occuranceX = std::max(occuranceX, (unsigned)1);
    occuranceY = std::max(occuranceY, (unsigned)1);

    vertices.resize(occuranceX * occuranceY * (unsigned)6);

    textureRect = sf::IntRect(0, 0, textureSize.x, textureSize.y);

    for (unsigned int i = 0; i < occuranceX; ++i) {
      for (unsigned int j = 0; j < occuranceY; ++j) {
        // get a pointer to the current tile's quad
        sf::Vertex* quad = &vertices[(i + j * occuranceX) * 6];

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

public:
  /**
   * @brief Constructs background with screen width and height. Fills the screen.
   * @param ref texture to fill
   * @param width of screen
   * @param height of screen
   */
  Background(const std::shared_ptr<sf::Texture>& ref, int width, int height) : offset(0,0), textureRect(0, 0, width, height), width(width), height(height), texture(ref) {
      texture = ref;
      texture->setRepeated(true);

      vertices.setPrimitiveType(sf::Triangles);

      sf::Vector2u textureSize = texture->getSize();

      FillScreen(textureSize);

      textureWrap = SHADERS.GetShader(ShaderType::TEXEL_TEXTURE_WRAP);
  }

  ~Background() { ;  }
  
  /**
   * @brief Implement for custom animated backgrounds
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed) = 0;

  /**
   * @brief Draw the animated background with applied values
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
  {
    // apply the transform
    states.transform *= getTransform();

    // apply the tileset texture
    states.texture = &*texture;

    sf::Vector2u size = texture->getSize();

    textureWrap->setUniform("x", (float)textureRect.left / (float)size.x);
    textureWrap->setUniform("y", (float)textureRect.top / (float)size.y);
    textureWrap->setUniform("w", (float)textureRect.width / (float)size.x);
    textureWrap->setUniform("h", (float)textureRect.height / (float)size.y);
    textureWrap->setUniform("offsetx", (float)(offset.x));
    textureWrap->setUniform("offsety", (float)(offset.y));

    states.shader = textureWrap;

    // draw the vertex array
    target.draw(vertices, states);
  }

  /**
   * @brief Apply color values to the background
   * @see bnUndernetBackground.h
   * @param color
   */
  void setColor(sf::Color color) {
    for (int i = 0; i < vertices.getVertexCount(); i++) {
      vertices[i].color = color;
    }
  }

protected:
  sf::VertexArray vertices; /*!< Geometry */
  std::shared_ptr<sf::Texture> texture; /*!< Texture aka spritesheet if animated */
  sf::IntRect textureRect; /*!< Frame of the animation if applicable */
  sf::Vector2f offset; /*!< Offset of the frame in pixels */
  int width, height; /*!< Dimensions of screen in pixels */
  sf::Shader* textureWrap; /*!< Scroll background values in normalized coord [0.0f, 1.0f] */
};

