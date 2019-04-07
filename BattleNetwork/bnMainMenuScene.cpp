#include <time.h>
#include "bnMainMenuScene.h"
#include "bnFolderScene.h"
#include "bnOverworldMap.h"
#include "bnInfiniteMap.h"
#include "bnSelectNaviScene.h"
#include "bnSelectMobScene.h"
#include "bnMemory.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnNaviRegistration.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

int MainMenuScene::Run()
{
  Camera camera(ENGINE.GetDefaultView());
  ENGINE.SetCamera(camera);

  bool showHUD = true;

  // Selection input delays
  double maxSelectInputCooldown = 0.5; // half of a second
  double selectInputCooldown = maxSelectInputCooldown;        

  // ui sprite maps
  sf::Sprite ui(LOAD_TEXTURE(MAIN_MENU_UI));
  ui.setScale(2.f, 2.f);
  Animation uiAnimator("resources/ui/main_menu_ui.animation");
  uiAnimator.Load();

  // Stream menu music 
  AUDIO.Stream("resources/loops/loop_navi_customizer.ogg", true);

  // Transition
  sf::Shader& transition = LOAD_SHADER(TRANSITION);
  transition.setUniform("texture", sf::Shader::CurrentTexture);
  transition.setUniform("map", LOAD_TEXTURE(NOISE_TEXTURE));
  transition.setUniform("progress", 0.f);
  float transitionProgress = 0.9f;
  ENGINE.RevokeShader();

  Clock clock;
  float elapsed = 0.0f;
  float totalTime = 0.f;
  int menuSelectionIndex = 0;

  sf::Sprite overlay(LOAD_TEXTURE(MAIN_MENU));
  overlay.setScale(2.f, 2.f);

  sf::Sprite ow(LOAD_TEXTURE(MAIN_MENU_OW));
  ow.setScale(2.f, 2.f);

  Background* bg = new LanBackground();

  Overworld::Map* map = new Overworld::InfiniteMap(10, 20, 47, 24);
  map->SetCamera(&camera);

  // Keep track of selected navi
  SelectedNavi currentNavi = 0;

  sf::Sprite owNavi(LOAD_TEXTURE(NAVI_MEGAMAN_ATLAS));
  owNavi.setScale(2.f, 2.f);
  owNavi.setPosition(0, 0.f);
  Animation naviAnimator("resources/navis/megaman/megaman.animation");
  naviAnimator.Load();
  naviAnimator.SetAnimation("PLAYER_OW_RD");
  naviAnimator << Animate::Mode(Animate::Mode::Loop);

  map->AddSprite(&owNavi);

  bool gotoNextScene = false;

  while (ENGINE.Running()) {

    float elapsedSeconds = clock.restart().asSeconds();
    totalTime += elapsedSeconds;

    float FPS = 0.f;

    if (elapsedSeconds > 0.f) {
      FPS = 1.0f / elapsedSeconds;
      std::string fpsStr = std::to_string(FPS);
      fpsStr.resize(4);
      ENGINE.GetWindow()->setTitle(sf::String(std::string("FPS: ") + fpsStr));
    }

    INPUT.update();
    map->Update(elapsed);

    ENGINE.Clear();

    camera.Update(elapsed);
    bg->Update(elapsed);

    ENGINE.Draw(bg);
    ENGINE.Draw(map);

    // Draw navi moving
    naviAnimator.Update(elapsed, &owNavi);

    int lastMenuSelectionIndex = menuSelectionIndex;

    // Move the navi down
    owNavi.setPosition(owNavi.getPosition() + sf::Vector2f(50.0f*elapsed, 0));

    // TODO: fix this broken camera system
    sf::Vector2f camOffset = camera.GetView().getSize();
    camOffset.x /= 5;
    camOffset.y /= 3.5;

    // Follow the navi
    camera.PlaceCamera(map->ScreenToWorld(owNavi.getPosition() - sf::Vector2f(0.5, 0.5)) + camOffset);
    
    if (!gotoNextScene && transitionProgress == 0.f) {
      if (INPUT.has(PRESSED_UP)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          // Go to previous mob 
          selectInputCooldown = maxSelectInputCooldown;
          menuSelectionIndex--;
        }
      }
      else if (INPUT.has(PRESSED_DOWN)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          // Go to next mob 
          selectInputCooldown = maxSelectInputCooldown;
          menuSelectionIndex++;
        }
      }
      else {
        selectInputCooldown = 0;
      }
      
      if (INPUT.has(PRESSED_A) || sf::Touch::isDown(0)) {

        // Folder Select
        if (menuSelectionIndex == 1) {
          gotoNextScene = true;
          AUDIO.Play(AudioType::CHIP_DESC);
        }

        // Navi select
        if (menuSelectionIndex == 2) {
          gotoNextScene = true;
          AUDIO.Play(AudioType::CHIP_DESC);
        }

        // Mob select
        if (menuSelectionIndex == 3 || sf::Touch::isDown(0)) {
          gotoNextScene = true;
          menuSelectionIndex = 3;
          AUDIO.Play(AudioType::CHIP_DESC);
        }
      }
    } 

    /*if (INPUT.has(PRESSED_PAUSE)) {
      static bool toggle = false;
      toggle = !toggle;
      showHUD = false;
      map->ToggleLighting(toggle);
    }*/

    if (elapsed > 0) {
      if (gotoNextScene) {
        transitionProgress += 1 * elapsed;
      }
      else {
        transitionProgress -= 1 * elapsed;
      }
    }

    transitionProgress = std::max(0.f, transitionProgress);
    transitionProgress = std::min(1.f, transitionProgress);

    if (transitionProgress >= 1.f) {

      if (menuSelectionIndex == 1) {
        int result = FolderScene::Run();

        // reset internal clock (or everything will teleport)
        elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
        std::cout << "time slept: " << elapsed << "\n";
        clock.restart();
        elapsed = 0;

        if (result == 0) {
          break; // Breaks the while-loop
        }
      } else if (menuSelectionIndex == 2) {
        currentNavi = SelectNaviScene::Run(currentNavi);

        owNavi.setTexture(NAVIS.At(currentNavi).GetOverworldTexture());
        naviAnimator = Animation(NAVIS.At(currentNavi).GetOverworldAnimationPath());
        naviAnimator.Load();
        naviAnimator.SetAnimation("PLAYER_OW_RD");
        naviAnimator << Animate::Mode(Animate::Mode::Loop);

        // reset internal clock (or everything will teleport)
        elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
        std::cout << "time slept: " << elapsed << "\n";
        clock.restart();
        elapsed = 0;

      } else if (menuSelectionIndex == 3) {
        int result = SelectMobScene::Run(currentNavi);

        // reset internal clock (or everything will teleport)
        elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
        std::cout << "time slept: " << elapsed << "\n";
        clock.restart();
        elapsed = 0;

        if (result == 0) {
          break; // Breaks the while-loop
        }
      } 

      gotoNextScene = false;
    }

    menuSelectionIndex = std::max(0, menuSelectionIndex);
    menuSelectionIndex = std::min(3, menuSelectionIndex);

    
    if (menuSelectionIndex != lastMenuSelectionIndex) {
      AUDIO.Play(AudioType::CHIP_SELECT);
    }

    ENGINE.Draw(overlay);

    if (showHUD) {
      uiAnimator.SetAnimation("CHIP_FOLDER");

      if (menuSelectionIndex == 0) {
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(50.f, 50.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(100.f, 50.f);
        ENGINE.Draw(ui);
      }
      else {
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(20.f, 50.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(100.f, 50.f);
        ENGINE.Draw(ui);
      }

      uiAnimator.SetAnimation("LIBRARY");

      if (menuSelectionIndex == 1) {
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(50.f, 120.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("LIBRARY_LABEL");
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(100.f, 120.f);
        ENGINE.Draw(ui);
      }
      else {
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(20.f, 120.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("LIBRARY_LABEL");
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(100.f, 120.f);
        ENGINE.Draw(ui);
      }

      uiAnimator.SetAnimation("NAVI");

      if (menuSelectionIndex == 2) {
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(50.f, 190.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("NAVI_LABEL");
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(100.f, 190.f);
        ENGINE.Draw(ui);
      }
      else {
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(20.f, 190.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("NAVI_LABEL");
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(100.f, 190.f);
        ENGINE.Draw(ui);
      }

      uiAnimator.SetAnimation("MOB_SELECT");

      if (menuSelectionIndex == 3) {
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(50.f, 260.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("MOB_SELECT_LABEL");
        uiAnimator.SetFrame(2, &ui);
        ui.setPosition(100.f, 260.f);
        ENGINE.Draw(ui);
      }
      else {
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(20.f, 260.f);
        ENGINE.Draw(ui);

        uiAnimator.SetAnimation("MOB_SELECT_LABEL");
        uiAnimator.SetFrame(1, &ui);
        ui.setPosition(100.f, 260.f);
        ENGINE.Draw(ui);
      }

      ENGINE.Draw(ui);
    }

    sf::Texture postprocessing = ENGINE.GetPostProcessingBuffer().getTexture(); // Make a copy
    sf::Sprite transitionPost;
    transitionPost.setTexture(postprocessing);

    transition.setUniform("progress", transitionProgress);

    LayeredDrawable* bake = new LayeredDrawable(transitionPost);
    bake->SetShader(&transition);

    ENGINE.Draw(bake);
    delete bake;

    // Write contents to screen (always last step)
    ENGINE.Display();

    elapsed = static_cast<float>(clock.getElapsedTime().asSeconds());
  }

  AUDIO.StopStream();
  ENGINE.RevokeShader();

  return 0; // signal game over to the main stack
}