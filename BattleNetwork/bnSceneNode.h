#pragma once
#include <vector>
#include <algorithm>

class SceneNode {
protected:
  std::vector<SceneNode*> childNodes;
public:
  virtual void OnDraw() = 0;
  void AddNode(SceneNode* child) { childNodes.push_back(child); }
  void RemoveNode(SceneNode* child) { std::find(childNodes.begin(), childNodes.end(), child); }
};