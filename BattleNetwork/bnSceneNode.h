#pragma once
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>

class SceneNode : public sf::Transformable {
protected:
  std::vector<SceneNode*> childNodes;
public:
  virtual void OnDraw(sf::RenderTexture& surface) = 0;
  void AddNode(SceneNode* child) { childNodes.push_back(child); }
  void RemoveNode(SceneNode* child) { /*std::find(childNodes.begin(), childNodes.end(), child);*/ }
};