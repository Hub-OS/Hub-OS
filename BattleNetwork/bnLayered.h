#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include "bnSmartShader.h"
#include "bnSceneNode.h"

using sf::Drawable;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
using sf::Sprite;
using std::vector;

// TODO: rename to SpriteNode and move to own file
class LayeredDrawable : public SceneNode {
private:
  int layer;
  int depth;
  SmartShader shader;
  sf::Sprite sprite;

public:
  LayeredDrawable(void)
    : layer(0),
    depth(0) {
    this->AddSprite(sprite);
  }

  LayeredDrawable(const sf::Sprite& rhs) : sprite(rhs) {
    layer = 0;
    depth = 0;
    this->AddSprite(sprite);

  }

  LayeredDrawable(int _layer)
    : layer(_layer),
    depth(0){
    this->AddSprite(sprite);
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
};

class Layers : vector<LayeredDrawable*> {
public:
  int min;
  int max;

  void Insert(LayeredDrawable* _layeredDrawable) {
    if (_layeredDrawable->GetLayer() > max) {
      max = _layeredDrawable->GetLayer();
    } else if (_layeredDrawable->GetLayer() < min) {
      min = _layeredDrawable->GetLayer();
    }

    push_back(_layeredDrawable);
  }

  vector<LayeredDrawable*> At(int _layer) {
    vector<LayeredDrawable*> layer = vector<LayeredDrawable*>();
    auto it = begin();
    for (it; it != end(); ++it) {
      if ((*it)->GetLayer() == _layer) {
        layer.push_back(*it);
      }
    }
    return layer;
  }

  void Clear() {
    clear();
  }

  Layers() {
    min = 0;
    max = 0;
  }
};

class Overlay : public vector<Drawable*> {
public:
  void Push(Drawable* _component) {
    push_back(_component);
  }

  void Clear() {
    clear();
  }

  Overlay() {
  }
};

class Underlay : public Overlay {
public:
  Underlay() {
  }
};
