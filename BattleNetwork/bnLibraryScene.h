#pragma once
#include <Swoosh/Ease.h>
#include <Swoosh/Activity.h>

#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnChip.h"
#include "bnAnimation.h"
#include "bnAnimatedTextBox.h"

#include <list>

/*! \brief Library scene shows a list of unique chip data collected by the player */
class LibraryScene : public swoosh::Activity {
private:
  Camera camera;
  AnimatedTextBox textbox; /*!< Display extra chip info*/

  sf::Font* font; /*!< Menu name font */
  sf::Text* menuLabel; /*!< The menu text */

  double maxSelectInputCooldown; /*!< Max time to delay input */
  double selectInputCooldown; /*!< Current time left in input delay */

  sf::Font *chipFont; /*!< Chip font */
  sf::Text *chipLabel; /*!< Chip text */

  sf::Font *numberFont; /*!< Font for numbers */
  sf::Text *numberLabel; /*!< Numbers as text */

  sf::Font *chipDescFont; /*!< Font used for chip desc */
  sf::Text* chipDesc; /*!< Actual chip desc */

  sf::Sprite bg; /*!< Background for this scene */
  sf::Sprite folderDock; /*!< Area where folder list appears */
  sf::Sprite scrollbar; /*!< Scrollbar moves */
  sf::Sprite stars; /*!< Some chips are rare */
  sf::Sprite chipHolder; /*!< Chip visual where card and desc sits on top of */
  sf::Sprite element; /*!< Element of the chip */
  sf::Sprite cursor; /*!< Nav cursor */

  sf::Sprite chip; /*!< Current chip graphic */
  sf::IntRect cardSubFrame; /*!< The frame of the current chip from the texture atlas */

  sf::Sprite chipIcon; /*!< The mini icon */
  swoosh::Timer chipRevealTimer; /*!< Ease into the next chip for a flip effect */
  swoosh::Timer easeInTimer;

  int maxChipsOnScreen; /*!< Number of chip items that can appear in a list*/
  int currChipIndex; /*!< Current selection in the list */
  int lastChipOnScreen; /*!< The topmost chip seen in the list */
  int prevIndex; /*!< Animator if we've selected a new chip this frame */
  int numOfChips; /*!< Total of all chips in the folder to list */

  double totalTimeElapsed; /*!< accum. of seconds passed */
  double frameElapsed; /*!< delta last frame time in seconds */
  bool gotoNextScene; /*!< If true, player cannot interact with scene */

  std::list<Chip> uniqueChips; /*!< List of unique chips in pack */

  /**
   * @brief Takes user's folder and puts unique chips in a list
   * 
   * member var uniqueChips will be modified after this call
   */
  void MakeUniqueChipsFromPack();

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
   * @brief Forces chip desc to fit the chipHolder area
   * @return std::string copy of modified input string
   * 
   * Places newlines and splits strings to look like the original
   */
  std::string FormatChipDesc(const std::string&& desc);

  /**
   * @brief gotoNextScene is false and user can iteract again
   */
  virtual void onStart();
  
  /**
   * @brief Updates the scene and polls for user input 
   * @param elapsed in seconds
   * 
   * Updates the camera and textbox objects. Uses input delays to 
   * increase the currChipIndex used to navigate the list of unique chips.
   * If the cursor is out of range [lastChipOnScreen, lastChipOnScreen + chipSize]
   * then the min and max of the range are shifted in the preview direction.
   * This keeps the illusion that we are scrolling through a list window.
   * 
   * If A is pressed, the textbox queue is cleared and the chip extra information
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
   * @brief Draws only the chips in view. Places the scrollbar.
   * @param surface
   */
  virtual void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief Deletes all allocated resources
   */
  virtual void onEnd();
 
  /**
   * @brief Loads graphics, places textbox at (4, 255) and makes unique list of chips
   */
  LibraryScene(swoosh::ActivityController&);
  
  /**
   * @brief deconstructor
   */
  virtual ~LibraryScene();
};
