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
#include "bnChargeEffectSceneNode.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
#include "bnPlayerHitState.h"
#include "bnPlayerChangeFormState.h"
#include "bnPlayerForm.h"

#include <array>

using sf::IntRect;

class Player : public Character, public AI<Player> {
  friend class PlayerControlledState;
  friend class PlayerIdleState;
  friend class PlayerHitState;
  friend class PlayerChangeFormState;

protected:
  bool RegisterForm(PlayerFormMeta* info);

  template<typename T>
  PlayerFormMeta* AddForm();
public:
  using DefaultState = PlayerControlledState;

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
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Fires a buster
   */
  virtual void Attack();

  /**
   * @brief Don't take damage if blinking. Responds to recoil props
   * @param props the hit props
   * @return true if the player got hit, false if missed
   */
  virtual const bool OnHit(const Hit::Properties props);

  /**
   * @brief when player is deleted, changes state to delete state and hide charge component
   */
  virtual void OnDelete();

  virtual const float GetHitHeight() const;
  /**
   * @brief Get how many times the player has moved across the grid
   * @return int
   */
  int GetMoveCount() const;
  
  /**
   * @brief Get how many times the player has been hit
   * @return int
   */
  int GetHitCount() const;

  /**
   * @brief Toggles the charge component
   * @param state
   */
  void SetCharging(bool state);

  /**
   * @brief Set the animation and on finish callback
   * @param _state name of the animation
   */
  void SetAnimation(string _state, std::function<void()> onFinish = nullptr);

  void EnablePlayerControllerSlideMovementBehavior(bool enable = true);
  const bool PlayerControllerSlideEnabled() const;

  virtual void ExecuteBusterAction() = 0;
  virtual void ExecuteChargedBusterAction() = 0;

  void ActivateFormAt(int index); 
  void DeactivateForm();
  const std::vector<PlayerFormMeta*> GetForms();
protected:
  int hitCount; /*!< How many times the player has been hit. Used by score board. */
  string state; /*!< Animation state name */
  bool playerControllerSlide;
  AnimationComponent* animationComponent;
  ChargeEffectSceneNode chargeEffect; /*!< Handles charge effect */

  std::array<PlayerFormMeta*, 5> forms;
  int formSize;
  PlayerForm* activeForm;
};

template<typename T>
PlayerFormMeta* Player::AddForm() {
  PlayerFormMeta* info = new PlayerFormMeta(formSize+1);
  info->SetFormClass<T>();
  
  if (!this->RegisterForm(info)) {
    delete info;
    info = nullptr;
  }

  return info;
}