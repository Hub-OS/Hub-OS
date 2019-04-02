#pragma once
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>

class SceneNode : public sf::Transformable, public sf::Drawable {
protected:
  mutable std::vector<SceneNode*> childNodes;
  SceneNode* parent;
  bool show;
  int layer;
  
public:
  SceneNode();

  virtual ~SceneNode();
  
  void SetLayer(int layer);
  
  const int GetLayer() const;

  void Hide();

  void Reveal();

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  void AddNode(SceneNode* child);
  
  void RemoveNode(SceneNode* find);
};
