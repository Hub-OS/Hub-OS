
/*! \brief Mystery data sits on the field and explodes if hit by anything 
 * 
 * Must tie  together with BattleOverTrigger to turn into "GET" sprite 
 * and generate a new reward
 */
 
#pragma once
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;

class MysteryData : public Character {
public:
  /**
   * @brief Sets health to 1 and begins loops spin animation
   */
  MysteryData(Field* _field, Team _team);
  
  /**
   * @brief deconstructor
   */
  virtual ~MysteryData();

  /**
   * @brief When health is zero, spawn an explosion effect and delete
   * @param _elapsed in seconds 
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Any impact damage instantly destroys the mystery data
   * @param props 
   * @return true if impact, false if non impact
   */
  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete() { ; }

  virtual const float GetHeight() const { return 0; }

  /**
   * @brief Changes the animation to "GET"
   */
  void RewardPlayer();

  /**
   * @brief Does not move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }

protected:
  AnimationComponent* animation; /*!< Animation component for mystery data */
};
