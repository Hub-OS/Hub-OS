/*! \brief Custom scene node that acts as a proxy for sf::Sprites 
 * 
 * Used by sprites that need to conform to a Scene Node system
 * Every Entity instance is a SpriteSceneNode
 * 
 * SpriteSceneNodes are special scene nodes. They can have child nodes with a Z 
 * order in front of themselves. To achieve this, during the draw, the sprite and
 * its children are sorted by Z. When the sprite scene node is next to draw,
 * the sprite scene node draws the proxied sprite instead. This allows sprites to have
 * children sprites that can be drawn in front and behind of them for special effects
 * and UI.
 */
#pragma once
#include "bnSceneNode.h"
#include "bnSmartShader.h"

class SpriteProxyNode : public SceneNode {
private:
  bool allocatedSprite; /*!< Whether or not SpriteSceneNode owns the sprite pointer */
  mutable SmartShader shader; /*!< Sprites can have shaders attached to them */
  sf::Sprite* sprite; /*!< Reference to sprite behind proxy */
  std::shared_ptr<sf::Texture> textureRef; /*!< We want to intelligently keep track so the cache doesn't eat it*/
public:
  /**
    * \brief Construct new SpriteSceneNode
    * 
    * Owns sprite and allocatedSprite will be true
    */
  SpriteProxyNode();

  /**
  * \brief Construct new SpriteSceneNode with a texture
  *
  * Owns sprite and allocatedSprite will be true
  */
  SpriteProxyNode(const sf::Texture& texture);

  SpriteProxyNode(const SpriteProxyNode& rhs) = delete;
  
  /**
   * @brief Construct SpriteSceneNode from an existing sprite
   * @param rhs sprite to reference
   * 
   * Does not own sprite and allocatedSprite will be false
   */
  SpriteProxyNode(sf::Sprite& rhs);
  
  /**
   * @brief If allocatedSprite is true, deletes the sprite
   */
  virtual ~SpriteProxyNode();
  
  /**
   * @brief If allocatedSprite is true, deletes sprite and then points to rhs
   * @param rhs new sprite to proxy
   */
  void operator=(sf::Sprite& rhs);

  /**
   * @brief reference to sprite being proxied
   */
  const sf::Sprite& getSpriteConst() const;
  sf::Sprite& getSprite() const;

  /**
  * @brief returns the sprite's origin values
  */
  const sf::Vector2f getOrigin() const {
    return sprite->getOrigin();
  }

  /**
   * @brief Get current texture proxy
   * @return texture of sprite
   */
  const std::shared_ptr<sf::Texture> getTexture() const;

  /**
   * @brief Set sprite color proxy
   * @param color
   */
  void setColor(sf::Color color);

  /**
   * @brief Get sprite color proxy
   * @return sprite color
   */
  const sf::Color& getColor() const;

  /**
   * @brief Get sprite rect proxy
   * @return texture rect
   */
  const sf::IntRect& getTextureRect();

  /**
   * @brief Set sprite rect proxy
   * @param rect
   */
  void setTextureRect(sf::IntRect& rect);

  /**
   * @brief Get sprite local bounds proxy
   * @return sf::FloatRect of local bounds
   */
  sf::FloatRect getLocalBounds();

  /**
   * @brief Set sprite texture proxy
   * @param texture 
   * @param resetRect
   */
  void setTexture(const std::shared_ptr<sf::Texture>& texture, bool resetRect = true);

  /**
   * @brief Converts sf::Shader to SmartShader and attaches it.
   * @param _shader
   * 
   * Revokes the pre-existing shader
   */
  void SetShader(sf::Shader* _shader);

  /**
   * @brief Attaches shader
   * @param _shader
   * 
   * Revokes the pre-existing shader
   */
  void SetShader(SmartShader& _shader);

  /**
   * @brief Fetches the attached shader
   * @return SmartShader&
   */
  SmartShader& GetShader();

  /**
   * @brief Revokes the attached shader
   */
  void RevokeShader();
  
  /**
   * @brief Draws all child scene nodes with respect to parent transformations
   * @param target
   * @param states
   * 
   * SpriteSceneNodes can have child nodes in front of them. To achieve this,
   * we sort by Z and if this sprite node is to be drawn, drawns the proxy sprite
   */
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};
