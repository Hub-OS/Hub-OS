/*! \brief Loads a scene to select the mob to battle 
 * 
 * Much like the SelectNaviScene, user can press LEFT/RIGHT to change 
 * the mob. Information about the mob displays on the right.
 * The name and data scramble for effects.
 * 
 * A textbox types out the extra information about the mob while a mugshot
 * animated along with the text
 */

#pragma once

#include "bnMobRegistration.h"
#include "bnNaviRegistration.h"
#include "bnTextBox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnStarman.h"
#include "bnMob.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnBattleScene.h"
#include "bnMobFactory.h"
#include "bnRandomMettaurMob.h"
#include "bnProgsManBossFight.h"
#include "bnTwoMettaurMob.h"
#include "bnCanodumbMob.h"

#include <time.h>
#include <SFML/Graphics.hpp>

#include <Swoosh/Activity.h>
#include "Segues/CrossZoom.h" // <-- GPU intensive and runs slowly on old hardware
#include "Segues/WhiteWashFade.h"
#include "Segues/BlackWashFade.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

class SelectMobScene : public swoosh::Activity
{
private:
  SelectedNavi selectedNavi; /*!< The selected navi */

  Camera camera;
  
  CardFolder& selectedFolder; /*!< Reference to the selected folder */

  Mob* mob; /*!< Pointer to the mob data */

  sf::Font* font; /*!< Menu title font */
  sf::Text* menuLabel; /*!< "Mob Select" */

  double maxSelectInputCooldown; /*!< Maximum time for input delay */
  double selectInputCooldown; /*!< Remaining time for input delay */
  double elapsed; /*!< delta seconds since last frame */

  sf::Font *mobFont; /*!< font for mob data */
  sf::Text *mobLabel; /*!< name */
  sf::Text *attackLabel; /*!< power */
  sf::Text *speedLabel; /*!< mob speed */
  sf::Text *hpLabel; /*!< mob total health */

  float maxNumberCooldown; /*!< Maximum time for the scramble effect */
  float numberCooldown; /*!< Remaining time for scramble effect */

  bool doOnce; /*!< Flag to trigger pixelate and scramble effects */
  bool showMob;

#ifdef __ANDROID__
  bool canSwipe;
  bool touchStart;

  int touchPosX;
  int touchPosStartX;

  bool releasedB;

  void StartupTouchControls();
  void ShutdownTouchControls();
#endif

  sf::Sprite bg; /*!< Background sprite */

  sf::Sprite mobSpr; /*!< Current mob sprite */

  sf::Sprite cursor; /*!< LEFT / RIGHT cursors */

  sf::Sprite navigator; /*!< Mugshot spritesheet */
  
  Animation navigatorAnimator; /*!< Animators mugshot */

  bool gotoNextScene; /*!< Cannot interact with scene if true */

  SmartShader shader; /*!< Pixelate shader effect*/
  
  float factor; /*!< Pixelate factor amount */

  int mobSelectionIndex; /*!< Mob selected */

  TextBox textbox; /*!< textbox message */

public:
  /**
   * @brief Loads graphics and sets original state of all items
   */
  SelectMobScene(swoosh::ActivityController&, SelectedNavi, CardFolder& selectedFolder);
  
  /**
   * @brief Deletes all allocated resource. If mob is non null, deletes the mob
   */
  ~SelectMobScene();

  /**
   * @brief sets gotoNextScene to false, allowing the user to interact
   * If mob is non null (from battling), deletes the mob
   */
  virtual void onResume();
  
  /**
   * @brief Animators and accepts user input: LEFt/RIGHT A to battle, B to return
   * @param elapsed
   */
  virtual void onUpdate(double elapsed);
  
  /**
   * @brief Draws the scene
   * @param surface
   */
  virtual void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief Unpauses textbox, triggers special effects to perform again
   */
  virtual void onStart();
  
  /**
   * @brief Pauses text box
   */
  virtual void onLeave();
  
  /**
   * @brief Clears the text box
   */
  virtual void onExit();
  
  /**
   * @brief nothing
   */
  virtual void onEnter();
  
  /**
   * @brief nothing
   */
  virtual void onEnd();
};

