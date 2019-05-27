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
<<<<<<< HEAD
#include "bnMemory.h"
=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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
<<<<<<< HEAD

#include <time.h>
#include <SFML/Graphics.hpp>
=======

#include <time.h>
#include <SFML/Graphics.hpp>

#include <Swoosh/Activity.h>
#include "Segues/CrossZoom.h" // <-- GPU intensive and runs slowly on old hardware
#include "Segues/WhiteWashFade.h"
#include "Segues/BlackWashFade.h"

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

<<<<<<< HEAD
#include <Swoosh/Activity.h>
#include "Segues/CrossZoom.h" // <-- GPU intensive and runs slowly on old hardware
#include "Segues/WhiteWashFade.h"
#include "Segues/BlackWashFade.h"

class SelectMobScene : public swoosh::Activity
{
private:
  SelectedNavi selectedNavi;

  Camera camera;
  ChipFolder& selectedFolder;

  Mob* mob;

  // Menu name font
  sf::Font* font;
  sf::Text* menuLabel;

  // Selection input delays
  double maxSelectInputCooldown;
  double selectInputCooldown;
  double elapsed; // time

  // MOB UI font
  sf::Font *mobFont;
  sf::Text *mobLabel;
  sf::Text *attackLabel;
  sf::Text *speedLabel;
  sf::Text *hpLabel;

  float maxNumberCooldown;
  float numberCooldown;

  bool doOnce; // scene effects
  bool showMob; // toggle hide / show mob

  // select menu graphic
  sf::Sprite bg;

  // Current mob graphic
  sf::Sprite mobSpr;

  // bobbing cursors
  sf::Sprite cursor;

  sf::Sprite navigator;
  Animation navigatorAnimator;

  bool gotoNextScene;

  SmartShader shader;
  float factor;

  // Current selection index
  int mobSelectionIndex;

  // Text box navigator
  TextBox textbox;

public:
  SelectMobScene(swoosh::ActivityController&, SelectedNavi, ChipFolder& selectedFolder);
  ~SelectMobScene();

  virtual void onResume();
  virtual void onUpdate(double elapsed);
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onStart();
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
=======
class SelectMobScene : public swoosh::Activity
{
private:
  SelectedNavi selectedNavi; /*!< The selected navi */

  Camera camera;
  
  ChipFolder& selectedFolder; /*!< Reference to the selected folder */

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

  sf::Sprite bg; /*!< Background sprite */

  sf::Sprite mobSpr; /*!< Current mob sprite */

  sf::Sprite cursor; /*!< LEFT / RIGHT cursors */

  sf::Sprite navigator; /*!< Mugshot spritesheet */
  
  Animation navigatorAnimator; /*!< Animates mugshot */

  bool gotoNextScene; /*!< Cannot interact with scene if true */

  SmartShader shader; /*!< Pixelate shader effect*/
  
  float factor; /*!< Pixelate factor amount */

  int mobSelectionIndex; /*!< Mob selected */

  TextBox textbox; /*!< textbox message */

public:
  /**
   * @brief Loads graphics and sets original state of all items
   */
  SelectMobScene(swoosh::ActivityController&, SelectedNavi, ChipFolder& selectedFolder);
  
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
   * @brief Animates and accepts user input: LEFt/RIGHT A to battle, B to return
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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void onEnd();
};

