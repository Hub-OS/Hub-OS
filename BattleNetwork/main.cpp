/*! \file  Main.cpp 
 *  \brief Main entry for the application.
 *         Loads resources for the entire game.
 * 
 * The main entry for the program loads resources in real-time
 * and prints status information for each resource in the background
 * of the title screen. 
 * 
 * All queued 3rd party plugins for navis, chips, and mobs are
 * parsed and loaded into the resource managers after primary
 * resources are loaded. 
 * 
 * Once all items are loaded, the user is allowed to press start and 
 * play. From there, the Swoosh ActivityController controls the state
 * of the app until the user quits. Afterwards all resources
 * are cleaned up.
 */

#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnNaviRegistration.h"
#include "bnInputManager.h"
#include "bnEngine.h"
#include "bnGameOverScene.h"
#include "bnMainMenuScene.h"
#include "bnFakeScene.h"
#include "bnAnimator.h"
#include "bnConfigReader.h"
#include "bnConfigScene.h"
#include "SFML/System.hpp"

#include <time.h>
#include <queue>
#include <atomic>
#include <cmath>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>

// #define BN_REGION_JAPAN 1

// Engine addons
#include "bnQueueNaviRegistration.h"
#include "bnQueueMobRegistration.h"

// Timer
using sf::Clock;

// Swoosh activity management
using swoosh::ActivityController;

// Title card character
// NOTE: This is legacy code written before
//        reusable animation classes and
//        shader resource manager. 
//        Can be upgraded.
#define TITLE_ANIM_CHAR_SPRITES 14
#define TITLE_ANIM_CHAR_WIDTH 128
#define TITLE_ANIM_CHAR_HEIGHT 221

// GBA draws 60 frames in one seconds
#define FIXED_TIME_STEP 1.0f/60.0f

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
void RunNaviInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();

  NAVIS.LoadAllNavis(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded registered navis: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();
}

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
void RunMobInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();

  MOBS.LoadAllMobs(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded registered mobs: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();
}

/*! \brief This thread loads textures and shaders
 * 
 * Uses and std::atomic<int> pointer to keep
 * count of successfully loaded objects
 */
