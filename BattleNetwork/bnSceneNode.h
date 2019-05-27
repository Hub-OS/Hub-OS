<<<<<<< HEAD
=======
/*! \brief SceneNodes are used for visually connected assets like aura's attached to entities 
 * 
 * Nodes attached are not handled by the parent node.
 * Do not expect the deletion of this node to free the memory.
 * */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>

class SceneNode : public sf::Transformable, public sf::Drawable {
<<<<<<< HEAD
private:
  std::vector<SceneNode*> childNodes;
  std::vector<sf::Drawable*> sprites;
  SceneNode* parent;
  bool show;

public:
  SceneNode() {
    show = true;
  }

  virtual ~SceneNode() {

  }

  void Hide()
  {
    show = false;
  }

  void Reveal()
  {
    show = true;
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!show) return;

    // combine the parent transform with the node's one
    //sf::Transform combinedTransform =this->getTransform();
    
    /*if (parent) {
      combinedTransform *= parent->getTransform();
    }*/

    //states.transform *= combinedTransform;

    // Draw sprites
    for (std::size_t i = 0; i < sprites.size(); i++) {
     target.draw(*sprites[i], states);
    }

    // draw its children
    for (std::size_t i = 0; i < childNodes.size(); i++) {
      childNodes[i]->draw(target, states);
    }
  }

  void AddNode(SceneNode* child) { if (child == nullptr) return;  child->parent = this; childNodes.push_back(child); }
  void RemoveNode(SceneNode* find) {
    if (find == nullptr) return;

    auto iter = std::remove_if(childNodes.begin(), childNodes.end(), [&find](SceneNode *in) { return in == find; });
    (*iter)->parent = nullptr;
    
    childNodes.erase(iter, childNodes.end());
  }

  void AddSprite(sf::Drawable* sprite) { sprites.push_back(sprite); }

  void RemoveSprite(sf::Drawable* find) {
    auto iter = std::remove_if(sprites.begin(), sprites.end(), [&find](sf::Drawable *in) { return in == find; });

    sprites.erase(iter, sprites.end());
  }

  const bool IsHidden() const {
    return !show;
  }

  const bool IsRevealed() const {
    return show;
  }
};
=======
protected:
  mutable std::vector<SceneNode*> childNodes; /*!< List of all children */
  SceneNode* parent; /*!< The node this node is a child of */
  bool show; /*!< Flag to hide or display a scene node and its children */
  int layer; /*!< Draw order of this node */
  
public:
  /**
   * @brief Sets layer to 0 and show to true
   */
  SceneNode();

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
};
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
