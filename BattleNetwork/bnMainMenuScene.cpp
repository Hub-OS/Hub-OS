#include <Swoosh/ActivityController.h>
#include "bnWebClientMananger.h"

#include "Android/bnTouchArea.h"

#include "bnMainMenuScene.h"
#include "bnCardFolderCollection.h"
#include "netplay/bnPVPScene.h"

#include "Segues/PushIn.h"
#include "Segues/Checkerboard.h"
#include "Segues/PixelateBlackWashFade.h"
#include "Segues/DiamondTileSwipe.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;
using namespace swoosh::types;


/// \brief Thunk to populate menu options to callbacks
auto MakeOptions = [] (MainMenuScene* scene) -> MenuWidget::OptionsList {
  return {
    { "chip_folder", std::bind(&MainMenuScene::GotoChipFolder, scene) },
    { "navi",        std::bind(&MainMenuScene::GotoNaviSelect, scene) },
    { "mob_select",  std::bind(&MainMenuScene::GotoMobSelect, scene) },
    { "config",      std::bind(&MainMenuScene::GotoConfig, scene) },
    { "sync",        std::bind(&MainMenuScene::GotoPVP, scene) }
  };
};

MainMenuScene::MainMenuScene(swoosh::ActivityController& controller, bool guestAccount) :
  guestAccount(guestAccount),
  camera(ENGINE.GetView()),
  lastIsConnectedState(false),
  showHUD(true),
  menuWidget("Overworld", MakeOptions(this)),
  textbox({ 4, 255 }),
  swoosh::Activity(&controller)
{
  // When we reach the menu scene we need to load the player information
  // before proceeding to next sub menus
  //data = CardFolderCollection::ReadFromFile("resources/database/folders.txt");

  webAccountIcon.setTexture(LOAD_TEXTURE(WEBACCOUNT_STATUS));
  webAccountIcon.setScale(2.f, 2.f);
  webAccountIcon.setPosition(4, getController().getVirtualWindowSize().y - 44.0f);
  webAccountAnimator = Animation("resources/ui/webaccount_icon.animation");
  webAccountAnimator.Load();
  webAccountAnimator.SetAnimation("NO_CONNECTION");

  // Draws the scrolling background
  bg = new LanBackground();

  // Show the HUD
  showHUD = true;

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // Keep track of selected navi
  currentNavi = 0;

  menuWidget.setScale(2.f, 2.f);

  gotoNextScene = true;

  /// WEB ACCOUNT LOADING

  WebAccounts::AccountState account;

  if (WEBCLIENT.IsLoggedIn()) {
    bool loaded = WEBCLIENT.LoadSession("profile.bin", &account);

    // Quickly load the session on disk to reduce wait times
    if (loaded) {
      Logger::Log("Found cached account data");

      WEBCLIENT.UseCachedAccount(account);
      WEBCLIENT.CacheTextureData(account);
      folders = CardFolderCollection::ReadFromWebAccount(account);
      programAdvance = PA::ReadFromWebAccount(account);

      NaviEquipSelectedFolder();
    }

    Logger::Log("Fetching account data...");

    // resent fetch command to get the a latest account info
    accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();

    Logger::Log("waiting for server...");
  }
  else {

    // If we are not actively online but we have a profile on disk, try to load our previous session
    // The user may be logging in but has not completed yet and we want to reduce wait times...
    // Otherwise, use the guest profile
    bool loaded = WEBCLIENT.LoadSession("profile.bin", &account) || WEBCLIENT.LoadSession("guest.bin", &account);

    if (loaded) {
      WEBCLIENT.UseCachedAccount(account);
      WEBCLIENT.CacheTextureData(account);
      folders = CardFolderCollection::ReadFromWebAccount(account);
      programAdvance = PA::ReadFromWebAccount(account);

      NaviEquipSelectedFolder();
    }
  }

  setView(sf::Vector2u(480, 320));

  // Spawn overworld player
  actor.LoadAnimations("resources/navis/megaman/overworld.animation");
  actor.setTexture(TEXTURES.LoadTextureFromFile("resources/navis/megaman/overworld.png"));
  actor.setPosition(200, 20);
  playerController.ControlActor(actor);

  // Share the camera
  map.SetCamera(&camera);
  map.AddSprite(&actor, 0);
  map.setScale(2.f, 2.f);

  // Load overworld assets
  this->treeTexture = TEXTURES.LoadTextureFromFile("resources/ow/tree.png");
  treeAnim = Animation("resources/ow/tree.animation") << "default" << Animator::Mode::Loop;

  this->warpTexture = TEXTURES.LoadTextureFromFile("resources/ow/warp.png");
  warpAnim = Animation("resources/ow/warp.animation") << "default" << Animator::Mode::Loop;

  this->gateTexture = TEXTURES.LoadTextureFromFile("resources/ow/gate.png");
  gateAnim = Animation("resources/ow/gate.animation") << "default" << Animator::Mode::Loop;

  this->coffeeTexture = TEXTURES.LoadTextureFromFile("resources/ow/coffee.png");
  coffeeAnim = Animation("resources/ow/coffee.animation") << "default" << Animator::Mode::Loop;

  // Effectively loads the map
  ResetMap();
}

