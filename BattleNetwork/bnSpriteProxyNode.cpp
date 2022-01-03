#include "bnSpriteProxyNode.h"
#include "bnLogger.h"

SpriteProxyNode::SpriteProxyNode() : SceneNode() {
  sprite = new sf::Sprite();
  allocatedSprite = true;
  show = true;
}

SpriteProxyNode::SpriteProxyNode(const sf::Texture & texture) : SceneNode()
{
  sprite = new sf::Sprite();
  textureRef = std::make_shared<sf::Texture>(texture);
  sprite->setTexture(*textureRef);
  allocatedSprite = true;
  show = true;
}

SpriteProxyNode::SpriteProxyNode(SpriteProxyNode&& rhs) noexcept
{
  std::swap(allocatedSprite, rhs.allocatedSprite);
  std::swap(childNodes, rhs.childNodes);
  std::swap(layer, rhs.layer);
  std::swap(parent, rhs.parent);
  std::swap(shader, rhs.shader);
  std::swap(show, rhs.show);
  std::swap(sprite, rhs.sprite);
  std::swap(textureRef, rhs.textureRef);
  std::swap(useParentShader, rhs.useParentShader);

  rhs.sprite = nullptr;
  rhs.allocatedSprite = false;
  rhs.textureRef.reset();

}

SpriteProxyNode::SpriteProxyNode(sf::Sprite& rhs) : SceneNode() {
  allocatedSprite = false;
  sprite = &rhs;
  show = true;
}

SpriteProxyNode::~SpriteProxyNode() {
  if(allocatedSprite) delete sprite;
}

void SpriteProxyNode::operator=(sf::Sprite& rhs) {
  if(allocatedSprite) delete sprite;

  sprite = &rhs;
  allocatedSprite = false;
}

const sf::Sprite & SpriteProxyNode::getSpriteConst() const
{
  return *sprite;
}

sf::Sprite& SpriteProxyNode::getSprite() const
{
  return *sprite;
}

const std::shared_ptr<sf::Texture> SpriteProxyNode::getTexture() const {
  return textureRef;
}

void SpriteProxyNode::setColor(sf::Color color) {
  sprite->setColor(color);
}

const sf::Color& SpriteProxyNode::getColor() const {
  return sprite->getColor();
}

void SpriteProxyNode::SetColorMode(ColorMode mode)
{
  colorMode = mode;
}

ColorMode SpriteProxyNode::GetColorMode() const
{
  return colorMode;
}

const sf::IntRect& SpriteProxyNode::getTextureRect() const {
  return sprite->getTextureRect();
}

void SpriteProxyNode::setTextureRect(const sf::IntRect& rect) {
  sprite->setTextureRect(rect);
}

sf::FloatRect SpriteProxyNode::getLocalBounds() const {
  return sprite->getLocalBounds();
}

void SpriteProxyNode::setTexture(const std::shared_ptr<sf::Texture> texture, bool resetRect) {
  if (!texture) return;

  textureRef = texture;
  sprite->setTexture(*texture, resetRect);
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

SmartShader& SpriteProxyNode::GetShader() const
{
  return shader;
}

void SpriteProxyNode::RevokeShader() {
  shader.Reset();
}

void SpriteProxyNode::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!show) return;

  // combine the parent transform with the node's one
  sf::Transform combinedTransform =getTransform();

  states.transform *= combinedTransform;

  sf::Shader* s = shader.Get();

  if (s) {
    states.shader = s;
  }
  else if(!IsUsingParentShader()) {
    states.shader = nullptr;
  }

  std::vector<SceneNode*> copies;
  copies.reserve(childNodes.size() + 1);

  for (auto& child : childNodes) {
    copies.push_back(child.get());
  }

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
