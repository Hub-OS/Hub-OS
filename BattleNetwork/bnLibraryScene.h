#pragma once
#include <Swoosh/Ease.h>

#include "bnScene.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"
#include "bnCard.h"
#include "bnAnimation.h"
#include "bnAnimatedTextBox.h"

#include <list>

/*! \brief Library scene shows a list of unique card data collected by the player 
    \warning no longer used. may not work.
*/
class LibraryScene : public Scene {
private:
  Camera camera;
  AnimatedTextBox textbox; /*!< Display extra card info*/

  Font font; /*!< Menu name font */
  Text menuLabel; /*!< The menu text */

  double maxSelectInputCooldown; /*!< Max time to delay input */
  double selectInputCooldown; /*!< Current time left in input delay */

  Font cardFont; /*!< Card font */
  Text cardLabel; /*!< Card text */

  Font numberFont; /*!< Font for numbers */
  Text numberLabel; /*!< Numbers as text */

  Font cardDescFont; /*!< Font used for card desc */
  Text cardDesc; /*!< Actual card desc */

  SpriteProxyNode bg; /*!< Background for this scene */
  SpriteProxyNode folderDock; /*!< Area where folder list appears */
  SpriteProxyNode scrollbar; /*!< Scrollbar moves */
  SpriteProxyNode stars; /*!< Some cards are rare */
  SpriteProxyNode cardHolder; /*!< Card visual where card and desc sits on top of */
  SpriteProxyNode element; /*!< Element of the card */
  SpriteProxyNode cursor; /*!< Nav cursor */

  sf::Sprite card; /*!< Current card graphic */
  sf::IntRect cardSubFrame; /*!< The frame of the current card from the texture atlas */

  SpriteProxyNode cardIcon; /*!< The mini icon */
  swoosh::Timer cardRevealTimer; /*!< Ease into the next card for a flip effect */
  swoosh::Timer easeInTimer;

  int maxCardsOnScreen; /*!< Number of card items that can appear in a list*/
  int currCardIndex; /*!< Current selection in the list */
  int firstCardOnScreen; /*!< The topmost card seen in the list */
  int prevIndex; /*!< Animator if we've selected a new card this frame */
  int numOfCards; /*!< Total of all cards in the folder to list */

  double totalTimeElapsed; /*!< accum. of seconds passed */
  double frameElapsed; /*!< delta last frame time in seconds */
  bool gotoNextScene; /*!< If true, player cannot interact with scene */

  std::list<Battle::Card> uniqueCards; /*!< List of unique cards in pack */

  /**
   * @brief Takes user's folder and puts unique cards in a list
   * 
   * member var uniqueCards will be modified after this call
   */
  void MakeUniqueCardsFromPack();

#ifdef __ANDROID__
    bool canSwipe;
    bool touchStart;

    int touchPosX;
    int touchPosStartX;

    bool releasedB;

    void StartupTouchControls();
    void ShutdownTouchControls();
#endif

public:
  /**
   * @brief Forces card desc to fit the cardHolder area
   * @return std::string copy of modified input string
   * 
   * Places newlines and splits strings to look like the original
   */
  std::string FormatCardDesc(const std::string&& desc);

  /**
   * @brief gotoNextScene is false and user can iteract again
   */
  virtual void onStart();
  
  /**
   * @brief Updates the scene and polls for user input 
   * @param elapsed in seconds
   * 
   * Updates the camera and textbox objects. Uses input delays to 
   * increase the currCardIndex used to navigate the list of unique cards.
   * If the cursor is out of range [lastCardOnScreen, lastCardOnScreen + cardSize]
   * then the min and max of the range are shifted in the preview direction.
   * This keeps the illusion that we are scrolling through a list window.
   * 
   * If A is pressed, the textbox queue is cleared and the card extra information
   * is entered, prompting to open.
   * 
   * The user can press B to cancel and close the textbox. B again to leave the scene.
   */
  virtual void onUpdate(double elapsed);
  
  /**
   * @brief Does nothing
   */
  virtual void onLeave();
  
  /**
   * @brief Does nothing
   */
  virtual void onExit();
  
  /**
   * @brief does nothing
   */
  virtual void onEnter();
  
  /**
    * @brief gotoNextScene is true and user can interact 
   */
  virtual void onResume();
  
  /**
   * @brief Draws only the cards in view. Places the scrollbar.
   * @param surface
   */
  virtual void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief Deletes all allocated resources
   */
  virtual void onEnd();
 
  /**
   * @brief Loads graphics, places textbox at (4, 255) and makes unique list of cards
   */
  LibraryScene(swoosh::ActivityController&);
  
  /**
   * @brief deconstructor
   */
  virtual ~LibraryScene();
};