void MainMenuScene::onStart() {
  // Stop any music already playing
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/loop_overworld.ogg", false);

#ifdef __ANDROID__
  StartupTouchControls();
#endif

  gotoNextScene = false;
}

void MainMenuScene::onUpdate(double elapsed) {
  if (menuWidget.IsClosed() && textbox.IsClosed()) {
    playerController.ListenToInputEvents(true);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Home)) {
      this->ResetMap();
    }
  }
  else {
    playerController.ListenToInputEvents(false);
  }

  playerController.Update(elapsed);

  /**
  * After updating the actor, we are moved to a new location
  * We do a post-check to see if we collided with anything marked solid
  * If so, we adjust the displacement or halt the actor
  **/
  auto lastPos = actor.getPosition();
  actor.Update(elapsed);
  auto newPos = actor.getPosition();

  if (map.GetTileAt(newPos).solid) {
    auto diff = newPos - lastPos;
    auto& [first, second] = Split(actor.GetHeading());

    actor.setPosition(lastPos);

    if (second != Direction::none) {
      actor.setPosition(lastPos);

      auto diffx = diff;
      diffx.y = 0;

      auto diffy = diff;
      diff.x = 0;

      if (map.GetTileAt(lastPos + diffx).solid == false) {
        actor.setPosition(lastPos + diffx);
      }
      
      if (map.GetTileAt(lastPos + diffy).solid == false) {
        actor.setPosition(lastPos + diffy);
      }
    }
  }

  // check to see if talk button was pressed
  if (textbox.IsClosed()) {
    if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
      // check to see what tile we pressed talk to
      const Overworld::Map::Tile tile = map.GetTileAt(actor.PositionInFrontOf());
      if (tile.token == "C") {
        textbox.EnqueMessage({}, "", new Message("A cafe sign.\nYou feel welcomed."));
        textbox.Open();
      }
      else if (tile.token == "G") {
        textbox.EnqueMessage({}, "", new Message("Do you know da way?"));
        textbox.Open();
      }
    }
  }
  else if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
    // continue the conversation if the text is complete
    if (textbox.IsEndOfMessage()) {
      textbox.DequeMessage();

      if (!textbox.HasMessage()) {
        // if there are no more messages, close
        textbox.Close();
      }
    }
    else {
      // double tapping talk will complete the block
      textbox.CompleteCurrentBlock();
    }
  }

  /**
  * update all overworld animations
  */
  animElapsed += elapsed;

  treeAnim.SyncTime(animElapsed);
  for (auto& t : trees) {
    treeAnim.Refresh(t->getSprite());
  };

  coffeeAnim.SyncTime(animElapsed);
  for (auto& c : coffees) {
    coffeeAnim.Refresh(c->getSprite());
  }

  gateAnim.SyncTime(animElapsed);
  for (auto& g : gates) {
    gateAnim.Refresh(g->getSprite());
  }

  warpAnim.SyncTime(animElapsed);
  for (auto& w : warps) {
    warpAnim.Refresh(w->getSprite());
  }

  #ifdef __ANDROID__
  if(gotoNextScene)
    return; // keep the screen looking the same when we come back
  #endif

  if (WEBCLIENT.IsLoggedIn() && accountCommandResponse.valid() && is_ready(accountCommandResponse)) {
    try {
      const WebAccounts::AccountState& account = accountCommandResponse.get();
      Logger::Logf("You have %i folders on your account", account.folders.size());
      WEBCLIENT.CacheTextureData(account);
      folders = CardFolderCollection::ReadFromWebAccount(account);
      programAdvance = PA::ReadFromWebAccount(account);

      NaviEquipSelectedFolder();

      // Replace
      WEBCLIENT.SaveSession("profile.bin");
    }
    catch (const std::runtime_error& e) {
      Logger::Logf("Could not fetch account.\nError: %s", e.what());
    }
  }

  // update the web connectivity icon
  bool currentConnectivity = WEBCLIENT.IsConnectedToWebServer();
  if (currentConnectivity != lastIsConnectedState) {
    if (WEBCLIENT.IsConnectedToWebServer()) {
       webAccountAnimator.SetAnimation("OK_CONNECTION");
    }
    else {
       webAccountAnimator.SetAnimation("NO_CONNECTION");
    }

    lastIsConnectedState = currentConnectivity;
  }

  // Update the map
  map.Update((float)elapsed);

  // Update the camera
  camera.Update((float)elapsed);
  
  // Loop the bg
  bg->Update((float)elapsed);

  // Update the widget
  menuWidget.Update((float)elapsed);

  // Update the textbox
  textbox.Update(elapsed);

  // Follow the navi
  sf::Vector2f pos = map.ScreenToWorld(actor.getPosition());
  pos = sf::Vector2f(pos.x*map.getScale().x, pos.y*map.getScale().y);
  camera.PlaceCamera(pos);

  if (!gotoNextScene && textbox.IsClosed()) {
    if (INPUTx.Has(EventTypes::PRESSED_PAUSE) && !INPUTx.Has(EventTypes::PRESSED_CANCEL)) {
      if (menuWidget.IsClosed()) {
        menuWidget.Open();
        AUDIO.Play(AudioType::CHIP_DESC);
      }
      else if (menuWidget.IsOpen()) {
        menuWidget.Close();
        AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      }
    }

    if (menuWidget.IsOpen()) {
      if (INPUTx.Has(EventTypes::PRESSED_UI_UP) || INPUTx.Has(EventTypes::HELD_UI_UP)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          if(!extendedHold) {
            selectInputCooldown = maxSelectInputCooldown;
            extendedHold = true;
          }
          else {
            selectInputCooldown = maxSelectInputCooldown / 4.0;
          }

          menuWidget.CursorMoveUp() ? AUDIO.Play(AudioType::CHIP_SELECT) : 0;
        }
      }
      else if (INPUTx.Has(EventTypes::PRESSED_UI_DOWN) || INPUTx.Has(EventTypes::HELD_UI_DOWN)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          if (!extendedHold) {
            selectInputCooldown = maxSelectInputCooldown;
            extendedHold = true;
          }
          else {
            selectInputCooldown = maxSelectInputCooldown / 4.0;
          }

          menuWidget.CursorMoveDown() ? AUDIO.Play(AudioType::CHIP_SELECT) : 0;
        }
      }
      else if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
        bool result = menuWidget.ExecuteSelection();

        if (result && menuWidget.IsOpen() == false) {
          AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
        }
      }
      else if (INPUTx.Has(EventTypes::PRESSED_UI_RIGHT) || INPUTx.Has(EventTypes::PRESSED_CANCEL)) {
        extendedHold = false;
        menuWidget.SelectExit() ? AUDIO.Play(AudioType::CHIP_SELECT) : 0;
      }
      else if (INPUTx.Has(EventTypes::PRESSED_UI_LEFT)) {
        menuWidget.SelectOptions() ? AUDIO.Play(AudioType::CHIP_SELECT) : 0;
        extendedHold = false;
      }
      else {
        extendedHold = false;
        selectInputCooldown = 0;
      }
    }
  }

  // Allow player to resync with remote account by pressing the pause action
  if (INPUTx.Has(EventTypes::PRESSED_QUICK_OPT)) {
      accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();
  }

  webAccountAnimator.Update((float)elapsed, webAccountIcon.getSprite());
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
  RefreshNaviSprite();

  // Set the camera back to ours
  ENGINE.SetCamera(camera);
}

