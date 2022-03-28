#ifdef ONB_MOD_SUPPORT
#include "bnSyncNode.h"

#include <algorithm>

std::shared_ptr<SyncNode> SyncNodeContainer::AddSyncNode(SceneNode& sceneNode, AnimationComponent& animationComponent, const std::string& point)
{
  auto syncNode = std::make_shared<SyncNode>();
  syncNode->sprite = std::make_shared<SpriteProxyNode>();
  syncNode->syncItem.anim = &syncNode->animation;
  syncNode->syncItem.node = syncNode->sprite;
  syncNode->syncItem.point = point;

  syncNodes.push_back(syncNode);
  sceneNode.AddNode(syncNode->sprite);
  animationComponent.AddToSyncList(syncNode->syncItem);

  return syncNode;
}

void SyncNodeContainer::RemoveSyncNode(SceneNode& sceneNode, AnimationComponent& animationComponent, std::shared_ptr<SyncNode>& syncNode)
{
  animationComponent.RemoveFromSyncList(syncNode->syncItem);
  sceneNode.RemoveNode(syncNode->sprite);

  auto iter = std::find(syncNodes.begin(), syncNodes.end(), syncNode);
  syncNodes.erase(iter);
}

#endif
