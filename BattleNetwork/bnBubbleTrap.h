#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

/**
 * @class BubbleTrap
 * @author mav
 * @date 05/05/19
 * @brief Visual artifact (bubble animates) that can be attached to an entity
 * 
 * Creates a defense rule to absord first damage and forces this bubble to pop
 * On pop, frees owner and deletes self
 */
class BubbleTrap : virtual public Artifact, virtual public Component
{
private:
  Animation animation;
  sf::Sprite bubble;
  double duration; /*!< when this reaches zero, pops */
  DefenseRule* defense; /*!< Add BubbleWrapTrap defense rule */
public:
  /**
   * @brief Attaches to the owner, sets the animation */
  BubbleTrap(Character* owner);
  
  ~BubbleTrap();
  
  /**
   * @brief Does not inject into the battle scene
   * 
   */
  virtual void Inject(BattleScene&);
  
  /**
   * @brief Animates bubble. When timer runs out, pops the bubble
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Does not move
   * @param _direction
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }

  /**
   * @brief Sets the pop animation and deletes self when over
   */
  void Pop();
};
