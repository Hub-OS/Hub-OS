#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>

#include "bnSmartShader.h"
#include "bnSpriteSceneNode.h"

using sf::Drawable;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
using sf::Sprite;
using std::vector;

class Layers : vector<SpriteSceneNode*> {
public:
  int min;
  int max;

  void Insert(SpriteSceneNode* _layeredDrawable) {
    if (SpriteSceneNode->GetLayer() > max) {
      max = _layeredDrawable->GetLayer();
    } else if (_layeredDrawable->GetLayer() < min) {
      min = _layeredDrawable->GetLayer();
    }

    push_back(_layeredDrawable);
  }

  vector<SpriteSceneNode*> At(int _layer) {
    vector<SpriteSceneNode*> layer = vector<LayeredDrawable*>();
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
