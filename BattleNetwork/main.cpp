#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnNaviRegistration.h"
#include "bnInputManager.h"
#include "bnEngine.h"
#include "bnGameOverScene.h"
#include "bnMainMenuScene.h"
#include "bnFakeScene.h"
#include "bnAnimate.h"
#include "bnChronoXConfigReader.h"
#include "Android/bnTouchArea.h"
#include "SFML/System.hpp"

#include <time.h>
#include <queue>
#include <atomic>
#include <cmath>
#include <Swoosh/ActivityController.h>

// #define BN_REGION_JAPAN 1

// Engine addons
#include "bnQueueNaviRegistration.h"
#include "bnQueueMobRegistration.h"

// Timer
using sf::Clock;

// Swoosh activity management
using swoosh::ActivityController;

// Title card character
#define TITLE_ANIM_CHAR_SPRITES 14
#define TITLE_ANIM_CHAR_WIDTH 128
#define TITLE_ANIM_CHAR_HEIGHT 221

#define FIXED_TIME_STEP 1.0f/60.0f

void RunNaviInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();

  NAVIS.LoadAllNavis(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded registered navis: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();
}

void RunMobInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();

  MOBS.LoadAllMobs(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded registered mobs: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();
}


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

void RunAudioInit(std::atomic<int> * progress) {
  const clock_t begin_time = clock();
  AUDIO.LoadAllSources(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded audio sources: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  Logger::GetMutex()->unlock();
}

int main(int argc, char** argv) {
  // Render context must:
  //                    1) always run from main thread and 
  //                    2) load before we do any loading screen rendering
  const clock_t begin_time = clock();
  ENGINE.Initialize();
  Logger::Logf("Engine initialized: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);

  // lazy init
  TEXTURES;
  SHADERS;
  AUDIO;
  QueuNaviRegistration(); // Queues navis to be loaded later
  QueueMobRegistration(); // Queues mobs to be loaded later

  // State flags
  bool inConfigMessageState = true;

  ChronoXConfigReader config("options.ini");

  if (!config.IsOK()) {
    inConfigMessageState = false; // skip the state
  }
  else {
    AUDIO.EnableAudio(config.IsAudioEnabled());
    INPUT.SupportChronoXGamepad(config);
  }

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
  mouseAnimation << Animate::Mode::Loop;
  sf::Vector2f lastMousepos;
  double mouseAlpha = 1.0;

  // Title screen logo

  #ifdef BN_REGION_JAPAN
  sf::Texture* logo = TEXTURES.LoadTextureFromFile("resources/backgrounds/title/tile.png");
  #else
  sf::Texture* logo = TEXTURES.LoadTextureFromFile("resources/backgrounds/title/tile_en.png");
  #endif

  LayeredDrawable logoSprite;
  logoSprite.setTexture(*logo);
  logoSprite.setOrigin(logoSprite.getLocalBounds().width / 2, logoSprite.getLocalBounds().height / 2);
  sf::Vector2f logoPos = (sf::Vector2f)((sf::Vector2i)ENGINE.GetWindow()->getSize() / 2);
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

  sf::Text* navisLoadedLabel = new sf::Text("Loading Navi Data...", *startFont);
  navisLoadedLabel->setCharacterSize(24);
  navisLoadedLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
  navisLoadedLabel->setPosition(sf::Vector2f(230.f, 230.f));

  sf::Text* mobLoadedLabel = new sf::Text("Loading Mob Data...", *startFont);
  mobLoadedLabel->setCharacterSize(24);
  mobLoadedLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
  mobLoadedLabel->setPosition(sf::Vector2f(230.f, 230.f));
  /* 
  Give a message to the player before loading 
  */

  sf::Text* message = new sf::Text("Your Chrono X config settings\nhave been imported", *startFont);
  message->setCharacterSize(24);
  message->setOrigin(message->getLocalBounds().width/2.f, message->getLocalBounds().height*2);
  message->setPosition(sf::Vector2f(300.f, 200.f));

  Clock clock;
  float elapsed = 0.0f;
  float messageCooldown = 3; 

  sf::RenderTexture loadSurface;
  //loadSurface.create(480, 320);
  loadSurface.create(ENGINE.GetWindow()->getSize().x, ENGINE.GetWindow()->getSize().y, ENGINE.GetWindow()->getSettings());
  ENGINE.SetRenderSurface(loadSurface);

  while (inConfigMessageState && ENGINE.Running()) {
    clock.restart();

    INPUT.Update();
    
    // Prepare for next draw calls
    ENGINE.Clear();

    // Write contents to screen
    // ENGINE.Display();

    if (messageCooldown <= 0) {
      inConfigMessageState = false;
      messageCooldown = 0;
    }

    float alpha = std::min((messageCooldown)*255.f, 255.f);
    alertSprite.setColor(sf::Color((sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)alpha));
    message->setFillColor(sf::Color((sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)alpha));
    messageCooldown -= elapsed;

    ENGINE.Draw(alertSprite);
    ENGINE.Draw(message);

    loadSurface.display();

    sf::Sprite postprocess(loadSurface.getTexture());

    ENGINE.GetWindow()->draw(postprocess);
    ENGINE.GetWindow()->display();

    elapsed = static_cast<float>(clock.getElapsedTime().asSeconds());
  }

  // Cleanup
  ENGINE.Clear();

  // Title screen background
  // This will be loaded from the resource manager AFTER it's ready
  sf::Texture* bg = nullptr;
  sf::Texture* progs = nullptr;
  FrameList progAnim;
  Animate animator;
  float progAnimProgress = 0.f;
  LayeredDrawable bgSprite;
  LayeredDrawable progSprite;

  int totalObjects = (unsigned)TextureType::TEXTURE_TYPE_SIZE + (unsigned)AudioType::AUDIO_TYPE_SIZE + (unsigned)ShaderType::SHADER_TYPE_SIZE;
  std::atomic<int> progress{0};
  std::atomic<int> navisLoaded{0};
  std::atomic<int> mobsLoaded{0};

  RunGraphicsInit(&progress);
  ENGINE.SetShader(nullptr);
  //sf::Thread graphicsLoad(&RunGraphicsInit, &progress);
  sf::Thread audioLoad(&RunAudioInit, &progress);

  // We must deffer the thread until graphics and audio are finished
  sf::Thread navisLoad(&RunNaviInit, &navisLoaded);
  sf::Thread mobsLoad(&RunMobInit, &mobsLoaded);

  //graphicsLoad.launch();
  audioLoad.launch();

  // play some music while we wait
  AUDIO.SetStreamVolume(10);
  AUDIO.Stream("resources/loops/loop_theme.ogg");

  // Draw some stats while we wait 
  bool inLoadState = true;
  bool ready = false;
  bool loadMobs = false;

  double shaderCooldown = 2000; // 2 seconds
  double logFadeOutTimer = 4000;
  double logFadeOutSpeed = 2000;

  sf::Shader* whiteShader = nullptr;

  while(inLoadState && ENGINE.Running()) {
    clock.restart();
    
    INPUT.Update();

    float percentage = (float)progress / (float)totalObjects;
    std::string percentageStr = std::to_string((int)(percentage*100));
    ENGINE.GetWindow()->setTitle(sf::String(std::string("Loading: ") + percentageStr + "%"));

    sf::Vector2f mousepos = ENGINE.GetWindow()->mapPixelToCoords(sf::Mouse::getPosition(*ENGINE.GetWindow()));
    mouseAlpha -= elapsed/1000.0f;
    mouseAlpha = std::max(0.0, mouseAlpha);

    if (mousepos != lastMousepos) {
      lastMousepos = mousepos;
      mouseAlpha = 1.0;
    }

    mouse.setPosition(mousepos);
    mouse.setColor(sf::Color(255, 255, 255, (sf::Uint8)(255 * mouseAlpha)));
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


    if (progress == totalObjects) {
      if (!ready) {
        ready = true;

        navisLoad.launch();
      }
      else { // Else we are ready next frame
        if (!bg) {
          // NOW we can load resources from internal storage throughout the game
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
          // NOW we can load resources from internal storage throughout the game
          try {
            progs = TEXTURES.GetTexture(TextureType::TITLE_ANIM_CHAR);

            progSprite.setTexture(*progs);
            progSprite.setPosition(200.f, 0.f);
            progSprite.setScale(2.f, 2.f);

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

        if (shaderCooldown < 0) {
          shaderCooldown = 0;
        }

        // Just a bunch of timers for events on screen
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

        // update shader 
        whiteShader->setUniform("opacity", (float)(shaderCooldown / 1000.f)*0.5f);
      }

      bool shouldStart = INPUT.Has(PRESSED_START);

#ifdef __ANDROID__
        shouldStart = sf::Touch::isDown(0);
#endif
        if (shouldStart && navisLoaded == NAVIS.Size()) {
        inLoadState = false;
      }
    }

    // Prepare for next draw calls
    ENGINE.Clear();

    // if background is ready and loaded from threads...
    if (ready) {
      // show it 
      ENGINE.Draw(&bgSprite);

      if (INPUT.HasChronoXGamepadSupport()) {
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
      ENGINE.Draw(logLabel);
    }

    if (progs) {
      animator(progAnimProgress, progSprite, progAnim);
      ENGINE.Draw(&progSprite);

      if (navisLoaded < (int)NAVIS.Size()) {
        navisLoadedLabel->setString(std::string("Loading Navi Data ") + std::to_string(navisLoaded) + " / " + std::to_string(NAVIS.Size()));
        sf::FloatRect bounds = navisLoadedLabel->getLocalBounds();
        sf::Vector2f origin = {bounds.width / 2.0f, bounds.height / 2.0f};
        navisLoadedLabel->setOrigin(origin);
        ENGINE.Draw(navisLoadedLabel);
      }
      else {
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
          INPUT.Update();
          ENGINE.Draw(startLabel);
        }
      }
    }

    // TODO BC: uncomment
    //ENGINE.Draw(&logoSprite);
    //ENGINE.DrawUnderlay();
    //ENGINE.DrawLayers();
    //ENGINE.DrawOverlay();

    loadSurface.display();

    sf::Sprite postprocess(loadSurface.getTexture());

    auto state = sf::RenderStates::Default;
    state.shader = SHADERS.GetShader(ShaderType::DEFAULT);

    ENGINE.GetWindow()->draw(postprocess, state);
    ENGINE.GetWindow()->draw(mouse, state);
    ENGINE.GetWindow()->display();

    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
  }

  sf::Texture loadingScreenSnapshot;; // = ENGINE.GetRenderSurface().getTexture();

  // Cleanup
  ENGINE.RevokeShader();
  ENGINE.Clear();
  delete mobLoadedLabel;
  delete navisLoadedLabel;
  delete logLabel;
  delete font;
  delete logo;

  // Stop music and go to menu screen 
  AUDIO.StopStream();

  // Create an activity controller 
  // Behaves like a state machine using stacks
  sf::Vector2u virtualWindowSize(480, 320);
  ActivityController app(*ENGINE.GetWindow(), virtualWindowSize);
  app.push<GameOverScene>();
  app.push<MainMenuScene>();

  // This scene will immediately pop off the stack 
  // and segue into the previous scene on the stack: MainMenuScene
  app.push<FakeScene>(loadingScreenSnapshot);

  double remainder = 0;
  elapsed = 0;

  srand((unsigned int)time(nullptr));

#ifdef __ANDROID__
  /* Android touch areas*/
  TouchArea& rightSide = TouchArea::create(sf::IntRect(240, 0, 240, 320));

  rightSide.enableExtendedRelease(true);
  bool releasedB = false;

  rightSide.onTouch([]() {
      INPUT.VirtualKeyEvent(InputEvent::RELEASED_A);
  });

  rightSide.onRelease([&releasedB](sf::Vector2i delta) {
      INPUT.VirtualKeyEvent(InputEvent::PRESSED_A);
      releasedB = false;

  });

  rightSide.onDrag([&releasedB](sf::Vector2i delta){
      if(delta.x < -40 && !releasedB) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_B);
        INPUT.VirtualKeyEvent(InputEvent::RELEASED_B);
        releasedB = true;
      }
  });

  rightSide.onDefault([&releasedB]() {
      releasedB = false;
  });

  TouchArea& custSelectButton = TouchArea::create(sf::IntRect(0, 0, 480, 100));
  custSelectButton.onTouch([]() {
      INPUT.VirtualKeyEvent(InputEvent::PRESSED_START);
  });
  custSelectButton.onRelease([](sf::Vector2i delta) {
      INPUT.VirtualKeyEvent(InputEvent::RELEASED_START);
  });

  TouchArea& dpad = TouchArea::create(sf::IntRect(0, 0, 240, 320));
  dpad.enableExtendedRelease(true);
  dpad.onDrag([](sf::Vector2i delta) {
      Logger::Log("dpad delta: " + std::to_string(delta.x) + ", " + std::to_string(delta.y));

    if(delta.x > 40) {
      INPUT.VirtualKeyEvent(InputEvent::PRESSED_RIGHT);
    }

    if(delta.x < -40) {
      INPUT.VirtualKeyEvent(InputEvent::PRESSED_LEFT);
    }

    if(delta.y > 40) {
      INPUT.VirtualKeyEvent(InputEvent::PRESSED_DOWN);
    }

    if(delta.y < -40) {
      INPUT.VirtualKeyEvent(InputEvent::PRESSED_UP);
    }
  });
#endif

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


      // Use the activity controller to update and draw scenes
      app.update((float) FIXED_TIME_STEP);

      sf::Vector2f mousepos = ENGINE.GetWindow()->mapPixelToCoords(
              sf::Mouse::getPosition(*ENGINE.GetWindow()));
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
      ENGINE.DrawUnderlay();
      ENGINE.DrawLayers();
      ENGINE.DrawOverlay();

      auto state = sf::RenderStates::Default;
      state.shader = SHADERS.GetShader(ShaderType::DEFAULT);

      app.draw();

      ENGINE.GetWindow()->draw(mouse, states);

      ENGINE.GetWindow()->display();

  }
  delete mouseTexture;

#if defined(__ANDROID__)
TouchArea::free();
#endif

  return EXIT_SUCCESS;
}
