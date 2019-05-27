<<<<<<< HEAD
=======
/*! \brief UIComponent is composed of a SceneNode and a Component class
 * 
 * UIComponents are injected into the battle scene and are drawn 
 * on top of everything else.
 * 
 * These components usually draw the health points of the entity
 * they are attached to but can be used to display other data.
 */ 

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once

#include "bnComponent.h"
#include "bnSceneNode.h"

class BattleScene;

<<<<<<< HEAD
/*
components used for UI lookup
*/

=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class UIComponent : public Component, public SceneNode {
private:

public:
  UIComponent() = delete;
<<<<<<< HEAD
=======
  /**
   * @brief Attaches this component to the owner
   * @param owner
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  UIComponent(Entity* owner) : Component(owner) { ; }
  ~UIComponent() { ; }

  UIComponent(UIComponent&& rhs) = delete;
  UIComponent(const UIComponent& rhs) = delete;

<<<<<<< HEAD
=======
  /**
   * @brief Calls parent SceneNode::draw() 
   * @param target
   * @param states
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const {
    SceneNode::draw(target, states);
  };

<<<<<<< HEAD
  virtual void Update(float _elapsed) = 0;
=======
  /**
   * @brief To be implemented: what happens on update
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed) = 0;
  
  /**
   * @brief To be implemented: what happens when the Battlescene requests injection
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Inject(BattleScene&) = 0;
}; 
