#pragma once
#include <SFML/Graphics.hpp>
using sf::IntRect;

#include "bnCharacter.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnMetalManIdleState.h"
#include "bnAnimationComponent.h"
#include "bnBossPatternAI.h"

/*! \brief Metalman is a boss that throws blades, fires rockets, and punches the ground */
class MetalMan : public Character, public BossPatternAI<MetalMan> {
  friend class MetalManIdleState;
  friend class MetalManMoveState;
  friend class MetalManPunchState;
  friend class MetalManMissileState;

public:
    using DefaultState = MetalManIdleState;

    MetalMan(Rank _rank);
  
  /**
   * @brief deconstructor
   */
  ~MetalMan();

  /**
   * @brief Forces a move on metalman if he was stunned. Updates AI. Explodes when health is zero.
   * @param _elapsed in seconds
    */
  void OnUpdate(float _elapsed);
  
  /**
   * @brief If the next tile does not contain obstacles or characters, Metalman can move to it
   * @param next the tile metalman wants to move to
   * @return true if free of characters and obstacles, false if otherwise
   * 
   * NOTE: Does not matter if either criteria has CanShareTile() enabled, metalman avoids it
   */
  bool CanMoveTo(Battle::Tile * next);
  
  /**
   * @brief Delegates work to animationComponent
   * @param _state new animation state
   * @param onFinish when finish ends callback
   */
  void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  
  /**
   * @brief Toggles counter flag for this frame
   * @param frame the animation frame to toggle counter flag for
   */
  void SetCounterFrame(int frame);
  
  /**
   * @brief Adds a callback when the frame is reached
   * @param frame the frame index to add callback to
   * @param onEnter the callback when the frame is first reached
   * @param onLeave the callback when leaving the frame
   * @param doOnce if true, callbacks never fire again, otherwise fire every time this fame
   */
  void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);

  /**
   * @brief If hit while on opponents side, requests a move next frame
   * @param props hit properties
   * @return true if hit, false if missed
   */
  const bool OnHit(const Hit::Properties props);

  void OnDelete();
 
private:
  AnimationComponent* animationComponent; /*!< animates this sprite scene node */
  DefenseRule* virusBody;
  float hitHeight; /*!< Hit height of metalman */
  string state; /*!< current animation name */
  MobHealthUI* healthUI; /*!< Health ui component */

  bool movedByStun; /*!< If metalman was stunned outside of this area, move him back to his space */
};