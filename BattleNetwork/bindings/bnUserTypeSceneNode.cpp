#ifdef BN_MOD_SUPPORT
#include "bnUserTypeSceneNode.h"

#include "bnWeakWrapper.h"
#include "../bnSceneNode.h"
#include "../bnScriptResourceManager.h"

void DefineSceneNodeUserType(sol::table& engine_namespace) {
  engine_namespace.new_usertype<WeakWrapper<SceneNode>>("SceneNode",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "SceneNode", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "SceneNode", key );
    },
    "show", [](WeakWrapper<SceneNode>& node) {
      node.Unwrap()->Reveal();
    },
    "hide", [](WeakWrapper<SceneNode>& node) {
      node.Unwrap()->Hide();
    },
    "set_layer", [](WeakWrapper<SceneNode>& node, int layer) {
      node.Unwrap()->SetLayer(layer);
    },
    "get_layer", [](WeakWrapper<SceneNode>& node) -> int {
      return node.Unwrap()->GetLayer();
    },
    "add_node", [](WeakWrapper<SceneNode>& node) -> WeakWrapper<SceneNode> {
      auto child = std::make_shared<SceneNode>();
      node.Unwrap()->AddNode(child);

      return WeakWrapper(child);
    },
    "remove_node", [](WeakWrapper<SceneNode>& node, WeakWrapper<SceneNode>& child) {
      node.Unwrap()->RemoveNode(child.Unwrap().get());
    },
    "add_tags", [](WeakWrapper<SceneNode>& node, std::initializer_list<std::string> tags) {
      node.Unwrap()->AddTags(tags);
    },
    "remove_tags", [](WeakWrapper<SceneNode>& node, std::initializer_list<std::string> tags) {
      node.Unwrap()->RemoveTags(tags);
    },
    "has_tags", [](WeakWrapper<SceneNode>& node, const std::string& tag) -> bool{
      return node.Unwrap()->HasTag(tag);
    },
    "find_child_nodes_with_tags", [](WeakWrapper<SceneNode>& node, std::initializer_list<std::string> tags) {
      auto nodes = node.Unwrap()->GetChildNodesWithTag(tags);
      std::vector<WeakWrapper<SceneNode>> result;
      result.reserve(nodes.size());

      for (auto& node : nodes) {
        result.push_back(WeakWrapper(node));
      }

      return sol::as_table(result);
    },
    "get_position", [](WeakWrapper<SceneNode>& node) -> sf::Vector2f {
      return node.Unwrap()->getPosition();
    },
    "set_position", [](WeakWrapper<SceneNode>& node, float x, float y) {
      node.Unwrap()->setPosition(x, y);
    },
    "enable_parent_shader", [](WeakWrapper<SceneNode>& node, bool enable) {
      node.Unwrap()->EnableParentShader(enable);
    }
  );
}
#endif
