#pragma once
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include "bnLayered.h"

#include <iostream>

class SceneNode : public sf::Transformable, public sf::Drawable {
protected:
  mutable std::vector<SceneNode*> childNodes;
  SceneNode* parent;
  bool show;
  int layer;
  
public:
  SceneNode() {
    show = true;
    layer = 0;
  }

  virtual ~SceneNode() {

  }
  
  void SetLayer(int layer) {
    this->layer = layer;
  }
  
  const int GetLayer() const {
	  return this->layer;
  }

  void Hide()
  {
    show = false;
  }

  void Reveal()
  {
    show = true;
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!show) return;

    // combine the parent transform with the node's one
    //sf::Transform combinedTransform =this->getTransform();
    
    /*if (parent) {
      combinedTransform *= parent->getTransform();
    }*/

    //states.transform *= combinedTransform;

    std::sort(childNodes.begin(), childNodes.end(), [](SceneNode* a, SceneNode* b) { return (a->GetLayer() > b->GetLayer()); });

    // draw its children
    for (std::size_t i = 0; i < childNodes.size(); i++) {
      childNodes[i]->draw(target, states);
    }
  }

  void AddNode(SceneNode* child) { 
	  if (child == nullptr) return;  child->parent = this; childNodes.push_back(child); 
  }
  
  void RemoveNode(SceneNode* find) {
    if (find == nullptr) return;
    
    auto iter = std::remove_if(childNodes.begin(), childNodes.end(), [find](SceneNode *in) { return in == find; });

    childNodes.erase(iter, childNodes.end());
  }
};
