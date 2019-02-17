#pragma once
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>

class SceneNode : public sf::Transformable, public sf::Drawable {
private:
  std::vector<SceneNode*> childNodes;
  SceneNode* parent;

public:
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // combine the parent transform with the node's one
    sf::Transform combinedTransform =this->getTransform();
    
    if (parent) {
      combinedTransform *= parent->getTransform();
    }

    states.transform *= combinedTransform;

      // draw its children
    for (std::size_t i = 0; i < childNodes.size(); ++i) {
      childNodes[i]->draw(target, states);
    }
  }

  void AddNode(SceneNode& child) { child.parent = this; childNodes.push_back(&child); }
  void RemoveNode(SceneNode& find) {
    auto iter = std::remove_if(childNodes.begin(), childNodes.end(), [&find](SceneNode *in) { return in == &find; });
    (*iter)->parent = nullptr;
    
    childNodes.erase(iter, childNodes.end());
  }
};