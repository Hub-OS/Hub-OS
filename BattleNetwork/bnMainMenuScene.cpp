#include "bnMainMenuScene.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

MainMenuScene::MainMenuScene(swoosh::ActivityController& controller) :
  camera(ENGINE.GetDefaultView()),
  swoosh::Activity(controller)
{
  // Transition
  transition = &LOAD_SHADER(TRANSITION);
  transition->setUniform("texture", sf::Shader::CurrentTexture);
  transition->setUniform("map", LOAD_TEXTURE(NOISE_TEXTURE));
  transition->setUniform("progress", 0.f);

  bg = new LanBackground();

  map = new Overworld::InfiniteMap(10, 20, 47, 24);
  map->SetCamera(&camera);

  showHUD = true;

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // ui sprite maps
  ui = sf::Sprite(LOAD_TEXTURE(MAIN_MENU_UI));
  ui.setScale(2.f, 2.f);
  uiAnimator = Animation("resources/ui/main_menu_ui.animation");
  uiAnimator.Load();

  // Keep track of selected navi
  currentNavi = 0;

  owNavi = sf::Sprite(LOAD_TEXTURE(NAVI_MEGAMAN_ATLAS));
  owNavi.setScale(2.f, 2.f);
  owNavi.setPosition(0, 0.f);
  naviAnimator = Animation("resources/navis/megaman/megaman.animation");
  naviAnimator.Load();
  naviAnimator.SetAnimation("PLAYER_OW_RD");
  naviAnimator << Animate::Mode(Animate::Mode::Loop);

  map->AddSprite(&owNavi);

  overlay = sf::Sprite(LOAD_TEXTURE(MAIN_MENU));
  overlay.setScale(2.f, 2.f);

  ow = sf::Sprite(LOAD_TEXTURE(MAIN_MENU_OW));
  ow.setScale(2.f, 2.f);
}

void MainMenuScene::onStart() {
  gotoNextScene = false;

  // Stream menu music 
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/loop_navi_customizer.ogg", true);
  ENGINE.SetCamera(camera);
}

void MainMenuScene::onUpdate(double elapsed) {
  INPUT.update();
  map->Update(elapsed);

  camera.Update(elapsed);
  bg->Update(elapsed);

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

  if (!gotoNextScene) {
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

    if (INPUT.has(PRESSED_A)) {

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
      if (menuSelectionIndex == 3) {
        gotoNextScene = true;
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


  if (menuSelectionIndex == 1) {
    /*int result = FolderScene::Run();

    // reset internal clock (or everything will teleport)
    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
    std::cout << "time slept: " << elapsed << "\n";
    clock.restart();
    elapsed = 0;

    if (result == 0) {
      // break; // Breaks the while-loop
      ActivityManager::Segue<CheckerSegue>::To<FolderScene>();
    }*/
  }
  else if (menuSelectionIndex == 2) {
    /*currentNavi = SelectNaviScene::Run(currentNavi);

    owNavi.setTexture(NAVIS.At(currentNavi).GetOverworldTexture());
    naviAnimator = Animation(NAVIS.At(currentNavi).GetOverworldAnimationPath());
    naviAnimator.Load();
    naviAnimator.SetAnimation("PLAYER_OW_RD");
    naviAnimator << Animate::Mode(Animate::Mode::Loop);

    // reset internal clock (or everything will teleport)
    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
    std::cout << "time slept: " << elapsed << "\n";
    clock.restart();
    elapsed = 0;*/

  }
  else if (menuSelectionIndex == 3) {
    /*int result = SelectMobScene::Run(currentNavi);

    // reset internal clock (or everything will teleport)
    elapsed = static_cast<float>(clock.getElapsedTime().asMilliseconds());
    std::cout << "time slept: " << elapsed << "\n";
    clock.restart();
    elapsed = 0;

    if (result == 0) {
      // ActivityManager::Pop();
    }*/
  }

  gotoNextScene = false;

  menuSelectionIndex = std::max(0, menuSelectionIndex);
  menuSelectionIndex = std::min(3, menuSelectionIndex);

  if (menuSelectionIndex != lastMenuSelectionIndex) {
    AUDIO.Play(AudioType::CHIP_SELECT);
  }
}

void MainMenuScene::onLeave() {

}

void MainMenuScene::onExit()
{
}

void MainMenuScene::onEnter()
{
}

void MainMenuScene::onResume() {

}

void MainMenuScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(map);

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
}

void MainMenuScene::onEnd() {
  AUDIO.StopStream();
  ENGINE.RevokeShader();
}