void MainMenuScene::onResume() {

  guestAccount = !WEBCLIENT.IsLoggedIn();

  if (!guestAccount) {
    accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();
  }
  else {
    try {
      WEBCLIENT.SaveSession("guest.bin");
    }
    catch (const std::runtime_error& e) {
      Logger::Logf("Could not fetch account.\nError: %s", e.what());
    }
  }

  gotoNextScene = false;

  ENGINE.SetCamera(camera);

#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void MainMenuScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);

  auto offset = ENGINE.GetViewOffset();
  map.move(offset);
  ENGINE.Draw(map);
  map.move(-offset);

  if (showHUD) {
    ENGINE.Draw(menuWidget);
  }

  // Add the web account connection symbol
  ENGINE.Draw(&webAccountIcon);

  ENGINE.Draw(textbox);
}

void MainMenuScene::onEnd() {
  AUDIO.StopStream();
  ENGINE.RevokeShader();

#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}

void MainMenuScene::RefreshNaviSprite()
{
  auto& meta = NAVIS.At(currentNavi);

  // refresh menu widget too
  int hp = std::atoi(meta.GetHPString().c_str());
  menuWidget.SetHealth(hp);
  menuWidget.SetMaxHealth(hp);

  // If coming back from navi select, the navi has changed, update it
  auto owPath = meta.GetOverworldAnimationPath();

  if (owPath.size()) {
    // TODO: replace with actor
    /*owNavi.setTexture(meta.GetOverworldTexture());
    naviAnimator = Animation(meta.GetOverworldAnimationPath());
    naviAnimator.Reload();
    naviAnimator.SetAnimation("PLAYER_OW_RD");
    naviAnimator << Animator::Mode::Loop;*/
  }
  else {
    Logger::Logf("Overworld animation not found for navi at index %i", currentNavi);
  }
}

void MainMenuScene::NaviEquipSelectedFolder()
{
  auto naviStr = WEBCLIENT.GetValue("SelectedNavi");
  if (!naviStr.empty()) {
    currentNavi = SelectedNavi(atoi(naviStr.c_str()));
    RefreshNaviSprite();

    auto folderStr = WEBCLIENT.GetValue("FolderFor:" + naviStr);
    if (!folderStr.empty()) {
      // preserve our selected folder
      if (int index = folders.FindFolder(folderStr); index >= 0) {
        folders.SwapOrder(index, 0); // Select this folder again
      }
    }
  }
  else {
    currentNavi = SelectedNavi(0);
    WEBCLIENT.SetKey("SelectedNavi", std::to_string(0));
  }
}

void MainMenuScene::ResetMap()
{
  // Load a map
  unsigned lastMapRows = map.GetRows();
  unsigned lastMapCols = map.GetCols();

  auto result = Overworld::Map::LoadFromFile(map, "resources/ow/homepage.txt");

  if (!result.first) {
    Logger::Log("Failed to load map homepage.txt");
  }
  else {
    // clear the last tile pointers
    for (unsigned i = 0; i < lastMapRows; i++) {
      delete[] tiles[i];
    }

    if (lastMapRows > 0 && lastMapCols > 0) {
      delete[] tiles;
    }
    
    // clean up sprites too
    std::vector<std::shared_ptr<SpriteProxyNode>> nodes;
    nodes.insert(nodes.end(), trees.begin(), trees.end());
    nodes.insert(nodes.end(), warps.begin(), warps.end());
    nodes.insert(nodes.end(), gates.begin(), gates.end());
    nodes.insert(nodes.end(), coffees.begin(), coffees.end());

    for (auto& node : nodes) {
      map.RemoveSprite(&*node);
    }

    trees.clear();
    warps.clear();
    gates.clear();
    coffees.clear();

    // Place some objects in the scene
    auto places = map.FindToken("S");;
    if (places.size()) {
      actor.setPosition(places[0]);
    }

    auto trees = map.FindToken("T");
    auto warps = map.FindToken("W");
    auto coff = map.FindToken("C");
    auto gates = map.FindToken("G");

    for (auto pos : trees) {
      auto new_tree = std::make_shared<SpriteProxyNode>(*treeTexture);
      new_tree->setPosition(pos);
      this->trees.push_back(new_tree);
      map.AddSprite(&*new_tree, 0);

      // do not walk through trees
      auto tile = map.GetTileAt(pos);
      tile.solid = true;
      map.SetTileAt(pos, tile);
    }

    for (auto pos : coff) {
      auto new_coffee = std::make_shared<SpriteProxyNode>(*coffeeTexture);
      new_coffee->setPosition(pos);
      this->coffees.push_back(new_coffee);
      map.AddSprite(&*new_coffee, 0);

      // do not walk through coffee displays
      auto tile = map.GetTileAt(pos);
      tile.solid = true;
      map.SetTileAt(pos, tile);
    }

    for (auto pos : gates) {
      auto new_gate = std::make_shared<SpriteProxyNode>(*gateTexture);
      new_gate->setPosition(pos);
      this->gates.push_back(new_gate);
      map.AddSprite(&*new_gate, 0);

      // do not walk through gates
      auto tile = map.GetTileAt(pos);
      tile.solid = true;
      tile.ID = 3; // make red
      map.SetTileAt(pos, tile);
    }

    for (auto pos : warps) {
      auto new_warp = std::make_shared<SpriteProxyNode>(*warpTexture);
      new_warp->setPosition(pos);
      this->warps.push_back(new_warp);
      map.AddSprite(&*new_warp, -1);
    }

    menuWidget.SetArea(map.GetName());
    this->tiles = result.second;
  }
}

void MainMenuScene::GotoChipFolder()
{
  gotoNextScene = true;
  AUDIO.Play(AudioType::CHIP_DESC);

  using effect = segue<PushIn<direction::left>, milliseconds<500>>;
  getController().push<effect::to<FolderScene>>(folders);
}

void MainMenuScene::GotoNaviSelect()
{
  // Navi select
  gotoNextScene = true;
  AUDIO.Play(AudioType::CHIP_DESC);

  using effect = segue<Checkerboard, milliseconds<250>>;
  getController().push<effect::to<SelectNaviScene>>(currentNavi);
}

void MainMenuScene::GotoConfig()
{
  // Config Select on PC
  gotoNextScene = true;
  AUDIO.Play(AudioType::CHIP_DESC);

  using effect = segue<DiamondTileSwipe<direction::right>, milliseconds<500>>;
  getController().push<effect::to<ConfigScene>>();
}

void MainMenuScene::GotoMobSelect()
{
  gotoNextScene = true;

  CardFolder* folder = nullptr;

  if (folders.GetFolder(0, folder)) {
    using effect = segue<PixelateBlackWashFade, milliseconds<500>>;
    AUDIO.Play(AudioType::CHIP_DESC);
    getController().push<effect::to<SelectMobScene>>(currentNavi, *folder, programAdvance);
  }
  else {
    AUDIO.Play(AudioType::CHIP_ERROR);
    Logger::Log("Cannot proceed to battles. You need 1 folder minimum.");
    gotoNextScene = false;
  }
}

void MainMenuScene::GotoPVP()
{
  gotoNextScene = true;

  CardFolder* folder = nullptr;

  if (folders.GetFolder(0, folder)) {
    AUDIO.Play(AudioType::CHIP_DESC);
    using effect = segue<PushIn<direction::down>, milliseconds<500>>;
    getController().push<effect::to<PVPScene>>(static_cast<int>(currentNavi), *folder, programAdvance);
  }
  else {
    AUDIO.Play(AudioType::CHIP_ERROR);
    Logger::Log("Cannot proceed to battles. You need 1 folder minimum.");
    gotoNextScene = false;
  }
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

        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::left>, swoosh::intent::milli<500>>;
        getController().push<segue::to<FolderScene>>(data);
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

        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::right>>;
        getController().push<segue::to<LibraryScene>, swoosh::intent::milli<500>>();
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
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);
        using segue = swoosh::intent::segue<Checkerboard, swoosh::intent::milli<500>>;
        using intent = segue::to<SelectNaviScene>;
        getController().push<intent>(currentNavi);
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
        gotoNextScene = true;

        CardFolder* folder = nullptr;

        if (data.GetFolder("Default", folder)) {
          AUDIO.Play(AudioType::CHIP_DESC);
          using segue = swoosh::intent::segue<PixelateBlackWashFade, swoosh::intent::milli<500>>::to<SelectMobScene>;
          getController().push<segue>(currentNavi, *folder);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR);
          Logger::Log("Cannot proceed to mob select. Error selecting folder 'Default'.");
          gotoNextScene = false;
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