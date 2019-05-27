#pragma once
#include <SFML/Graphics.hpp>
using sf::IntRect;

#include "bnCharacter.h"
#include "bnMobState.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnMetalManIdleState.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"

<<<<<<< HEAD
=======
/*! \brief Metalman is a boss that throws blades, fires rockets, and punches the ground */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class MetalMan : public Character, public AI<MetalMan> {
public:
  friend class MetalManIdleState;
  friend class MetalManMoveState;
  friend class MetalManPunchState;
<<<<<<< HEAD
  friend class MetalManMissileState;

  MetalMan(Rank _rank);
  virtual ~MetalMan();

  virtual void Update(float _elapsed);
  virtual void RefreshTexture();
  virtual bool CanMoveTo(Battle::Tile * next);
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  virtual void SetCounterFrame(int frame);
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
  virtual int GetHealth() const;
  virtual TextureType GetTextureType() const;

  void SetHealth(int _health);
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);

  virtual const float GetHitHeight() const;
  virtual int* GetAnimOffset();

private:
  sf::Clock clock;

  AnimationComponent animationComponent;

  float hitHeight;
  string state;
  TextureType textureType;
  MobHealthUI* healthUI;
  sf::Shader* whiteout;
  sf::Shader* stun;

  bool movedByStun;
  bool hit;
=======

  MetalMan(Rank _rank);
  
  /**
   * @brief deconstructor
   */
  virtual ~MetalMan();

  /**
   * @brief Forces a move on metalman if he was stunned. Updates AI. Explodes when health is zero.
   * @param _elapsed in seconds
    */
  virtual void Update(float _elapsed);
  
  /**
   * @brief If the next tile does not contain obstacles or characters, Metalman can move to it
   * @param next the tile metalman wants to move to
   * @return true if free of characters and obstacles, false if otherwise
   * 
   * NOTE: Does not matter if either criteria has CanShareTile() enabled, metalman avoids it
   */
  virtual bool CanMoveTo(Battle::Tile * next);
  
  /**
   * @brief Delegates work to animationComponent
   * @param _state new animation state
   * @param onFinish when finish ends callback
   */
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  
  /**
   * @brief Toggles counter flag for this frame
   * @param frame the animation frame to toggle counter flag for
   */
  virtual void SetCounterFrame(int frame);
  
  /**
   * @brief Adds a callback when the frame is reached
   * @param frame the frame index to add callback to
   * @param onEnter the callback when the frame is first reached
   * @param onLeave the callback when leaving the frame
   * @param doOnce if true, callbacks never fire again, otherwise fire every time this fame
   */
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);

  /**
   * @brief If hit while on opponents side, requests a move next frame
   * @param props hit properties
   * @return true if hit, false if missed
   */
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);

  virtual const bool OnHit(Hit::Properties props) { return true; }

  virtual void OnDelete() { ; }
  
  /**
   * @brief Get the hit height for metalman
   * @return  const float
   * 
   * Used by spells that emit particle artifacts on hit
   */
  virtual const float GetHitHeight() const;

private:
  AnimationComponent animationComponent; /*!< animates this sprite scene node */

  float hitHeight; /*!< Hit height of metalman */
  string state; /*!< current animation name */
  MobHealthUI* healthUI; /*!< Health ui component */
  sf::Shader* whiteout; /*!< white shader */
  sf::Shader* stun; /*!< yellow shader */

  bool movedByStun; /*!< If metalman was stunned outside of this area, move him back to his space */
  bool hit; /*!< Flash white if his this frame */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

};