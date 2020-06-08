#pragma once
#include <Swoosh/ActivityController.h>
#include <atomic>

#include "bnDrawWindow.h"
#include "bnConfigReader.h"
#include "bnConfigSettings.h"
#include "bnSpriteProxyNode.h"
#include "bnAnimation.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"

#define ONB_REGION_JAPAN 0
#define ONB_ENABLE_PIXELATE_GFX 0

// GBA draws 60 frames in one seconds
#define FIXED_TIME_STEP 1.0f/60.0f

/*! \file  bnGame.cpp
 *  \brief Main entry for the application.
 *         Encapsulates all resources and loading steps for the game.
 *
 * The bootup step loads resources in real-time
 * and prints status information for each resource in the background
 * of the title screen.
 *
 * All queued 3rd party plugins for navis, cards, and mobs are
 * parsed and loaded into the resource managers after primary
 * resources are loaded.
 *
 * Once all items are loaded, the user is allowed to press start and
 * play. From there, the Swoosh ActivityController controls the state
 * of the app until the user quits. Afterwards all resources
 * are cleaned up.
 */

using swoosh::ActivityController;

class Game final : public ActivityController {
private:
  TextureResourceManager textureManager;
  AudioResourceManager audioManager;
  ShaderResourceManager shaderManager;

  DrawWindow window;
  bool showScreenBars;
  ConfigReader reader;
  ConfigSettings configSettings;

  // mouse stuff
  SpriteProxyNode mouse;
  sf::Vector2f lastMousepos;
  Animation mouseAnimation;
  double mouseAlpha;

  // loading spinner
  SpriteProxyNode spinner;
  Animation spinnerAnimator;

  // We need a render surface to draw to so Swoosh ActivityController
  // can add screen transition effects from the title screen
  sf::RenderTexture loadSurface;
public:
  Game();
  Game(const Game&) = delete;
  ~Game();

  void Boot();
  void Poll();
  void SetWindowMode(DrawWindow::WindowMode mode);

private:
  DrawWindow::WindowMode windowMode;

  void LoadConfigSettings();

 /*! \brief This thread initializes all navis
 *
 * Uses an std::atomic<int> pointer
 * to keep track of all successfully loaded
 * objects.
 *
 * After the media resources are loaded we
 * safely load all registered navis.
 * Loaded navis will show up in navi select
 * screen and can be chosen to play as in
 * battle.
 */
  void RunNaviInit(std::atomic<int>* progress);

 /*! \brief This thread tnitializes all mobs
 *
 * @see RunNaviInit()
 *
 * After the media resources are loaded we
 * safely load all registed mobs.
 * Loaded mobs will show up in mob select
 * screen and can be chosen to battle
 * against.
 */
  void RunMobInit(std::atomic<int>* progress);

 /*! \brief This thread loads textures and shaders
 *
 * Uses and std::atomic<int> pointer to keep
 * count of successfully loaded objects
 */
  void RunGraphicsInit(std::atomic<int> * progress);

 /*! \brief This thread loads sound effects
 *
 * Uses and std::atomic<int> pointer to keep
 * count of successfully loaded objects
 */
  void RunAudioInit(std::atomic<int> * progress);

 /*! \brief This function describes how the app behaves on focus regain
 *
 * Refresh the graphics context and enable Audio() again
 */
  void GainFocus();

 /*! \brief This function describes how the app behaves on focus lost
 *
 * Mute the Audio()
 */
  void LoseFocus();

 /*! \brief This function describes how the app behaves on window resize
 *
 * Refresh the viewport
 */
  void Resize(int newWidth, int newHeight);

};