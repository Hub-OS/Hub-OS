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

/*! \warning Legacy code that can and should be removed from the engine.
 *  \brief Originally the battle scene was split into layers before using scene nodes
 * 
 * Bottom layer was tiles, mid layer was sprites, and top layer was UI.
 * This is no longer used but lingers in the code base. Remove asap.
 */
class Layers : vector<SpriteSceneNode*> {
public:
  int min;
  int max;

  void Insert(SpriteSceneNode* node) {
    if (node->GetLayer() > max) {
      max = node->GetLayer();
    } else if (node->GetLayer() < min) {
      min = node->GetLayer();
    }

    push_back(node);
  }

  vector<SpriteSceneNode*> At(int _layer) {
    vector<SpriteSceneNode*> layer = vector<SpriteSceneNode*>();
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
