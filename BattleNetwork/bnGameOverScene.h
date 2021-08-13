#pragma once
#include "bnScene.h"
#include "bnGame.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

/**
 * @class GameOverScene
 * @author mav
 * @date 04/05/19
 * @brief Very simple scene shows GAME OVER graphic.
 * 
 * This is the first scene on the stack and resumed when players lose
 */
class GameOverScene : public Scene {
private:
  float fadeInCooldown; /*!< Fade in time */
  sf::Sprite gameOver; /*!< GAME OVER */
  sf::Texture gameOverTexture; /*< GAME OVER TEXTURE */
  bool leave; /*!< Scene state coming/going flag */

public:
  GameOverScene(swoosh::ActivityController&);
  ~GameOverScene();

  /**
 * @brief Play game over stream
 */
  void onStart();
  
  /**
   * @brief Fades in graphic
   * @param elapsed in seconds
   */
  void onUpdate(double elapsed);
  
  void onLeave() { ; }
  void onExit() { ; }
  void onEnter() { ; }
  
  void onResume();
  
  /**
   * @brief Draws graphic
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface);
  
  void onEnd() { ; }
};
