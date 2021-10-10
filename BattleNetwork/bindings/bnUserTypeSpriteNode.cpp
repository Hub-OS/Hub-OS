#ifdef BN_MOD_SUPPORT
#include "bnUserTypeSpriteNode.h"
#include "../bnScriptResourceManager.cpp"

void DefineSpriteNodeUserType(sol::table& engine_namespace) {
  const auto& node_record = engine_namespace.new_usertype<SpriteProxyNode>("SpriteNode",
    sol::factories(
      []() -> std::shared_ptr<SpriteProxyNode> {
        return std::make_shared<SpriteProxyNode>();
      }
    ),
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "SpriteNode", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "SpriteNode", key );
    },
    "get_texture", &SpriteProxyNode::getTexture,
    "set_texture", &SpriteProxyNode::setTexture,
    "show", &SpriteProxyNode::Reveal,
    "hide", &SpriteProxyNode::Hide,
    "set_layer", &SpriteProxyNode::SetLayer,
    "get_layer", &SpriteProxyNode::GetLayer,
    "add_node", &SpriteProxyNode::AddNode,
    "remove_node", &SpriteProxyNode::RemoveNode,
    "add_tag", &SpriteProxyNode::AddTags,
    "remove_tags", &SpriteProxyNode::RemoveTags,
    "has_tag", &SpriteProxyNode::HasTag,
    "find_child_nodes_with_tags", &SpriteProxyNode::GetChildNodesWithTag,
    "get_layer", &SpriteProxyNode::GetLayer,
    "set_position", sol::resolve<void(float, float)>(&SpriteProxyNode::setPosition),
    "get_position", &SpriteProxyNode::getPosition,
    "get_color", &SpriteProxyNode::getColor,
    "set_color", &SpriteProxyNode::setColor,
    "unwrap", &SpriteProxyNode::getSprite,
    "enable_parent_shader", &SpriteProxyNode::EnableParentShader,
    sol::base_classes, sol::bases<SceneNode>()
  );
}
#endif
