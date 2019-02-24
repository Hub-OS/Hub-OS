#include <Swoosh\ActivityController.h>

#include "bnMainMenuScene.h"
#include "bnChipFolderCollection.h"

#include "Segues\PushIn.h"
#include "Segues\Checkerboard.h"
#include "Segues\WhiteWashFade.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

MainMenuScene::MainMenuScene(swoosh::ActivityController& controller) :
  camera(ENGINE.GetDefaultView()),
  swoosh::Activity(controller)
{
  data = ChipFolderCollection::ReadFromFile("resources/database/folders.txt");

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
  uiAnimator.Reload();

  // Keep track of selected navi
  currentNavi = 0;

  owNavi = sf::Sprite(LOAD_TEXTURE(NAVI_MEGAMAN_ATLAS));
  owNavi.setScale(2.f, 2.f);
  owNavi.setPosition(0, 0.f);
  naviAnimator = Animation("resources/navis/megaman/megaman.animation");
  naviAnimator.Reload();
  naviAnimator.SetAnimation("PLAYER_OW_RD");
  naviAnimator << Animate::Mode::Loop;

  map->AddSprite(&owNavi);

  overlay = sf::Sprite(LOAD_TEXTURE(MAIN_MENU));
  overlay.setScale(2.f, 2.f);

  ow = sf::Sprite(LOAD_TEXTURE(MAIN_MENU_OW));
  ow.setScale(2.f, 2.f);

  gotoNextScene = false;

}

void MainMenuScene::onStart() {
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/loop_overworld.ogg", true);
  ENGINE.SetCamera(camera);
}

void MainMenuScene::onUpdate(double elapsed) {
  // std::cout << "elapsed: " << elapsed << std::endl;
  map->Update((float)elapsed);

  camera.Update((float)elapsed);
  bg->Update((float)elapsed);

  // Draw navi moving
  naviAnimator.Update((float)elapsed, owNavi);

  int lastMenuSelectionIndex = menuSelectionIndex;

  // Move the navi down
  owNavi.setPosition(owNavi.getPosition() + sf::Vector2f(50.0f*(float)elapsed, 0));

  // TODO: fix this broken camera system
  sf::Vector2f camOffset = camera.GetView().getSize();
  camOffset.x /= 5;
  camOffset.y /= 3.5;

  // Follow the navi
  camera.PlaceCamera(map->ScreenToWorld(owNavi.getPosition() - sf::Vector2f(0.5, 0.5)) + camOffset);

  if (!gotoNextScene) {
    if (INPUT.has(PRESSED_A)) {

      // Folder Select
      if (menuSelectionIndex == 0) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::left>>;
        this->getController().push<segue::to<FolderScene>>(data);
      }

      // Library Select
      if (menuSelectionIndex == 1) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::right>>;
        this->getController().push<segue::to<LibraryScene>>();
      }

      // Navi select
      if (menuSelectionIndex == 2) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);
        using segue = swoosh::intent::segue<Checkerboard, swoosh::intent::milli<500>>;
        using intent = segue::to<SelectNaviScene>;
        this->getController().push<intent>(currentNavi);
      }

      // Mob select
      if (menuSelectionIndex == 3) {
        gotoNextScene = true;

        ChipFolder* folder = nullptr;

        if (data.GetFolder("Default", folder)) {
          AUDIO.Play(AudioType::CHIP_DESC);
          using segue = swoosh::intent::segue<WhiteWashFade>::to<SelectMobScene>;
          this->getController().push<segue>(currentNavi, *folder);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR); 
          Logger::Log("Cannot proceed to mob select. Error selecting folder 'Default'.");
        }
      }
    }

    if (INPUT.has(PRESSED_UP)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous selection 
        selectInputCooldown = maxSelectInputCooldown;
        menuSelectionIndex--;
      }
    }
    else if (INPUT.has(PRESSED_DOWN)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to next selection 
        selectInputCooldown = maxSelectInputCooldown;
        menuSelectionIndex++;
      }
    }
    else {
      selectInputCooldown = 0;
    }
  }

  /*if (INPUT.has(PRESSED_PAUSE)) {
    static bool toggle = false;
    toggle = !toggle;
    showHUD = false;
    map->ToggleLighting(toggle);
  }*/

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
  // If coming back from navi select, the navi has changed, update it
  owNavi.setTexture(NAVIS.At(currentNavi).GetOverworldTexture());
  naviAnimator = Animation(NAVIS.At(currentNavi).GetOverworldAnimationPath());
  naviAnimator.Reload();
  naviAnimator.SetAnimation("PLAYER_OW_RD");
  naviAnimator << Animate::Mode::Loop;
}

void MainMenuScene::onResume() {
  gotoNextScene = false;

  ENGINE.SetCamera(camera);
}

void MainMenuScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(map);

  ENGINE.Draw(overlay);

  if (showHUD) {
    uiAnimator.SetAnimation("CHIP_FOLDER");

    if (menuSelectionIndex == 0) {
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(50.f, 50.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(100.f, 50.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(20.f, 50.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(100.f, 50.f);
      ENGINE.Draw(ui);
    }

    uiAnimator.SetAnimation("LIBRARY");

    if (menuSelectionIndex == 1) {
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(50.f, 120.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("LIBRARY_LABEL");
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(100.f, 120.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(20.f, 120.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("LIBRARY_LABEL");
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(100.f, 120.f);
      ENGINE.Draw(ui);
    }

    uiAnimator.SetAnimation("NAVI");

    if (menuSelectionIndex == 2) {
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(50.f, 190.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("NAVI_LABEL");
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(100.f, 190.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(20.f, 190.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("NAVI_LABEL");
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(100.f, 190.f);
      ENGINE.Draw(ui);
    }

    uiAnimator.SetAnimation("MOB_SELECT");

    if (menuSelectionIndex == 3) {
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(50.f, 260.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("MOB_SELECT_LABEL");
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(100.f, 260.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(20.f, 260.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("MOB_SELECT_LABEL");
      uiAnimator.SetFrame(1, ui);
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