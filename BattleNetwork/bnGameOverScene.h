#pragma once
#include "bnEngine.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

#include <Swoosh/Activity.h>

/**
 * @class GameOverScene
 * @author mav
 * @date 04/05/19
 * @file bnGameOverScene.h
 * @brief Very simple scene shows GAME OVER graphic.
 * 
 * This is the first scene on the stack and resumed when players lose
 */
class GameOverScene : public swoosh::Activity {
private:
  float fadeInCooldown; /*!< Fade in time */
  sf::Sprite gameOver; /*!< GAME OVER */

public:
  GameOverScene(swoosh::ActivityController&);
  ~GameOverScene();

  void onStart();
  
  /**
   * @brief Fades in graphic
   * @param elapsed in seconds
   */
  void onUpdate(double elapsed);
  
  void onLeave() { ; }
  void onExit() { ; }
  void onEnter() { ; }
  
  /**
   * @brief Play game over stream
   */
  void onResume();
  
  /**
   * @brief Draws graphic
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface);
  
  void onEnd() { ; }
};