void RunGraphicsInit(std::atomic<int> * progress) {
  clock_t begin_time = clock();
  TEXTURES.LoadAllTextures(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded textures: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();

  begin_time = clock();
  SHADERS.LoadAllShaders(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded shaders: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();
}

/*! \brief This thread loads sound effects
 * 
 * Uses and std::atomic<int> pointer to keep
 * count of successfully loaded objects
 */
void RunAudioInit(std::atomic<int> * progress) {
  const clock_t begin_time = clock();
  AUDIO.LoadAllSources(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded audio sources: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();
}

/*! \brief This function describes how the app behaves on focus regain
 *  
 * Refresh the graphics context and enable audio again 
 */
void AppRegainFocus() {
  ENGINE.RegainFocus();
  AUDIO.EnableAudio(true);

#ifdef __ANDROID__
  // TODO: Reload all graphics and somehow reassign all gl IDs to all allocated sfml graphics structures
  // TEXTURES.RecoverLostGLContext();
  // ENGINE.RecoverLostGLContext(); // <- does the window need recreation too?
#endif
}
/*! \brief This function describes how the app behaves on window resize
 *
 * Refresh the viewport
 */

void AppResize(int newWidth, int newHeight) {
  ENGINE.Resize(newWidth, newHeight);
}

/*! \brief This function describes how the app behaves on focus lost
 * 
 * Mute the audio 
 */
void AppLoseFocus() {
  AUDIO.EnableAudio(false);
}

int main(int argc, char** argv) {
  // Initialize the engine and log the startup time
  const clock_t begin_time = clock();
  ENGINE.Initialize();
  Logger::Logf("Engine initialized: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);

  // lazy init
  // 
  // These macros hide the singleton that
  // allocates the resource managers when 
  // they are first called
  TEXTURES;
  SHADERS;
  AUDIO;
  QueuNaviRegistration(); // Queues navis to be loaded later
  QueueMobRegistration(); // Queues mobs to be loaded later

  AUDIO.SetStreamVolume(0);

  // Tell the input event loop how to behave when the app loses and regains focus
  INPUT.BindLoseFocusEvent(AppLoseFocus);
  INPUT.BindRegainFocusEvent(AppRegainFocus);
  INPUT.BindResizedEvent(AppResize);

  // State flags
  bool inConfigMessageState = true;

  // try to read the config file
  ConfigReader config("options.ini");

  if (!config.IsOK()) {
    // If the file is not ok, don't use the config
    // And skip the success screen
    inConfigMessageState = false; 
  }
  else {
    // If the file is good, use the audio and 
    // controller settings from the config
    AUDIO.EnableAudio(config.IsAudioEnabled());
    INPUT.SupportConfigSettings(config);
  }

  /**
    Because the resource managers have yet to be loaded 
    We must manually load some graphics ourselves
  */
  sf::Texture* alert = TEXTURES.LoadTextureFromFile("resources/ui/alert.png");
  sf::Sprite alertSprite(*alert);
  alertSprite.setScale(2.f, 2.f);
  alertSprite.setOrigin(alertSprite.getLocalBounds().width / 2, alertSprite.getLocalBounds().height / 2);
  sf::Vector2f alertPos = (sf::Vector2f)((sf::Vector2i)ENGINE.GetWindow()->getSize() / 2);
  alertSprite.setPosition(sf::Vector2f(100.f, alertPos.y));

  sf::Texture* mouseTexture = TEXTURES.LoadTextureFromFile("resources/ui/mouse.png");
  sf::Sprite mouse(*mouseTexture);
  mouse.setScale(2.f, 2.f);
  Animation mouseAnimation("resources/ui/mouse.animation");
  mouseAnimation.Reload();
  mouseAnimation.SetAnimation("DEFAULT");
  mouseAnimation << Animator::Mode::Loop;
  sf::Vector2f lastMousepos;
  double mouseAlpha = 1.0;

  // Title screen logo based on region
#ifdef BN_REGION_JAPAN
  sf::Texture* logo = TEXTURES.LoadTextureFromFile("resources/backgrounds/title/tile.png");
#else
  sf::Texture* logo = TEXTURES.LoadTextureFromFile("resources/backgrounds/title/tile_en.png");
#endif BN_REGION_JAPAN

  SpriteSceneNode logoSprite;

  logoSprite.setTexture(*logo);
  logoSprite.setOrigin(logoSprite.getLocalBounds().width / 2, logoSprite.getLocalBounds().height / 2);
  sf::Vector2f logoPos = sf::Vector2f(240.f, 160.f);
  logoSprite.setPosition(logoPos);

  // Log output text
  sf::Font* font = TEXTURES.LoadFontFromFile("resources/fonts/NETNAVI_4-6_V3.ttf");
  sf::Text* logLabel = new sf::Text("...", *font);
  logLabel->setCharacterSize(10);
  logLabel->setOrigin(0.f, logLabel->getLocalBounds().height);
  std::vector<std::string> logs;

  // Press Start text
  sf::Font* startFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");

#if defined(__ANDROID__)
  sf::Text* startLabel = new sf::Text("TAP SCREEN", *startFont);
#else
  sf::Text* startLabel = new sf::Text("PRESS START", *startFont);
#endif

  startLabel->setCharacterSize(24);
  startLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
  startLabel->setPosition(sf::Vector2f(180.0f, 240.f));

  // Loaded navis label text
  sf::Text* navisLoadedLabel = new sf::Text("Loading Navi Data...", *startFont);
  navisLoadedLabel->setCharacterSize(24);
  navisLoadedLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
  navisLoadedLabel->setPosition(sf::Vector2f(230.f, 230.f));


  // Loaded mobs label text
  sf::Text* mobLoadedLabel = new sf::Text("Loading Mob Data...", *startFont);
  mobLoadedLabel->setCharacterSize(24);
  mobLoadedLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
  mobLoadedLabel->setPosition(sf::Vector2f(230.f, 230.f));
 
  // If using chrono x config file, deliver a message to them
  sf::Text* message = new sf::Text("Your Chrono X config settings\nhave been imported", *startFont);
  message->setCharacterSize(24);
  message->setOrigin(message->getLocalBounds().width/2.f, message->getLocalBounds().height*2);
  message->setPosition(sf::Vector2f(300.f, 200.f));

  Clock clock;
  float elapsed = 0.0f;
  float messageCooldown = 3;

  // We need a render surface to draw to so Swoosh ActivityController
  // can add screen transition effects from the title screen
  sf::RenderTexture loadSurface;

  // Use the engine's window settings for this platform to create a properly 
  // sized render surface...
  loadSurface.create(ENGINE.GetWindow()->getSize().x, ENGINE.GetWindow()->getSize().y, ENGINE.GetWindow()->getSettings());
  
  // Use our external render surface as the game's screen
  ENGINE.SetRenderSurface(loadSurface);

  // This loop is just for the chrono x message screen
  while (inConfigMessageState && ENGINE.Running()) {
    clock.restart();

    // Poll input
    INPUT.Update();

    // Prepare for next draw calls
    ENGINE.Clear();

    // If 3 seconds is over, exit this loop
    if (messageCooldown <= 0) {
      inConfigMessageState = false;
      messageCooldown = 0;
    }

    // Fade out 
    float alpha = std::min((messageCooldown)*255.f, 255.f);
    alertSprite.setColor(sf::Color((sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)alpha));
    message->setFillColor(sf::Color((sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)alpha));
    messageCooldown -= elapsed;

    // Draw the message
    ENGINE.Draw(alertSprite);
    ENGINE.Draw(message);

    // Flip the buffer 
    loadSurface.display();

    // Create a sf::Drawable from the buffer's texture data
    sf::Sprite postprocess(loadSurface.getTexture());

    // Draw it to the screen
    ENGINE.GetWindow()->draw(postprocess);
    
    // Show the screen
    ENGINE.GetWindow()->display();

    elapsed = static_cast<float>(clock.getElapsedTime().asSeconds());
  }

  // Cleanup
  ENGINE.Clear();

  // Title screen background
  // This will be loaded from the resource manager AFTER it's ready
  sf::Texture* bg = nullptr;
  sf::Texture* progs = nullptr;
  sf::Texture* cursor = nullptr;

  // List of frames
  FrameList progAnim;
  
  // Animator object uses frames to animate
  Animator animator;
  float progAnimProgress = 0.f;
  
  SpriteSceneNode bgSprite;
  SpriteSceneNode progSprite;
  SpriteSceneNode cursorSprite;
  float totalElapsed = 0.f;

  // When progress is equal to the totalObject count, we are 100% ready
  int totalObjects = (unsigned)TextureType::TEXTURE_TYPE_SIZE 
                   + (unsigned)AudioType::AUDIO_TYPE_SIZE 
                   + (unsigned)ShaderType::SHADER_TYPE_SIZE;
                   
  std::atomic<int> progress{0};
  std::atomic<int> navisLoaded{0};
  std::atomic<int> mobsLoaded{0};

  RunGraphicsInit(&progress);
  ENGINE.SetShader(nullptr);

#ifdef __ANDROID__
  loadSurface.setDefaultShader(&LOAD_SHADER(DEFAULT));
#endif

    //sf::Thread graphicsLoad(&RunGraphicsInit, &progress);
  sf::Thread audioLoad(&RunAudioInit, &progress);

  // We must deffer these threads until graphics and audio are finished
  sf::Thread navisLoad(&RunNaviInit, &navisLoaded);
  sf::Thread mobsLoad(&RunMobInit, &mobsLoaded);

  audioLoad.launch();

  // stream some music while we wait
  AUDIO.Stream("resources/loops/loop_theme.ogg");

  // Draw some log info while we wait
  bool inLoadState = true;
  bool ready = false;
  bool loadMobs = false;
  bool pressedStart = false;

  int selected = 0; // menu options are CONTINUE (0) and CONFIGURE (1)

  // When resources are loaded, flash the screen white
  double shaderCooldown = 2000;  // 2 seconds
  double logFadeOutTimer = 4000; // 4 seconds of idle before logs fade out
  double logFadeOutSpeed = 2000; // 2 seconds for logs to fade out

  // Because the graphics are loading, we may not have the shader yet
  // Point to null
  sf::Shader* whiteShader = nullptr;

  while(inLoadState && ENGINE.Running()) {
    clock.restart();
    
    INPUT.Update();

    // Set title bar to loading %
    float percentage = (float)progress / (float)totalObjects;
    std::string percentageStr = std::to_string((int)(percentage*100));
    ENGINE.GetWindow()->setTitle(sf::String(std::string("Loading: ") + percentageStr + "%"));

    // Show the mouse to the user
    sf::Vector2f mousepos = ENGINE.GetWindow()->mapPixelToCoords(sf::Mouse::getPosition(*ENGINE.GetWindow()));
    
    // Mouse fades out if not being used
    mouseAlpha -= elapsed/1000.0f;
    mouseAlpha = std::max(0.0, mouseAlpha);

    // Mouse shows up when touched
    if (mousepos != lastMousepos) {
      lastMousepos = mousepos;
      mouseAlpha = 1.0;
    }

    mouse.setPosition(mousepos);
    mouse.setColor(sf::Color(255, 255, 255, (sf::Uint8)(255 * mouseAlpha)));
    
    // Mouse blinks
    mouseAnimation.Update(elapsed/1000.0f, mouse);

    /*
      Get next logs. One at a time for effect.
    */
    std::string log;

    Logger::GetMutex()->lock();
    if(Logger::GetNextLog(log)) {
      logs.insert(logs.begin(), log);
    }
    Logger::GetMutex()->unlock();

    // If progress is equal to total resources, 
    // we can show graphics and load external data
    if (progress == totalObjects) {
      // read boolean is used to track if we loaded media 
      if (!ready) {
        ready = true;

        // Now that media is ready, we can launch the navis thread
        navisLoad.launch();
      }
      else { 
        // Else we may be ready this frame
        if (!bg) {
          // Load resources from internal storage
          try {
            bg = TEXTURES.GetTexture(TextureType::BACKGROUND_BLUE);
            bgSprite.setTexture(*bg);
            bgSprite.setScale(2.f, 2.f);
          }
          catch (std::exception e) {
            // didnt catchup? debug
          }
        }

        if (!progs) {
          // Load resources from internal storage
          try {
            progs = TEXTURES.GetTexture(TextureType::TITLE_ANIM_CHAR);

            progSprite.setTexture(*progs);
            progSprite.setPosition(200.f, 0.f);
            progSprite.setScale(2.f, 2.f);

            // This adds the title character frames to the FramesList 
            // to animate
            int i = 0;
            for (int x = 0; x < TITLE_ANIM_CHAR_SPRITES; x++) {
              progAnim.Add(1.f/(float)TITLE_ANIM_CHAR_SPRITES, sf::IntRect(TITLE_ANIM_CHAR_WIDTH*i, 0, TITLE_ANIM_CHAR_WIDTH, TITLE_ANIM_CHAR_HEIGHT));
              i++;
            }

          }
          catch (std::exception e) {
            // didnt catchup? debug
          }
        }

        if (!cursor) {
          // Load resources from internal storage
          try {
            cursor = TEXTURES.GetTexture(TextureType::TEXT_BOX_CURSOR);

            cursorSprite.setTexture(*cursor);
            cursorSprite.setPosition(sf::Vector2f(160.0f, 225.f));
            cursorSprite.setScale(2.f, 2.f);
          }
          catch (std::exception e) {
            // didnt catchup? debug
          }
        }

        if (!whiteShader) {
          try {
            whiteShader = SHADERS.GetShader(ShaderType::WHITE_FADE);
            whiteShader->setUniform("opacity", 0.0f);
            ENGINE.SetShader(whiteShader);
          }
          catch (std::exception e) {
            // didnt catchup? debug
          }
        }

        shaderCooldown -= elapsed;
        progAnimProgress += elapsed/2000.f;

        // If the white flash is less than zero
        // can cause unexpected visual effects
        if (shaderCooldown < 0) {
          shaderCooldown = 0;
        }

        // Adjust timers by elapsed time
        if (shaderCooldown == 0) {
          logFadeOutTimer -= elapsed;
        }

        if (logFadeOutTimer <= 0) {
          logFadeOutSpeed -= elapsed;
        }

        if (logFadeOutSpeed < 0) {
          logFadeOutSpeed = 0;
        }

        // keep animation in bounds
        if (progAnimProgress > 1.f) {
          progAnimProgress = 0.f;
        }

        // update white flash
        whiteShader->setUniform("opacity", (float)(shaderCooldown / 1000.f)*0.5f);
      }
    }

    // Prepare for next draw calls
    ENGINE.Clear();

    // if background is ready and loaded from threads...
    if (ready) {
      // show it
      ENGINE.Draw(&bgSprite);

      // Show the gamepad icon at the top-left if we have joystick support
      if (INPUT.IsJosytickAvailable()) {
        sf::Sprite gamePadICon(*TEXTURES.GetTexture(TextureType::GAMEPAD_SUPPORT_ICON));
        gamePadICon.setScale(2.f, 2.f);
        gamePadICon.setPosition(10.f, 5.0f);
        ENGINE.Draw(gamePadICon);
      }
    }

    // Draw logs on top of bg
    for (int i = 0; i < logs.size(); i++) {
      // fade from newest to oldest
      // newest at bottom full opacity
      // oldest at the top (at most 30 on screen) at full transparency
      logLabel->setString(logs[i]);
      logLabel->setPosition(0.f, 320 - (i * 10.f) - 15.f);
      logLabel->setFillColor(sf::Color(255, 255, 255, (sf::Uint8)((logFadeOutSpeed/2000.f)*std::fmax(0, 255 - (255 / 30)*i))));
      //ENGINE.Draw(logLabel);
    }

    if (progs) {
      // Animator the prog character at the title screen
      // and draw him if we have it loaded
      animator(progAnimProgress, progSprite, progAnim);
      ENGINE.Draw(&progSprite);
      ENGINE.Draw(&logoSprite);

      // If the progs resource is valid we know we are
      // loading navi and mob data. Check which one 
      // and display their loading %
      if (navisLoaded < (int)NAVIS.Size()) {
        navisLoadedLabel->setString(std::string("Loading Navi Data ") + std::to_string(navisLoaded) + " / " + std::to_string(NAVIS.Size()));
        sf::FloatRect bounds = navisLoadedLabel->getLocalBounds();
        sf::Vector2f origin = {bounds.width / 2.0f, bounds.height / 2.0f};
        navisLoadedLabel->setOrigin(origin);
        ENGINE.Draw(navisLoadedLabel);
      }
      else {
        // Else, navis are loaded, launch next thread and display mob %
        if (mobsLoaded < (int)MOBS.Size()) {
          if (!loadMobs) {
            loadMobs = true;
            mobsLoad.launch();
          }
          else {
            mobLoadedLabel->setString(std::string("Loading Mob Data ") + std::to_string(mobsLoaded) + " / " + std::to_string(MOBS.Size()));
            sf::FloatRect bounds = mobLoadedLabel->getLocalBounds();
            sf::Vector2f origin = { bounds.width / 2.0f, bounds.height / 2.0f };
            mobLoadedLabel->setOrigin(origin);
            ENGINE.Draw(mobLoadedLabel);
          }
        }
        else {
          // Finally everything is loaded

          if (!pressedStart) {
            // Finally everything is loaded, show "Press Start"
            ENGINE.Draw(startLabel);

            bool shouldStart = INPUT.Has(RELEASED_START);

#ifdef __ANDROID__
            shouldStart = sf::Touch::isDown(0);
#endif
            if (shouldStart) {
              pressedStart = true;
              AUDIO.Play(AudioType::CHIP_CHOOSE);

            }
          }
          else {
            // darken bg and title card like the game
            auto interpol = int(swoosh::ease::interpolate((elapsed/1000.0f)*3.0f, float(bgSprite.getColor().r), 105.f));
            interpol = std::max(105, interpol);

            sf::Color darken = sf::Color(interpol, interpol, interpol);
            bgSprite.setColor(darken);
            progSprite.setColor(darken);
            logoSprite.setColor(darken);

            auto offset = std::abs((std::sin(totalElapsed / 100.0f)*4.f));

            if (selected == 0) {
              cursorSprite.setPosition(sf::Vector2f(163.0f + offset, 227.f));
            }
            else {
              cursorSprite.setPosition(sf::Vector2f(163.0f + offset, 257.f));
            }

            ENGINE.Draw(cursorSprite);

            // Show continue or settings options
            startLabel->setString("CONTINUE");
            startLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
            startLabel->setPosition(sf::Vector2f(200.0f, 240.f));
            ENGINE.Draw(startLabel);

            startLabel->setString("CONFIGURE");
            startLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
            startLabel->setPosition(sf::Vector2f(200.0f, 270.f));
            ENGINE.Draw(startLabel);

            bool shouldStart = INPUT.Has(RELEASED_START);

            if (INPUT.Has(PRESSED_UP)) {
              if (selected != 0) {
                AUDIO.Play(AudioType::CHIP_SELECT);
              }

              selected = 0;
            }
            else if (INPUT.Has(PRESSED_DOWN)) {
              if (selected != 1) {
                AUDIO.Play(AudioType::CHIP_SELECT);
              }

              selected = 1;
            }

#ifdef __ANDROID__
            shouldStart = sf::Touch::isDown(0);
#endif
            if (shouldStart) {
              inLoadState = false;
              AUDIO.Play(AudioType::NEW_GAME);
            }
          }
        }
      }
    }

    loadSurface.display();

    sf::Sprite postprocess(loadSurface.getTexture());

    auto states = sf::RenderStates::Default;
    //states.transform.scale(4.f,4.f);

#ifdef __ANDROID__
    states.shader = SHADERS.GetShader(ShaderType::DEFAULT);
#endif

    ENGINE.GetWindow()->draw(postprocess, states);

#ifndef __ANDROID__
    ENGINE.GetWindow()->draw(mouse, states);
#endif

    // Finally, everything is drawn to window buffer, display it to screen
    ENGINE.GetWindow()->display();

    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
    totalElapsed += elapsed;
  }

    // Do not clear the Engine's render surface
    // Instead grab a copy of it first
    // So we can use it in screen transitions
    // provided by Swoosh Activity Controller
  sf::Texture loadingScreenSnapshot = ENGINE.GetRenderSurface().getTexture();

#ifdef __ANDROID__
    loadingScreenSnapshot.flip(true);
#endif

  // Cleanup
  ENGINE.RevokeShader();
  ENGINE.Clear();
  delete mobLoadedLabel;
  delete navisLoadedLabel;

  //delete logLabel;
  //delete font;
  delete logo;

  // Stop music and go to menu screen
  AUDIO.StopStream();

  // Create an activity controller
  // Behaves like a state machine using stacks
  // The activity controller uses a virtual window
  // To draw screen transitions onto
  sf::Vector2u virtualWindowSize(480, 320);
  ActivityController app(*ENGINE.GetWindow(), virtualWindowSize);

  // The last screen the player will see is the game over screen
  app.push<GameOverScene>();

  // We want the next screen to be the main menu screen
  app.push<MainMenuScene>();

  if (selected > 0) {
    app.push<ConfigScene>();
  }

  // This scene is designed to immediately pop off the stack
  // and segue into the previous scene on the stack: MainMenuScene
  // It takes a snapshot of the loading/title screen
  // And draws it with supported transition effects
  app.push<FakeScene>(loadingScreenSnapshot);

  double remainder = 0;
  elapsed = 0;

  srand((unsigned int)time(nullptr));

  logLabel->setFillColor(sf::Color::Red);
  logLabel->setPosition(296,18);
  logLabel->setStyle(sf::Text::Style::Bold);

  // Make sure we didn't quit the loop prematurely
  while (ENGINE.Running()) {
      // Non-simulation
      elapsed = static_cast<float>(clock.restart().asSeconds()) + static_cast<float>(remainder);

      INPUT.Update();

      float FPS = 0.f;

      FPS = (float) (1.0 / (float) elapsed);
      std::string fpsStr = std::to_string(FPS);
      fpsStr.resize(4);
      ENGINE.GetWindow()->setTitle(sf::String(std::string("FPS: ") + fpsStr));

      logLabel->setString(sf::String(std::string("FPS: ") + fpsStr));

      // Use the activity controller to update and draw scenes
      app.update((float) FIXED_TIME_STEP);

      sf::Vector2f mousepos = ENGINE.GetWindow()->mapPixelToCoords(sf::Mouse::getPosition(*ENGINE.GetWindow()));
      mouseAlpha -= FIXED_TIME_STEP;
      mouseAlpha = std::max(0.0, mouseAlpha);

      if (mousepos != lastMousepos) {
          lastMousepos = mousepos;
          mouseAlpha = 1.0;
      }

      mouse.setPosition(mousepos);
      mouse.setColor(sf::Color(255, 255, 255, (sf::Uint8) (255 * mouseAlpha)));
      mouseAnimation.Update((float) FIXED_TIME_STEP, mouse);

      ENGINE.Clear();

      auto states = sf::RenderStates::Default;
      //states.transform.scale(4.f,4.f);

#ifdef __ANDROID__
      states.shader = SHADERS.GetShader(ShaderType::DEFAULT);
#endif 

      app.draw(loadSurface);
      loadSurface.display();

      sf::Sprite toScreen(loadSurface.getTexture());
      ENGINE.GetWindow()->draw(toScreen, states);

#ifdef __ANDROID__
      ENGINE.GetWindow()->draw(*logLabel, states);
#else
      ENGINE.GetWindow()->draw(mouse, states);
#endif

      ENGINE.GetWindow()->display();

  }
  delete mouseTexture;
  delete logLabel;
  delete font;

  return EXIT_SUCCESS;
}
