#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnNaviRegistration.h"
#include "bnInputManager.h"
#include "bnEngine.h"
#include "bnMainMenuScene.h"
#include "bnAnimate.h"
#include "bnChronoXConfigReader.h"
#include "SFML/System.hpp"
#include "SFML/Window/Touch.hpp"
#include <time.h>
#include <queue>
#include <atomic>
#include <cmath>

// Engine addons
#include "bnQueueNaviRegistration.h"

// Timer
using sf::Clock;

// Title card character
#define TITLE_ANIM_CHAR_SPRITES 14
#define TITLE_ANIM_CHAR_WIDTH 128
#define TITLE_ANIM_CHAR_HEIGHT 221
#define SHADER_FRAG_WHITE_PATH "resources/shaders/white_fade.frag.txt"

void RunNaviInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();

  NAVIS.LoadAllNavis(*progress);

  Logger::GetMutex()->lock();
  Logger::Logf("Loaded registered navis: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
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
  Logger::Logf("Shader size is %i", ShaderResourceManager::GetInstance().GetSize());
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
  sf::Keyboard::setVirtualKeyboardVisible(true);

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

  // Title screen logo
  sf::Texture* logo = TEXTURES.LoadTextureFromFile("resources/backgrounds/title/tile_en.png");
  LayeredDrawable logoSprite;
  logoSprite.setTexture(*logo);
  logoSprite.setOrigin(logoSprite.getLocalBounds().width / 2, logoSprite.getLocalBounds().height / 2);
  sf::Vector2f logoPos = (sf::Vector2f)((sf::Vector2i)ENGINE.GetWindow()->getSize() / 2);
  logoSprite.setPosition(logoPos);

  // Log output text
  sf::Font* font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthin_regular.ttf");
  sf::Text* logLabel = new sf::Text("...", *font);
  logLabel->setCharacterSize(10);
  logLabel->setOrigin(0.f, logLabel->getLocalBounds().height);
  std::vector<std::string> logs;

  // Press Start text
  sf::Font* startFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  sf::Text* startLabel = new sf::Text("PRESS START", *startFont);
  startLabel->setCharacterSize(24);
  startLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
  startLabel->setPosition(sf::Vector2f(180.0f, 240.f));

  sf::Text* navisLoadedLabel = new sf::Text("Loading Navi Data...", *startFont);
  navisLoadedLabel->setCharacterSize(24);
  navisLoadedLabel->setOrigin(0.f, startLabel->getLocalBounds().height);
  navisLoadedLabel->setPosition(sf::Vector2f(230.f, 230.f));

  /* 
  Give a message to the player before loading 
  */

  sf::Text* message = new sf::Text("Your Chrono X config settings\nhave been imported", *startFont);
  message->setCharacterSize(24);
  message->setOrigin(message->getLocalBounds().width/2.f, message->getLocalBounds().height*2);
  message->setPosition(sf::Vector2f(300.f, 200.f));

  Clock clock;
  float elapsed = 0.0f;
  float messageCooldown = 3000; 

  while (inConfigMessageState && ENGINE.Running()) {
    clock.restart();

    INPUT.update();

    // Prepare for next draw calls
    ENGINE.Clear();

    // Write contents to screen
    ENGINE.Display();

    if (messageCooldown <= 0) {
      inConfigMessageState = false;
      messageCooldown = 0;
    }

    float alpha = std::min((messageCooldown/1000.f)*255.f, 255.f);
    alertSprite.setColor(sf::Color((sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)alpha));
    message->setFillColor(sf::Color((sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)255.f, (sf::Uint8)alpha));
    messageCooldown -= elapsed;

    ENGINE.Draw(alertSprite);
    ENGINE.Draw(message);

    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
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

  int totalObjects = (unsigned)TextureType::TEXTURE_TYPE_SIZE + (unsigned)AudioType::AUDIO_TYPE_SIZE  + (unsigned)ShaderType::SHADER_TYPE_SIZE;
  std::atomic<int> progress(0);
  std::atomic<int> navisLoaded(0);

  // Mav Note: Currently do not have the default frag and vertex shaders
  totalObjects -= 2;

  sf::Thread graphicsLoad(&RunGraphicsInit, &progress);
  sf::Thread audioLoad(&RunAudioInit, &progress);

  // We must deffer the thread until graphics and audio are finished
  sf::Thread navisLoad(&RunNaviInit, &navisLoaded);

  graphicsLoad.launch();
  audioLoad.launch();

  // play some music while we wait
  AUDIO.SetStreamVolume(10);
  AUDIO.Stream("resources/loops/loop_theme.ogg", true);

  // Draw some stats while we wait 
  bool inLoadState = true;
  bool ready = false;

  double shaderCooldown = 2000; // 2 seconds
  double logFadeOutTimer = 4000;
  double logFadeOutSpeed = 2000;

  sf::Shader* whiteShader = nullptr;

  while(inLoadState && ENGINE.Running()) {
    clock.restart();

    float percentage = (float)progress / (float)totalObjects;
    std::string percentageStr = std::to_string((int)(percentage*100));
    ENGINE.GetWindow()->setTitle(sf::String(std::string("Loading: ") + percentageStr + "%"));

    INPUT.update();

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
            //whiteShader = SHADERS.GetShader(ShaderType::WHITE_FADE);
            //whiteShader->setUniform("opacity", 0.0f);
            //ENGINE.SetShader(whiteShader);
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
        //whiteShader->setUniform("opacity", (float)(shaderCooldown / 1000.f)*0.5f);
      }

      if ((sf::Touch::isDown(0) || INPUT.has(PRESSED_START)) && navisLoaded == NAVIS.Size()) {
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
        ENGINE.Draw(startLabel);
      }
    }

    ENGINE.Draw(&logoSprite);

    ENGINE.DrawUnderlay();
    ENGINE.DrawLayers();
    ENGINE.DrawOverlay();

    // Write contents to screen
    ENGINE.Display();

    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
  }

  // Cleanup
  ENGINE.RevokeShader();
  ENGINE.Clear();
  delete logLabel;
  delete font;
  delete logo;

  // Stop music and go to select screen 
  AUDIO.StopStream();

  // Make sure we didn't quit the loop prematurely
  while (ENGINE.Running()) {
    int win = MainMenuScene::Run(); // MainMenuScene::Run();

    if (win != 1) {
      // Start the game over music
      AUDIO.Stream("resources/loops/game_over.ogg");
      ENGINE.Clear();
      break;
    }
  }

  sf::Sprite gameOver;
  gameOver.setTexture(*TEXTURES.GetTexture(TextureType::GAME_OVER));
  gameOver.setScale(2.f, 2.f);
  gameOver.setOrigin(gameOver.getLocalBounds().width / 2, gameOver.getLocalBounds().height / 2);
  gameOver.setPosition(logoPos);
  float fadeInCooldown = 500.0f; // half a second

  // Show gameover screen
  while (ENGINE.Running()) {
    clock.restart();

    INPUT.update();

    fadeInCooldown -= elapsed;

    if (fadeInCooldown < 0) {
      fadeInCooldown = 0;
    }

    gameOver.setColor(sf::Color(255, 255, 255, (sf::Uint32)(255 - (255 * (fadeInCooldown / 500.f)))));
    ENGINE.Draw(gameOver);

    // Draw loop
    ENGINE.DrawUnderlay();
    ENGINE.DrawLayers();
    ENGINE.DrawOverlay();

    // Write contents to screen
    ENGINE.Display();

    // Prepare for next render 
    ENGINE.Clear();

    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());

  }
  
  return EXIT_SUCCESS;
}
