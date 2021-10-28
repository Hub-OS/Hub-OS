#ifdef BN_MOD_SUPPORT
#include "bnUserTypeSpriteNode.h"

#include "bnWeakWrapper.h"
#include "../bnSpriteProxyNode.h"
#include "../bnScriptResourceManager.h"

void DefineSpriteNodeUserType(sol::table& engine_namespace) {
  engine_namespace.new_usertype<WeakWrapper<SpriteProxyNode>>("SpriteNode",
    sol::meta_function::index, []( sol::table table, const std::string key ) { 
      ScriptResourceManager::PrintInvalidAccessMessage( table, "SpriteNode", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) { 
      ScriptResourceManager::PrintInvalidAssignMessage( table, "SpriteNode", key );
    },
    "get_texture", [](WeakWrapper<SpriteProxyNode>& node) -> std::shared_ptr<sf::Texture> {
      return node.Unwrap()->getTexture();
    },
    "set_texture", [](WeakWrapper<SpriteProxyNode>& node, std::shared_ptr<sf::Texture> texture) {
      node.Unwrap()->setTexture(texture);
    },
    "show", [](WeakWrapper<SpriteProxyNode>& node) {
      node.Unwrap()->Reveal();
    },
    "hide", [](WeakWrapper<SpriteProxyNode>& node) {
      node.Unwrap()->Hide();
    },
    "set_layer", [](WeakWrapper<SpriteProxyNode>& node, int layer) {
      node.Unwrap()->SetLayer(layer);
    },
    "get_layer", [](WeakWrapper<SpriteProxyNode>& node) -> int {
      return node.Unwrap()->GetLayer();
    },
    "create_node", [](WeakWrapper<SpriteProxyNode>& node) -> WeakWrapper<SpriteProxyNode> {
      auto child = std::make_shared<SpriteProxyNode>();
      node.Unwrap()->AddNode(child);

      return WeakWrapper(child);
    },
    "remove_node", [](WeakWrapper<SpriteProxyNode>& node, WeakWrapper<SpriteProxyNode>& child) {
      node.Unwrap()->RemoveNode(child.Unwrap().get());
    },
    "add_tags", [](WeakWrapper<SpriteProxyNode>& node, std::initializer_list<std::string> tags) {
      node.Unwrap()->AddTags(tags);
    },
    "remove_tags", [](WeakWrapper<SpriteProxyNode>& node, std::initializer_list<std::string> tags) {
      node.Unwrap()->RemoveTags(tags);
    },
    "has_tags", [](WeakWrapper<SpriteProxyNode>& node, const std::string& tag) -> bool{
      return node.Unwrap()->HasTag(tag);
    },
    "find_child_nodes_with_tags", [](WeakWrapper<SpriteProxyNode>& node, std::vector<std::string> tags) {
      auto nodes = node.Unwrap()->GetChildNodesWithTag(tags);
      std::vector<WeakWrapper<SceneNode>> result;
      result.reserve(nodes.size());

      for (auto node : nodes) {
        result.push_back(WeakWrapper(node));
      }

      return sol::as_table(result);
    },
    "get_position", [](WeakWrapper<SpriteProxyNode>& node) -> sf::Vector2f {
      return node.Unwrap()->getPosition();
    },
    "set_position", [](WeakWrapper<SpriteProxyNode>& node, float x, float y) {
      node.Unwrap()->setPosition(x, y);
    },
    "get_origin", [](WeakWrapper<SpriteProxyNode>& node) -> sf::Vector2f {
      return node.Unwrap()->getOrigin();
    },
    "set_origin", [](WeakWrapper<SpriteProxyNode>& node, float x, float y) {
      node.Unwrap()->setOrigin(x, y);
    },
    "get_width", [](WeakWrapper<SpriteProxyNode>& node) -> float {
      auto self = node.Unwrap();
      return self->getLocalBounds().width * self->getScale().x;
    },
    "get_height", [](WeakWrapper<SpriteProxyNode>& node) -> float {
      auto self = node.Unwrap();
      return self->getLocalBounds().height * self->getScale().y;
    },
    "get_color", [](WeakWrapper<SpriteProxyNode>& node) -> sf::Color {
      return node.Unwrap()->getColor();
    },
    "set_color", [](WeakWrapper<SpriteProxyNode>& node, sf::Color color) {
      node.Unwrap()->setColor(color);
    },
    "get_color_mode", [](WeakWrapper<SpriteProxyNode>& node) -> ColorMode {
      return node.Unwrap()->GetColorMode();
    },
    "set_color_mode", [](WeakWrapper<SpriteProxyNode>& node, ColorMode mode) {
      node.Unwrap()->SetColorMode(mode);
    },
    "unwrap", [](WeakWrapper<SpriteProxyNode>& node) -> WeakWrapper<SpriteProxyNode> {
      return node;
    },
    "enable_parent_shader", [](WeakWrapper<SpriteProxyNode>& node, bool enable) {
      node.Unwrap()->EnableParentShader(enable);
    }
  );
}
#endif
