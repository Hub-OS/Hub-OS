#include "bnSpriteProxyNode.h"

SpriteProxyNode::SpriteProxyNode() : SceneNode() {
  sprite = new sf::Sprite();
  allocatedSprite = true;
}

SpriteProxyNode::SpriteProxyNode(sf::Sprite& rhs) : SceneNode() {
  allocatedSprite = false;
  sprite = &rhs;
}

SpriteProxyNode::~SpriteProxyNode() {
  if(allocatedSprite) delete this->sprite;
}

void SpriteProxyNode::operator=(sf::Sprite& rhs) {
  if(allocatedSprite) delete this->sprite;

  sprite = &rhs;

  allocatedSprite = false;
}

SpriteProxyNode::operator sf::Sprite&() {
  return *sprite;
}

const sf::Sprite & SpriteProxyNode::getSprite() const
{
  return *sprite;
}

const sf::Texture* SpriteProxyNode::getTexture() const {
  return sprite->getTexture();
}

void SpriteProxyNode::setColor(sf::Color color) {
  sprite->setColor(color);
}

const sf::Color& SpriteProxyNode::getColor() const {
  return sprite->getColor();
}

const sf::IntRect& SpriteProxyNode::getTextureRect() {
  return sprite->getTextureRect();
}

void SpriteProxyNode::setTextureRect(sf::IntRect& rect) {
  sprite->setTextureRect(rect);
}

sf::FloatRect SpriteProxyNode::getLocalBounds() {
  return sprite->getLocalBounds();
}

void SpriteProxyNode::setTexture(const sf::Texture& texture, bool resetRect) {
  sprite->setTexture(texture, resetRect);
}

void SpriteProxyNode::SetShader(sf::Shader* _shader) {
  if (shader.Get() == _shader && _shader != nullptr) return;

  RevokeShader();

  if (_shader) {
    shader = *_shader;
  }
}

void SpriteProxyNode::SetShader(SmartShader& _shader) {
  RevokeShader();
  shader = _shader;
}

SmartShader& SpriteProxyNode::GetShader() {
  return shader;
}

void SpriteProxyNode::RevokeShader() {
  shader.Reset();
}

void SpriteProxyNode::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!show) return;

  // combine the parent transform with the node's one
  sf::Transform combinedTransform =this->getTransform();

  states.transform *= combinedTransform;

  const sf::Shader* s = const_cast<const sf::Shader*>(shader.Get());

  if (s) {
    states.shader = s;
  }
  else if(!IsUsingParentShader()){
    states.shader = nullptr;
  }

  std::vector<SceneNode*> copies = this->childNodes;
  copies.push_back((SceneNode*)this);

  std::sort(copies.begin(), copies.end(), [](SceneNode* a, SceneNode* b) { return (a->GetLayer() > b->GetLayer()); });

  // draw its children
  for (std::size_t i = 0; i < copies.size(); i++) {
    // If it's time to draw our scene node, we draw the proxy sprite
    if(copies[i] == this) {
      target.draw(*sprite, states);
    } else {
      copies[i]->draw(target, states);
    }
  }
}
