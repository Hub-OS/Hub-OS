#include "bnSceneNode.h"

class SpriteSceneNode : public SceneNode {
private:
  int layer;
  int depth;
  SmartShader shader;
  sf::Sprite sprite;

public:
  SpriteSceneNode()
    : layer(0),
    depth(0) {
    this->AddSprite(&sprite);
  }

  SpriteSceneNode(const sf::Sprite& rhs) : sprite(rhs) {
    layer = 0;
    depth = 0;
    this->AddSprite(&sprite);

  }

  SpriteSceneNode(int _layer)
    : layer(_layer),
    depth(0){
    this->AddSprite(&sprite);
  }

  void operator=(const sf::Sprite& rhs) {
    sprite = rhs;
  }

  operator sf::Sprite&() {
    return sprite;
  }

  /*operator sf::Sprite() {
    return sprite;
  }*/

  const sf::Texture* getTexture() const {
    return sprite.getTexture();
  }

  void setColor(sf::Color color) {
    sprite.setColor(color);
  }

  const sf::Color& getColor() const {
    return sprite.getColor();
  }

  const sf::IntRect& getTextureRect() {
    return sprite.getTextureRect();
  }

  void setTextureRect(sf::IntRect& rect) {
    sprite.setTextureRect(rect);
  }

  sf::FloatRect getLocalBounds() {
    return sprite.getLocalBounds();
  }

  void setTexture(const sf::Texture& texture, bool resetRect = false) {
    sprite.setTexture(texture, resetRect);
  }

  void SetLayer(int _layer) {
    layer = _layer;
  }

  void SetShader(sf::Shader* _shader) {
    RevokeShader();

    if (_shader) {
      shader = *_shader;
    }

  }

  void SetShader(SmartShader& _shader) {
    RevokeShader();
    shader = _shader;
  }

  SmartShader& GetShader() {
    return shader;
  }

  void RevokeShader() {
    shader.Reset();
  }

  int GetLayer() const {
    return layer;
  }

  void SetDepth(int _depth) {
    depth = _depth;
  }

  int GetDepth() const {
    return depth;
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!show) return;

    // combine the parent transform with the node's one
    //sf::Transform combinedTransform =this->getTransform();
    
    //states.transform *= combinedTransform;

    std::sort(childNodes.begin(), childNodes.end(), [](SceneNode* a, SceneNode* b) { a->GetLayer() > b->GetLayer(); });

    // draw its children
    for (std::size_t i = 0; i < childNodes.size(); i++) {
      childNodes[i]->draw(target, states);
    }
  }
};
