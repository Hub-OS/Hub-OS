/*! \brief ProgsMan is boss that throws Prog Bombs and punches the player
 * 
 * ProgsMan is a Character which means he has health and can take damage.
 * He uses a State Machine for his AI.
 * 
 * When his health is below or equal to zero, he goes into navi delete state.
 */
#pragma once
#include <SFML/Graphics.hpp>
using sf::IntRect;

#include "bnCharacter.h"
#include "bnMobState.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnProgsManIdleState.h"
#include "bnProgsManHitState.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"

class ProgsMan : public Character, public AI<ProgsMan> {
public:
  friend class ProgsManIdleState;
  friend class ProgsManMoveState;
  friend class ProgsManAttackState;

  /**
   * \brief Loads resources and sets health
   */
  ProgsMan(Rank _rank);
  
  /**
   * @brief deconstructor
   */
  virtual ~ProgsMan();

  /**
   * @brief Calls Character::Update() for battle resolution and updates animation
   * @param _elapsed
   * 
   * When health is zero, changes to NaviExplosion state
   */
  virtual void Update(float _elapsed);

  /**
   * @brief Delegates animation commands to animationComponent
   * @param _state the animation to change to
   * @param onFinish the callback that happens when the animation ends
   */
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  
  /**
   * @brief Sets the animation at frame to toggle the counter flag
   * @param frame the frame of the animation that can be countered
   * 
   * When a spell attacks progsman with counter enabled, progsman is stunned
   */
  virtual void SetCounterFrame(int frame);
  
  /**
   * @brief Delegate animation commands to animationComponnent
   * @param frame the frame to add callbacks to
   * @param onEnter callbacks fire when entering this frame
   * @param onLeave callbacks fire when leaving this frame
   * @param doOnce If true, the callbacks will fire and never fire again
   */
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
  
  /**
   * @brief Describes what happens when progsman gets hit
   * @param props the propeties progsman was hit with
   * @return true if hit, false if missed
   */
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);

  virtual const bool OnHit(Hit::Properties props) { return true; }

  virtual void OnDelete() { ; }

  /**
   * @brief Returns progsman's height 
   * @return const float
   */
  virtual const float GetHitHeight() const;
private:
  AnimationComponent animationComponent; /*!< component animates entities*/

  float hitHeight; /*!< The height for progsman for any given frame */
  string state; /*!< Animation name */
  TextureType textureType; /*!< Progsman's texture */

  sf::Shader* whiteout; /*!< whiteout effect for basics hits */
  sf::Shader* stun; /*!< Stun yellow effect */
};
