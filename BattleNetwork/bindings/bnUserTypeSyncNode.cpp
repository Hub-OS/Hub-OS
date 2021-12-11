#ifdef BN_MOD_SUPPORT
#include "bnUserTypeSyncNode.h"
#include "bnWeakWrapper.h"
#include "bnUserTypeAnimation.h"
#include "../bnSyncNode.h"

void DefineSyncNodeUserType(sol::table& engine_namespace) {
  engine_namespace.new_usertype<SyncNode>("SyncNode",
    "sprite", [](SyncNode& node) -> WeakWrapper<SpriteProxyNode> {
      return WeakWrapper(node.sprite);
    },
    "get_animation", [](std::shared_ptr<SyncNode>& node) -> AnimationWrapper {
      return AnimationWrapper(node, node->animation);
    }
  );
}
#endif
