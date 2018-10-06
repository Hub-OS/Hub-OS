#include <SFML/Graphics.hpp>
#include "bnSmartShader.h"
using sf::Drawable;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
using sf::Sprite;
#include <vector>
using std::vector;

class LayeredDrawable : public Sprite {
private:
  int layer;
  int depth;
  SmartShader shader;

public:
  LayeredDrawable(void)
    : layer(0),
    depth(0) {
  }

  LayeredDrawable(const sf::Sprite& rhs) {
    *((sf::Sprite*)this) = rhs;
    layer = 0;
    depth = 0;
  }

  LayeredDrawable(int _layer)
    : layer(_layer),
    depth(0){
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
