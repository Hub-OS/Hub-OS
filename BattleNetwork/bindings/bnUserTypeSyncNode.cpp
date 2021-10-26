#ifdef BN_MOD_SUPPORT
#include "bnUserTypeSyncNode.h"

#include "bnWeakWrapper.h"
#include "bnUserTypeAnimation.h"
#include "bnSyncNode.h"

void DefineSyncNodeUserType(sol::table& engine_namespace) {
  engine_namespace.new_usertype<SyncNode>("SyncNode",
    sol::meta_function::index, [](SyncNode& node, const std::string& key) {
      return node.dynamic_get(key);
    },
    sol::meta_function::new_index, [](SyncNode& node, const std::string& key, sol::stack_object value) {
      node.dynamic_set(key, value);
    },
    sol::meta_function::length, [](SyncNode& node) {
      return node.entries.size();
    },
    "sprite", [](SyncNode& node) -> WeakWrapper<SpriteProxyNode> {
      return WeakWrapper(node.sprite);
    },
    "get_animation", [](std::shared_ptr<SyncNode>& node) -> AnimationWrapper {
      return AnimationWrapper(node, node->animation);
    }
  );
}
#endif
