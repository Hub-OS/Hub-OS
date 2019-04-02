#pragma once
#include "bnSceneNode.h"
#include "bnSmartShader.h"

class SpriteSceneNode : public SceneNode {
private:
  bool allocatedSprite;
  SmartShader shader;
  sf::Sprite* sprite;

public:
  SpriteSceneNode();
  SpriteSceneNode(sf::Sprite& rhs);
  
  virtual ~SpriteSceneNode();
  
  void operator=(sf::Sprite& rhs);

  operator sf::Sprite&();

  const sf::Texture* getTexture() const;

  void setColor(sf::Color color);

  const sf::Color& getColor() const;

  const sf::IntRect& getTextureRect();

  void setTextureRect(sf::IntRect& rect);

  sf::FloatRect getLocalBounds();

  void setTexture(const sf::Texture& texture, bool resetRect = false);

  void SetShader(sf::Shader* _shader);

  void SetShader(SmartShader& _shader);

  SmartShader& GetShader();

  void RevokeShader();

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};
