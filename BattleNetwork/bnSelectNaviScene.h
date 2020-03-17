/*! \brief This scene allows user to select navi and see their stats
 * 
 * The scene takes SelectedNavi reference as input. The currently selected navi will
 * be directly modified. 
 * 
 * User can press LEFT/RIGHT to move through the navis loaded @see QueueNaviRegistration
 * User presses A to confirm selection
 * 
 * When entering/leaving the status props will animate and slide in/out
 */

#pragma once
#include <time.h>
#include <SFML/Graphics.hpp>

#include "bnSelectMobScene.h"
#include "bnNaviRegistration.h"
#include "bnGridBackground.h"
#include "bnCamera.h"
#include "bnAnimation.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnTextBox.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

constexpr float UI_LEFT_POS_MAX = 10.f; /*!< Left ui stops here */
constexpr float UI_RIGHT_POS_MAX = 300.f; /*!< Right ui stops here */
constexpr float UI_SPACING = 55.f; /*!< Spacing between status bars */
constexpr float UI_LEFT_POS_START = -300.f; /*!< Left ui begins here */
constexpr float UI_RIGHT_POS_START = 640.f; /*!< Right ui begins here */
constexpr float UI_TOP_POS_START = 250.f; /*!< ui begins at top pos here */
constexpr float UI_TOP_POS_MAX = 0.0f; /*!< ui ends here */
constexpr float MAX_PIXEL_FACTOR = 125.f;

class SelectNaviScene : public swoosh::Activity
{
private:
  SelectedNavi& naviSelectionIndex; /*!< SelectedNavi reference. Will change when user selects new navi */
  SelectedNavi prevChosen; /*!< The previous selected navi. Used to start effects. */

  Camera camera;

  double maxSelectInputCooldown; /*!< half of a second input delay */
  double selectInputCooldown;    /*!< count down before registering input */

  // NAVI UI font
  sf::Font* font;
  sf::Font *naviFont;
  sf::Text* menuLabel;

  sf::Text *naviLabel; /*!< navi name text */
  sf::Text *attackLabel; /*!< attack text */
  sf::Text *speedLabel; /*!< speed text */
  sf::Text *hpLabel; /*!< hp text */

  float maxNumberCooldown; /*!< half a second scramble effect */
  float numberCooldown; /*!< Effect count down */

  Background* bg; /*!< background graphics */

  // UI Sprites position
  float UI_RIGHT_POS; 
  float UI_LEFT_POS;
  float UI_TOP_POS;

  SpriteProxyNode charName; /*!< name holder panel */
  SpriteProxyNode charElement; /*!< element holder panel */
  SpriteProxyNode charStat; /*!< stat holder panel */
  SpriteProxyNode charInfo; /*!< navi info box */
  SpriteProxyNode element; /*! element icon */

  bool loadNavi;

  SpriteProxyNode navi; /*!< Navi sprite */
  
  Animation naviAnimator; /*!< Some navi idle graphics animate */

  double factor; /*!< Used in pixelate distortion effect */

  bool gotoNextScene; /*!< If true, user cannot interact with the scene */

  SmartShader pixelated; /*!< Pixelate distortion effect */

  Animation glowpadAnimator; /*!< Animator for the glowing pad */
  SpriteProxyNode glowpad; /*!< Glow pad ring piece */
  SpriteProxyNode glowbase; /*!< G;ow pad base */
  SpriteProxyNode glowbottom; /*!< Glow pad bottom piece */

  TextBox textbox; /*!< Displays extra navi info. Use UP/DOWN to read more */

  double elapsed; /*!< delta seconds sice last frame */
public:
  /**
   * @brief gotoNextScene is set to true so user must wait for transition to end to interact
   *
   * Loads and positions all graphics
   */
  SelectNaviScene(swoosh::ActivityController& controller, SelectedNavi& navi);
  
  /**
   * @brief Cleanup all allocated memory
   */
  virtual ~SelectNaviScene();

  /**
   * @brief Updates scene
   * @param elapsed in seconds
   * 
   * If the scene just started, bring in the UI LEFT and RIGHT panels
   * Once the panels are in view, the RIGHT panels space upward.
   * Then the name and navi data display on the screen and the user can interact.
   * 
   * The opposite happens when the user leaves the scene (Press B)
   */
  virtual void onUpdate(double elapsed);
  
  /**
   * @brief Draws entire scene
   * @param surface
   */
  virtual void onDraw(sf::RenderTexture& surface);
  
  /**
   * @brief gotoNextScene = false, user can interact
   */
  virtual void onStart();
  
  /**
   * @brief nothing
   */
  virtual void onLeave();
  
  /**
   * @brief nothing
   */
  virtual void onExit();
  
  /**
   * @brief nothing
   */
  virtual void onEnter();
  
  /**
   * @brief gotoNextScene = false, user can interact
   */
  virtual void onResume();
  
  /**
   * @brief nothing
   */
  virtual void onEnd();
};

