#include <Swoosh/ActivityController.h>
#include "Android/bnTouchArea.h"

#include "bnMainMenuScene.h"
#include "Segues/DiamondTileSwipe.h"
#include "bnChipFolderCollection.h"

#include "Segues/PushIn.h"
#include "Segues/Checkerboard.h"
#include "Segues/PixelateBlackWashFade.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

MainMenuScene::MainMenuScene(swoosh::ActivityController& controller) :
  camera(ENGINE.GetView()),
  swoosh::Activity(&controller)
{
  // When we reach the menu scene we need to load the player information
  // before proceeding to next sub menus
  data = ChipFolderCollection::ReadFromFile("resources/database/folders.txt");

  // Draws the scrolling background
  bg = new LanBackground();

  // Generate an infinite map with a branch depth of 3, column size of 10
  // and tile dimensions 47x24
  map = new Overworld::InfiniteMap(3, 10, 47, 24);
  
  // Share the camera
  map->SetCamera(&camera);

  // Show the HUD
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
  naviAnimator << Animator::Mode::Loop;

  // Share the navi sprite
  // Map will transform navi's ortho position into isometric position
  map->AddSprite(&owNavi);

  overlay = sf::Sprite(LOAD_TEXTURE(MAIN_MENU));
  overlay.setScale(2.f, 2.f);

  ow = sf::Sprite(LOAD_TEXTURE(MAIN_MENU_OW));
  ow.setScale(2.f, 2.f);

  gotoNextScene = true;

  menuSelectionIndex = lastMenuSelectionIndex = 0;
}

void MainMenuScene::onStart() {
  // Stop any music already playing
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/loop_overworld.ogg", false);
  
  // Set the camera back to ours
  ENGINE.SetCamera(camera);

#ifdef __ANDROID__
  StartupTouchControls();
#endif

  gotoNextScene = false;
}

void MainMenuScene::onUpdate(double elapsed) {
#ifdef __ANDROID__
  if(gotoNextScene)
    return; // keep the screen looking the same when we come back
#endif

  // Update the map
  map->Update((float)elapsed);

  // Update the camera
  camera.Update((float)elapsed);
  
  // Loop the bg
  bg->Update((float)elapsed);

  // Draw navi moving
  naviAnimator.Update((float)elapsed, owNavi);

  // Move the navi down
  owNavi.setPosition(owNavi.getPosition() + sf::Vector2f(50.0f*(float)elapsed, 0));

  // TODO: fix this broken camera system
  sf::Vector2f camOffset = camera.GetView().getSize();
  camOffset.x /= 5;
  camOffset.y /= 3.5;

  // Follow the navi
  camera.PlaceCamera(map->ScreenToWorld(owNavi.getPosition() - sf::Vector2f(0.5, 0.5)) + camOffset);

  if (!gotoNextScene) {
    if (INPUT.Has(EventTypes::PRESSED_CONFIRM) && !INPUT.Has(EventTypes::PRESSED_CANCEL)) {

      // Folder Select
      if (menuSelectionIndex == 0) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::left>, swoosh::intent::milli<500>>;
        this->getController().push<segue::to<FolderScene>>(data);
      }

      // Config Select on PC 
      if (menuSelectionIndex == 1) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<DiamondTileSwipe<direction::right>, swoosh::intent::milli<500>>;
        this->getController().push<segue::to<ConfigScene>>();
      }

      // Navi select
      if (menuSelectionIndex == 2) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);
        using segue = swoosh::intent::segue<Checkerboard, swoosh::intent::milli<250>>;
        using intent = segue::to<SelectNaviScene>;
        this->getController().push<intent>(currentNavi);
      }

      // Mob select
      if (menuSelectionIndex == 3) {
        gotoNextScene = true;

        ChipFolder* folder = nullptr;

        if (data.GetFolder(0, folder)) {
          AUDIO.Play(AudioType::CHIP_DESC);
          using segue = swoosh::intent::segue<PixelateBlackWashFade, swoosh::intent::milli<500>>::to<SelectMobScene>;
          this->getController().push<segue>(currentNavi, *folder);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR); 
          Logger::Log("Cannot proceed to mob select. Error selecting folder 'Default'.");
          gotoNextScene = false;
        }
      }
    }

    if (INPUT.Has(EventTypes::PRESSED_UI_UP)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous selection 
        selectInputCooldown = maxSelectInputCooldown;
        menuSelectionIndex--;
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_DOWN)) {
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

  /*if (INPUT.Has(EventTypes::PRESSED_PAUSE)) {
    static bool toggle = false;
    toggle = !toggle;
    showHUD = false;
    map->ToggleLighting(toggle);
    map->AddLight(new Overworld::Light(owNavi.getPosition(), sf::Color(135+(120%rand()), 135+(120%rand()), 135+(120%rand()), 255), 10));
  }*/

  // Keep menu selection in range
  menuSelectionIndex = std::max(0, menuSelectionIndex);
  menuSelectionIndex = std::min(3, menuSelectionIndex);

  if (menuSelectionIndex != lastMenuSelectionIndex) {
    AUDIO.Play(AudioType::CHIP_SELECT);
  }

  lastMenuSelectionIndex = menuSelectionIndex;
}

