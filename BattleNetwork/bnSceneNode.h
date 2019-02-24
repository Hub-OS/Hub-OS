#pragma once
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>

class SceneNode : public sf::Transformable, public sf::Drawable {
private:
  std::vector<SceneNode*> childNodes;
  std::vector<sf::Sprite*> sprites;
  SceneNode* parent;

public:
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // combine the parent transform with the node's one
    sf::Transform combinedTransform =this->getTransform();
    
    if (parent) {
      combinedTransform *= parent->getTransform();
    }

    states.transform *= combinedTransform;

    // Draw sprites
    for (std::size_t i = 0; i < sprites.size(); ++i) {
     target.draw(*sprites[i], states);
    }

    // draw its children
    for (std::size_t i = 0; i < childNodes.size(); ++i) {
      childNodes[i]->draw(target, states);
    }
  }

  void AddNode(SceneNode* child) { if (child == nullptr) return;  child->parent = this; childNodes.push_back(child); }
  void RemoveNode(SceneNode* find) {
    if (find == nullptr) return;

    auto iter = std::remove_if(childNodes.begin(), childNodes.end(), [&find](SceneNode *in) { return in == find; });
    (*iter)->parent = nullptr;
    
    childNodes.erase(iter, childNodes.end());
  }

  void AddSprite(sf::Sprite& sprite) { sprites.push_back(&sprite); }

  void RemoveSprite(sf::Sprite& find) {
    auto iter = std::remove_if(sprites.begin(), sprites.end(), [&find](sf::Sprite *in) { return in == &find; });

    sprites.erase(iter, sprites.end());
  }
};