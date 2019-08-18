#include "bnSpriteSceneNode.h"

SpriteSceneNode::SpriteSceneNode() : SceneNode() {
  sprite = new sf::Sprite();
  allocatedSprite = true;
}

SpriteSceneNode::SpriteSceneNode(sf::Sprite& rhs) : SceneNode() {
  allocatedSprite = false;
  sprite = &rhs;
}

SpriteSceneNode::~SpriteSceneNode() {
  if(allocatedSprite) delete this->sprite;
}

void SpriteSceneNode::operator=(sf::Sprite& rhs) {
  if(allocatedSprite) delete this->sprite;

  sprite = &rhs;

  allocatedSprite = false;
}

SpriteSceneNode::operator sf::Sprite&() {
  return *sprite;
}

const sf::Texture* SpriteSceneNode::getTexture() const {
  return sprite->getTexture();
}

void SpriteSceneNode::setColor(sf::Color color) {
  sprite->setColor(color);
}

const sf::Color& SpriteSceneNode::getColor() const {
  return sprite->getColor();
}

const sf::IntRect& SpriteSceneNode::getTextureRect() {
  return sprite->getTextureRect();
}

void SpriteSceneNode::setTextureRect(sf::IntRect& rect) {
  sprite->setTextureRect(rect);
}

sf::FloatRect SpriteSceneNode::getLocalBounds() {
  return sprite->getLocalBounds();
}

void SpriteSceneNode::setTexture(const sf::Texture& texture, bool resetRect) {
  sprite->setTexture(texture, resetRect);
}

void SpriteSceneNode::SetShader(sf::Shader* _shader) {
  if (shader.Get() == _shader && _shader != nullptr) return;

  RevokeShader();

  if (_shader) {
    shader = *_shader;
  }
}

void SpriteSceneNode::SetShader(SmartShader& _shader) {
  RevokeShader();
  shader = _shader;
}

SmartShader& SpriteSceneNode::GetShader() {
  return shader;
}

void SpriteSceneNode::RevokeShader() {
  shader.Reset();
}

void SpriteSceneNode::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!show) return;

  // combine the parent transform with the node's one
  sf::Transform combinedTransform =this->getTransform();

  states.transform *= combinedTransform;

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
