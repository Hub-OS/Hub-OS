#pragma once

#include "bnShaderResourceManager.h"
#include "bnResourceHandle.h"
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

class Background : public sf::Drawable, public sf::Transformable, public ResourceHandle
{
protected:
  void RecalculateColors();

  /**
   * @brief Wraps the texture area seemlessly to simulate scrolling
   * @param _amount in normalized values [0,1]
   */
  void Wrap(sf::Vector2f _amount);
 
  /**
   * @brief Offsets the texture area to do animated backgrounds from spritesheets
   * @param _offset in pixels from 0 -> textureSize
   */
  void TextureOffset(sf::Vector2f _offset);
  void TextureOffset(const sf::IntRect& _offset);

  /**
   * @brief Given the single texture's size creates geometry to fill the screen
   * @param textureSize size of the texture you want to fill the screen with
   */
  void FillScreen(sf::Vector2u textureSize);

public:
  /**
   * @brief Constructs background with screen width and height. Fills the screen.
   * @param ref texture to fill
   * @param width of screen
   * @param height of screen
   */
  Background(const std::shared_ptr<sf::Texture>& ref, int width, int height);

  virtual ~Background() { ;  }
  virtual void Update(double _elapsed) = 0;

  /**
   * @brief Draw the animated background with applied values
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  /**
   * @brief Apply color values to the background
   * @see bnUndernetBackground.h
   * @param color
   */
  void SetColor(sf::Color color);
  void SetMix(float tint);
  void SetOpacity(float opacity);
  sf::Vector2f GetOffset();

  /**
   * @brief Set a background offset. May get overridden by the background within the Update function
   * @param offset
   */
  void SetOffset(sf::Vector2f offset);

protected:
  sf::VertexArray vertices; /*!< Geometry */
  std::shared_ptr<sf::Texture> texture{ nullptr }; /*!< Texture aka spritesheet if animated */
  sf::IntRect textureRect; /*!< Frame of the animation if applicable */
  sf::Vector2f offset; /*!< Offset of the frame in pixels */
  sf::Color color{ sf::Color::White };
  int width{}, height{}; /*!< Dimensions of screen in pixels */
  float mix{ 1. }; /*!< used to darken screen: 1.f is normal colors, 0.f is black */
  float opacity{ 1. };
  sf::Shader* textureWrap{ nullptr }; /*!< Scroll background values in normalized coord [0.0f, 1.0f] */
};