void MainMenuScene::onLeave() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
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
  naviAnimator << Animator::Mode::Loop;
}

void MainMenuScene::onResume() {
  gotoNextScene = false;

  ENGINE.SetCamera(camera);

  std::string folderPath("resources/database/folders.txt");
  std::string libraryPath("resources/database/library.txt");

  if (data.GetFolderNames().size() > 0) {
    data.WriteToFile(folderPath);
  }

  CHIPLIB.SaveLibrary(libraryPath);

#ifdef __ANDROID__
  StartupTouchControls();
#endif
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

    uiAnimator.SetAnimation("CONFIG");

    if (menuSelectionIndex == 1) {
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(50.f, 120.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CONFIG_LABEL");
      uiAnimator.SetFrame(2, ui);
      ui.setPosition(100.f, 120.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui);
      ui.setPosition(20.f, 120.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CONFIG_LABEL");
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

#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}

#ifdef __ANDROID__
void MainMenuScene::StartupTouchControls() {
    ui.setScale(2.f,2.f);

    uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 50.f);

    auto bounds = ui.getGlobalBounds();
    auto rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));
    auto& folderBtn = TouchArea::create(rect);

    folderBtn.onRelease([this](sf::Vector2i delta) {
        Logger::Log("folder released");

        this->gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::left>, swoosh::intent::milli<500>>;
        this->getController().push<segue::to<FolderScene>>(this->data);
    });

    folderBtn.onTouch([this]() {
        menuSelectionIndex = 0;
    });

    uiAnimator.SetAnimation("LIBRARY_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 120.f);

    bounds = ui.getGlobalBounds();
    rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));

    Logger::Log(std::string("rect: ") + std::to_string(rect.left) + ", " + std::to_string(rect.top) + ", " + std::to_string(rect.width) + ", " + std::to_string(rect.height));

    auto& libraryBtn = TouchArea::create(rect);

    libraryBtn.onRelease([this](sf::Vector2i delta) {
        Logger::Log("library released");

        this->gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::right>>;
        this->getController().push<segue::to<LibraryScene>, swoosh::intent::milli<500>>();
    });

    libraryBtn.onTouch([this]() {
        menuSelectionIndex = 1;
    });


    uiAnimator.SetAnimation("NAVI_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 190.f);

    bounds = ui.getGlobalBounds();
    rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));
    auto& naviBtn = TouchArea::create(rect);

    naviBtn.onRelease([this](sf::Vector2i delta) {
        this->gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);
        using segue = swoosh::intent::segue<Checkerboard, swoosh::intent::milli<500>>;
        using intent = segue::to<SelectNaviScene>;
        this->getController().push<intent>(this->currentNavi);
    });

    naviBtn.onTouch([this]() {
        menuSelectionIndex = 2;
    });

    uiAnimator.SetAnimation("MOB_SELECT_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 260.f);

    bounds = ui.getGlobalBounds();
    rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));
    auto& mobBtn = TouchArea::create(rect);

    mobBtn.onRelease([this](sf::Vector2i delta) {
        this->gotoNextScene = true;

        ChipFolder* folder = nullptr;

        if (this->data.GetFolder("Default", folder)) {
          AUDIO.Play(AudioType::CHIP_DESC);
          using segue = swoosh::intent::segue<PixelateBlackWashFade, swoosh::intent::milli<500>>::to<SelectMobScene>;
          this->getController().push<segue>(this->currentNavi, *folder);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR);
          Logger::Log("Cannot proceed to mob select. Error selecting folder 'Default'.");
          this->gotoNextScene = false;
        }
    });

    mobBtn.onTouch([this]() {
        menuSelectionIndex = 3;
    });
}

void MainMenuScene::ShutdownTouchControls() {
  TouchArea::free();
}
#endif