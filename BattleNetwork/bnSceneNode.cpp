#include "bnSceneNode.h"

SceneNode::SceneNode() {
  show = true;
  layer = 0;
}

SceneNode::~SceneNode() {

}

void SceneNode::SetLayer(int layer) {
  this->layer = layer;
}

const int SceneNode::GetLayer() const {
  return this->layer;
}

void SceneNode::Hide() {
  show = false;
}

void SceneNode::Reveal() {
  show = true;
}

const bool SceneNode::IsHidden() const {
  return !show;
}

const bool SceneNode::IsVisible() const {
  return show;
}

void SceneNode::draw(sf::RenderTarget& target, sf::RenderStates states) const {
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

void SceneNode::AddNode(SceneNode* child) { 
  if (child == nullptr) return;  child->parent = this; childNodes.push_back(child); 
}

void SceneNode::RemoveNode(SceneNode* find) {
  if (find == nullptr) return;

  auto iter = std::remove_if(childNodes.begin(), childNodes.end(), [find](SceneNode *in) { return in == find; }); 

  childNodes.erase(iter, childNodes.end());
}
