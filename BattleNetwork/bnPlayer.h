/*! \brief Player controlled net navi prefab 
 * 
 * This class is the foundation for all player controlled characters
 * To write a new netnavi inherit from this class
 * And simply load new sprites and animation file
 * And change some properties
 */

#pragma once

#include "bnCharacter.h"
#include "bnPlayerState.h"
#include "bnTextureType.h"
#include "bnChargeComponent.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
#include "bnPlayerHitState.h"

using sf::IntRect;

class Player : public Character, public AI<Player> {
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;
  friend class PlayerHitState;

  /**
   * @brief Loads graphics and adds a charge component
   */
  Player();
  
  /**
   * @brief deconstructor
   */
  virtual ~Player();

  /**
   * @brief Polls for interrupted states and fires delete state when deleted
   * @param _elapsed for secs
   */
  virtual void Update(float _elapsed);
<<<<<<< HEAD
  void Attack(float _charge);

  virtual int GetHealth() const;
  virtual void SetHealth(int _health);
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
=======
  
  /**
   * @brief Fires a buster
   */
  void Attack();

  /**
   * @brief Don't take damage if blinking. Responds to recoil props
   * @param props the hit props
   * @return true if the player got hit, false if missed
   */
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
  
  virtual const bool OnHit(Hit::Properties props) { return true;  }

  virtual void OnDelete() { ; }

  virtual const float GetHitHeight() const { return 0; }
  /**
   * @brief Get how many times the player has moved across the grid
   * @return int
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  int GetMoveCount() const;
  
  /**
   * @brief Get how many times the player has been hit
   * @return int
   */
  int GetHitCount() const;

<<<<<<< HEAD
=======
  /**
   * @brief Get the animation component used by the player
   * @return AnimationComponent&
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  AnimationComponent& GetAnimationComponent();

  /**
   * @brief Toggles the charge component
   * @warning This will be removed in future versions. Use GetComponent<ChargeComponent>()
   * @param state
   */
  void SetCharging(bool state);

<<<<<<< HEAD
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);

protected:
  int health;
  int hitCount;

  double invincibilityCooldown;

  TextureType textureType;
  string state;

  //-Animation-
  float animationProgress;

  ChargeComponent chargeComponent;
  AnimationComponent animationComponent;
=======
  /**
   * @brief Set the animation and on finish callback
   * @param _state name of the animation
   */
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);

protected:
  int hitCount; /*!< How many times the player has been hit. Used by score board. */
  double invincibilityCooldown; /*!< The blinking timer */
  string state; /*!< Animation state name */

  ChargeComponent chargeComponent; /*!< Handles charge effect */
  AnimationComponent animationComponent; /*!< Animates the character */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};