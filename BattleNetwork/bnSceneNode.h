
/*! \brief SceneNodes are used for visually connected assets like aura's attached to entities 
 * 
 * Nodes attached are not handled by the parent node.
 * Do not expect the deletion of this node to free the memory.
 * */

#pragma once
#include <vector>
#include<set>
#include <algorithm>
#include <SFML/Graphics.hpp>

class SceneNode : public sf::Transformable, public sf::Drawable {
protected:
  std::set<std::string> tags; /*!< Tags to lookup nodes by*/
  mutable std::vector<SceneNode*> childNodes; /*!< List of all children */
  SceneNode* parent; /*!< The node this node is a child of */
  bool show; /*!< Flag to hide or display a scene node and its children */
  int layer; /*!< Draw order of this node */
  bool useParentShader{ false }; /*!< Default: use your own internal shader*/

public:
  /**
   * @brief Sets layer to 0 and show to true
   */
  SceneNode();

  SceneNode(const SceneNode& rhs) = delete;

  /**
   * @brief Deconstructor does not delete children
   */
  virtual ~SceneNode();
  
  /**
   * @brief Sets the layer
   * @param layer
   */
  void SetLayer(int layer);
  
  /**
   * @brief Get the layer
   * @return const int
   */
  const int GetLayer() const;

  /**
   * @brief Hide this node and all children nodes
   */
  void Hide();

  /**
   * @brief Show this node and all children nodes
   */
  void Reveal();

  /**
   * Query if node is hidden
   * @return true if hidden, false otherwise
   */
  const bool IsHidden() const;

  /**
   * Query is node is visible
   * @return false if hidden, true otherwise
   */
  const bool IsVisible() const;

  /**
   * @brief Sort the nodes by Ascending Z Order and draw the nodes
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  /**
   * @brief Adds a child node
   * @param child the node to add
   * 
   * Must be non null
   */
  void AddNode(SceneNode* child);
  
  /**
   * @brief Removes a child node
   * @param find the node to remove if it exists
   */
  void RemoveNode(SceneNode* find);

  /**
  * @brief Forces the child node to use its parents shader in the render step
  */
  void EnableParentShader(bool enable = true);

  /**
  * @brief Query if the node is using its parent's shader
  */
  const bool IsUsingParentShader() const;

  /**
  * Fetches all the child nodes attached to this node
  * @return a reference to the vector of SceneNode*
  */
  std::vector<SceneNode*>& GetChildNodes();

  /**
  * Fetches all the nodes attached to this node with any of the tags
  * @return a vector of SceneNode*
  */
  std::set<SceneNode*> GetChildNodesWithTag(const std::initializer_list<std::string>& query);
  
  SceneNode* GetParent();

  void AddTags(const std::initializer_list<std::string>& tags);
  void RemoveTags(const std::initializer_list<std::string>& tags);
  const bool HasTag(const std::string& name);
};
