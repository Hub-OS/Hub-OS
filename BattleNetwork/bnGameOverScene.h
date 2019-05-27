#pragma once
#include "bnEngine.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

#include <Swoosh/Activity.h>

<<<<<<< HEAD
class GameOverScene : public swoosh::Activity {
private:
  float fadeInCooldown;
  sf::Sprite gameOver;
  bool leave;
=======
/**
 * @class GameOverScene
 * @author mav
 * @date 04/05/19
 * @brief Very simple scene shows GAME OVER graphic.
 * 
 * This is the first scene on the stack and resumed when players lose
 */
class GameOverScene : public swoosh::Activity {
private:
  float fadeInCooldown; /*!< Fade in time */
  sf::Sprite gameOver; /*!< GAME OVER */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

public:
  GameOverScene(swoosh::ActivityController&);
  ~GameOverScene();

  void onStart();
<<<<<<< HEAD
  void onUpdate(double elapsed);
  void onLeave() { ; }
  void onExit() { ; }
  void onEnter() { ; }
  void onResume();
  void onDraw(sf::RenderTexture& surface);
=======
  
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
  
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void onEnd() { ; }
};
