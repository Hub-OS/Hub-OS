#pragma once
#ifdef BN_MOD_SUPPORT

#include "bnSpriteProxyNode.h"
#include "bnAnimation.h"
#include "bnAnimationComponent.h"
#include <vector>

struct SyncNode {
  std::shared_ptr<SpriteProxyNode> sprite;
  Animation animation;
  AnimationComponent::SyncItem syncItem;
};

class SyncNodeContainer {
private:
  std::vector<std::shared_ptr<SyncNode>> syncNodes;
public:
  std::shared_ptr<SyncNode> AddSyncNode(SceneNode& sceneNode, AnimationComponent& animationComponent, const std::string& point);
  void RemoveSyncNode(SceneNode& sceneNode, AnimationComponent& animationComponent, std::shared_ptr<SyncNode>& syncNode);
};

#endif
