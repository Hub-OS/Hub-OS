#include <Swoosh/ActivityController.h>
#include <Segues/PushIn.h>
#include <Segues/Checkerboard.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/DiamondTileSwipe.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "bnOverworldSceneBase.h"

#include "bnXML.h"

#include "../bnWebClientMananger.h"
#include "../Android/bnTouchArea.h"

#include "../bnFolderScene.h"
#include "../bnSelectNaviScene.h"
#include "../bnSelectMobScene.h"
#include "../bnLibraryScene.h"
#include "../bnConfigScene.h"
#include "../bnFolderScene.h"
#include "../bnCardFolderCollection.h"
#include "../bnLanBackground.h"
#include "../bnACDCBackground.h"
#include "../bnGraveyardBackground.h"
#include "../bnVirusBackground.h"
#include "../bnMedicalBackground.h"
#include "../bnJudgeTreeBackground.h"
#include "../bnMiscBackground.h"
#include "../bnRobotBackground.h"
#include "../bnSecretBackground.h"
#include "../bnUndernetBackground.h"
#include "../bnWeatherBackground.h"

#include "../netplay/bnPVPScene.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;

using namespace swoosh::types;

/// \brief Thunk to populate menu options to callbacks
auto MakeOptions = [](Overworld::SceneBase* scene) -> MenuWidget::OptionsList {
  return {
    { "chip_folder", std::bind(&Overworld::SceneBase::GotoChipFolder, scene) },
    { "navi",        std::bind(&Overworld::SceneBase::GotoNaviSelect, scene) },
    { "mob_select",  std::bind(&Overworld::SceneBase::GotoMobSelect, scene) },
    { "config",      std::bind(&Overworld::SceneBase::GotoConfig, scene) },
    { "sync",        std::bind(&Overworld::SceneBase::GotoPVP, scene) }
  };
};

Overworld::SceneBase::SceneBase(swoosh::ActivityController& controller, bool guestAccount) :
  guestAccount(guestAccount),
  lastIsConnectedState(false),
  menuWidget("Overworld", MakeOptions(this)),
  textbox({ 4, 255 }),
  camera(controller.getWindow().getView()),
  Scene(controller),
  map(0, 0, 0, 0),
  playerActor(std::make_shared<Overworld::Actor>("You"))
{
  // When we reach the menu scene we need to load the player information
  // before proceeding to next sub menus
  // data = CardFolderCollection::ReadFromFile("resources/database/folders.txt");

  webAccountIcon.setTexture(LOAD_TEXTURE(WEBACCOUNT_STATUS));
  webAccountIcon.setScale(2.f, 2.f);
  webAccountIcon.setPosition(4, getController().getVirtualWindowSize().y - 44.0f);
  webAccountAnimator = Animation("resources/ui/webaccount_icon.animation");
  webAccountAnimator.Load();
  webAccountAnimator.SetAnimation("NO_CONNECTION");

  // Draws the scrolling background
  SetBackground(new LanBackground);

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  menuWidget.setScale(2.f, 2.f);
  emote.setScale(2.f, 2.f);

  auto windowSize = getController().getVirtualWindowSize();
  emote.setPosition(windowSize.x / 2.f, windowSize.y / 2.f);

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
  playerActor->setPosition(200, 20);
  playerActor->SetCollisionRadius(6);
  playerActor->CollideWithMap(map);
  playerActor->CollideWithQuadTree(quadTree);
  playerActor->AddNode(&emoteNode);

  emoteNode.SetLayer(-100);

  AddSprite(playerActor, 0);
  AddSprite(teleportController.GetBeam(), 0);

  map.setScale(2.f, 2.f);

  // reserve 1000 elements so no references are invalidated
  npcs.reserve(1000);
  pathControllers.reserve(1000);

  // emotes
  emote.OnSelect(std::bind(&Overworld::SceneBase::OnEmoteSelected, this, std::placeholders::_1));
}

void Overworld::SceneBase::onStart() {
#ifdef __ANDROID__
  StartupTouchControls();
#endif

  gotoNextScene = false;
}

void Overworld::SceneBase::onUpdate(double elapsed) {
  if (menuWidget.IsClosed() && textbox.IsClosed()) {
    playerController.ListenToInputEvents(true);
  }
  else {
    playerController.ListenToInputEvents(false);
  }

  // check to see if talk button was pressed
  if (textbox.IsClosed() && menuWidget.IsClosed()) {
    if (Input().Has(InputEvents::pressed_interact)) {
      // check to see what tile we pressed talk to
      auto layerIndex = playerActor->GetLayer();
      auto layer = map.GetLayer(layerIndex);
      auto tileSize = map.GetTileSize();

      auto frontPosition = playerActor->PositionInFrontOf();

      auto tile = layer.GetTile(frontPosition.x / tileSize.x, frontPosition.y / tileSize.y);

      // todo: this needs to be adjusted lol
      // if (tile.token == "C") {
      //   textbox.EnqueMessage({}, "", new Message("A cafe sign.\nYou feel welcomed."));
      //   textbox.Open();
      // }
      // else if (tile.token == "G") {
      //   textbox.EnqueMessage({}, "", new Message("The gate needs a key to get through."));
      //   textbox.Open();
      // }
      // else if (tile.token == "X") {
      //   textbox.EnqueMessage({}, "", new Message("An xmas tree program developed by Mars"));
      //   textbox.EnqueMessage({}, "", new Message("It really puts you in the holiday spirit!"));
      //   textbox.Open();
      // }
    }
  }
  else if (Input().Has(InputEvents::pressed_interact)) {
    // continue the conversation if the text is complete
    if (textbox.IsEndOfMessage()) {
      textbox.DequeMessage();

      if (!textbox.HasMessage()) {
        // if there are no more messages, close
        textbox.Close();
      }
    }
    else if (textbox.IsEndOfBlock()) {
      textbox.ShowNextLines();
    }
    else {
      // double tapping talk will complete the block
      textbox.CompleteCurrentBlock();
    }
  }

  // handle custom tile collision
  if (!HasTeleportedAway() && teleportController.IsComplete()) {
    this->OnTileCollision();
  }

  /**
  * update all overworld objects and animations
  */

  // update tile animations
  map.Update(*this, elapsed);

  // objects
  for (auto& pathController : pathControllers) {
    pathController.Update(elapsed);
  }

  if (gotoNextScene == false) {
    playerController.Update(elapsed);
    teleportController.Update(elapsed);
  }

  playerActor->Update(elapsed);

  for (auto& npc : npcs) {
    npc.Update(elapsed);
  }

  // animations
  animElapsed += elapsed;

  static float ribbon_factor = 0;
  static bool ribbon_bounce = false;

  if (ribbon_factor >= 1.25f) {
    ribbon_bounce = true;
  }
  else if (ribbon_factor <= -0.25f) {
    ribbon_bounce = false;
  }

  if (ribbon_bounce) {
    ribbon_factor -= static_cast<float>(elapsed * 1.6);
  }
  else {
    ribbon_factor += static_cast<float>(elapsed * 1.6);
  }

  float input_factor = std::min(ribbon_factor, 1.0f);
  input_factor = std::max(input_factor, 0.0f);

#ifdef __ANDROID__
  if (gotoNextScene)
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

  // sort sprites
  std::sort(sprites.begin(), sprites.end(),
    [](WorldSprite A, WorldSprite B)
  {
    int A_layer_inv = -A.layer;
    int B_layer_inv = -B.layer;

    auto A_pos = A.node->getPosition();
    auto B_pos = B.node->getPosition();
    auto A_compare = A_pos.x + A_pos.y;
    auto B_compare = B_pos.x + B_pos.y;

    return std::tie(A_layer_inv, A_compare) < std::tie(B_layer_inv, B_compare);
  }
  );

  // Update the camera
  camera.Update((float)elapsed);

  // Loop the bg
  bg->Update((float)elapsed);

  // Update the widget
  menuWidget.Update((float)elapsed);

  // Update the emote widget
  emote.Update(elapsed);

  // Update the active emote
  emoteNode.Update(elapsed);

  // Update the textbox
  textbox.Update(elapsed);

  // Follow the navi
  sf::Vector2f pos = map.WorldToScreen(playerActor->getPosition());
  pos = sf::Vector2f(pos.x * map.getScale().x, pos.y * map.getScale().y);
  camera.PlaceCamera(pos);

  if (!gotoNextScene && textbox.IsClosed()) {
    // emotes widget
    if (Input().Has(InputEvents::pressed_option) && menuWidget.IsClosed()) {
      if (emote.IsClosed()) {
        emote.Open();
      }
      else if (emote.IsOpen()) {
        emote.Close();
      }
    }

    if (emote.IsClosed()) {
      // menu widget
      if (Input().Has(InputEvents::pressed_pause) && !Input().Has(InputEvents::pressed_cancel)) {
        if (menuWidget.IsClosed()) {
          menuWidget.Open();
          Audio().Play(AudioType::CHIP_DESC);
        }
        else if (menuWidget.IsOpen()) {
          menuWidget.Close();
          Audio().Play(AudioType::CHIP_DESC_CLOSE);
        }
      }

      // menu widget controlls
      if (menuWidget.IsOpen()) {
        if (Input().Has(InputEvents::pressed_ui_up) || Input().Has(InputEvents::held_ui_up)) {
          selectInputCooldown -= elapsed;

          if (selectInputCooldown <= 0) {
            if (!extendedHold) {
              selectInputCooldown = maxSelectInputCooldown;
              extendedHold = true;
            }
            else {
              selectInputCooldown = maxSelectInputCooldown / 4.0;
            }

            menuWidget.CursorMoveUp() ? Audio().Play(AudioType::CHIP_SELECT) : 0;
          }
        }
        else if (Input().Has(InputEvents::pressed_ui_down) || Input().Has(InputEvents::held_ui_down)) {
          selectInputCooldown -= elapsed;

          if (selectInputCooldown <= 0) {
            if (!extendedHold) {
              selectInputCooldown = maxSelectInputCooldown;
              extendedHold = true;
            }
            else {
              selectInputCooldown = maxSelectInputCooldown / 4.0;
            }

            menuWidget.CursorMoveDown() ? Audio().Play(AudioType::CHIP_SELECT) : 0;
          }
        }
        else if (Input().Has(InputEvents::pressed_confirm)) {
          bool result = menuWidget.ExecuteSelection();

          if (result && menuWidget.IsOpen() == false) {
            Audio().Play(AudioType::CHIP_DESC_CLOSE);
          }
        }
        else if (Input().Has(InputEvents::pressed_ui_right) || Input().Has(InputEvents::pressed_cancel)) {
          extendedHold = false;

          bool exitSelected = !menuWidget.SelectExit();

          if (exitSelected) {
            if (Input().Has(InputEvents::pressed_cancel)) {
              bool result = menuWidget.ExecuteSelection();

              if (result && menuWidget.IsOpen() == false) {
                Audio().Play(AudioType::CHIP_DESC_CLOSE);
              }
            }
            else {
              // already selected, switch to options
              menuWidget.SelectOptions();
            }
          }

          Audio().Play(AudioType::CHIP_SELECT);
        }
        else if (Input().Has(InputEvents::pressed_ui_left)) {
          menuWidget.SelectOptions() ? Audio().Play(AudioType::CHIP_SELECT) : 0;
          extendedHold = false;
        }
        else {
          extendedHold = false;
          selectInputCooldown = 0;
        }
      }
    }
  }

  // Allow player to resync with remote account by pressing the pause action
  /*if (Input().Has(InputEvents::pressed_option)) {
      accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();
  }*/

  webAccountAnimator.Update((float)elapsed, webAccountIcon.getSprite());
}

void Overworld::SceneBase::onLeave() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}

void Overworld::SceneBase::onExit()
{
  emoteNode.Reset();
}

void Overworld::SceneBase::onEnter()
{
  RefreshNaviSprite();
}

void Overworld::SceneBase::onResume() {

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

  // if we left this scene for a new OW scene... return to our warp area
  if (teleportedOut) {
    playerController.ReleaseActor();

    auto& command = teleportController.TeleportIn(
      playerActor,
      returnPoint,
      Reverse(playerActor->GetHeading())
    );

    command.onFinish.Slot([=] {
      teleportedOut = false;
      playerController.ControlActor(playerActor);
    });
  }

#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void Overworld::SceneBase::onDraw(sf::RenderTexture& surface) {
  surface.draw(*bg);

  DrawMap(surface, sf::RenderStates::Default);

  surface.draw(emote);
  surface.draw(menuWidget);

  // Add the web account connection symbol
  surface.draw(webAccountIcon);

  surface.draw(textbox);
}

void Overworld::SceneBase::DrawMap(sf::RenderTarget& target, sf::RenderStates states) {

  auto offset = getView().getCenter() - camera.GetView().getCenter();

  map.move(offset);
  states.transform *= map.getTransform();
  map.move(-offset);

  DrawTiles(target, states);
  DrawSprites(target, states);
}

void Overworld::SceneBase::DrawTiles(sf::RenderTarget& target, sf::RenderStates states) {
  if (map.GetLayerCount() == 0) {
    return;
  }

  auto rows = map.GetRows();
  auto cols = map.GetCols();
  auto tileSize = map.GetTileSize();

  auto layer = map.GetLayer(0);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      auto& tile = layer.GetTile(j, i);
      if (tile.gid == 0) continue;

      auto& tileMeta = map.GetTileMeta(tile.gid);

      // failed to load tile
      if (tileMeta == nullptr) continue;

      auto& tileSprite = tileMeta->sprite;
      auto spriteBounds = tileSprite.getLocalBounds();

      auto originalOrigin = tileSprite.getOrigin();
      tileSprite.setOrigin(spriteBounds.width / 2, tileSize.y / 2);

      sf::Vector2f pos(static_cast<float>(j * tileSize.x * 0.5f), static_cast<float>(i * tileSize.y));
      auto ortho = map.WorldToScreen(pos);
      auto tileOffset = sf::Vector2f(-tileSize.x / 2 + spriteBounds.width / 2, tileSize.y + tileSize.y / 2 - spriteBounds.height);

      tileSprite.setPosition(ortho + tileMeta->offset + tileOffset);
      tileSprite.setRotation(tile.rotated ? -90 : 0);
      tileSprite.setScale(
        (tile.flippedHorizontal && !tile.rotated) || (tile.rotated && tile.flippedVertical) ? -1.0f : 1.0f,
        (tile.flippedVertical && !tile.rotated) || (tile.rotated && tile.flippedHorizontal) ? -1.0f : 1.0f
      );

      /*auto color = tileSprite.getColor();

      auto& [y, x] = PixelToRowCol(sf::Mouse::getPosition(*ENGINE.GetWindow()));

      bool hover = (y == i && x == j);

      if (hover) {
        tileSprite.setColor({ color.r, color.b, color.g, 200 });
      }*/

      if (/*cam && cam->IsInView(tileSprite)*/ true) {
        target.draw(tileSprite, states);
      }

      tileSprite.setOrigin(originalOrigin);
    }
  }
}

void Overworld::SceneBase::DrawSprites(sf::RenderTarget& target, sf::RenderStates states) const {
  for (auto& sprite : sprites) {
    auto iso = sprite.node->getPosition();
    auto ortho = map.WorldToScreen(iso);

    auto tileSprite = sprite.node;
    tileSprite->setPosition(ortho);

    if (/*cam && cam->IsInView(tileSprite->getSprite())*/ true) {
      target.draw(*tileSprite, states);
    }

    tileSprite->setPosition(iso);
  }
}

void Overworld::SceneBase::onEnd() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}

void Overworld::SceneBase::RefreshNaviSprite()
{
  // Only refresh all data and graphics if this is a new navi
  if (lastSelectedNavi == currentNavi) return;

  lastSelectedNavi = currentNavi;

  auto& meta = NAVIS.At(currentNavi);

  // refresh menu widget too
  int hp = std::atoi(meta.GetHPString().c_str());
  menuWidget.SetHealth(hp);
  menuWidget.SetMaxHealth(hp);

  // If coming back from navi select, the navi has changed, update it
  auto owPath = meta.GetOverworldAnimationPath();

  if (owPath.size()) {
    if (auto tex = meta.GetOverworldTexture()) {
      playerActor->setTexture(tex);
    }
    playerActor->LoadAnimations(Animation(owPath));

    // move the emote above the player's head
    float emoteY = -playerActor->getOrigin().y - emoteNode.getSprite().getLocalBounds().height / 2;
    emoteNode.setPosition(0, emoteY);

    auto iconTexture = meta.GetIconTexture();

    if (iconTexture) {
      menuWidget.UseIconTexture(iconTexture);
    }
    else {
      menuWidget.ResetIconTexture();
    }
  }
  else {
    Logger::Logf("Overworld animation not found for navi at index %i", currentNavi);
  }
}

void Overworld::SceneBase::NaviEquipSelectedFolder()
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

void Overworld::SceneBase::LoadBackground(const std::string& value)
{
  std::string str = value;
  std::transform(str.begin(), str.end(), str.begin(), [](auto in) {
    return std::tolower(in);
  });

  if (value == "undernet") {
    SetBackground(new UndernetBackground);
  }
  else if (value == "robot") {
    SetBackground(new RobotBackground);
  }
  else if (value == "misc") {
    SetBackground(new MiscBackground);
  }
  else if (value == "grave") {
    SetBackground(new GraveyardBackground);
  }
  else if (value == "weather") {
    SetBackground(new WeatherBackground);
  }
  else if (value == "medical") {
    SetBackground(new MedicalBackground);
  }
  else if (value == "acdc") {
    SetBackground(new ACDCBackground);
  }
  else if (value == "virus") {
    SetBackground(new VirusBackground);
  }
  else if (value == "judge") {
    SetBackground(new JudgeTreeBackground);
  }
  else if (value == "secret") {
    SetBackground(new SecretBackground);
  }
  else {
    SetBackground(new LanBackground);
  }

  // TODO: else if (isPNG(value)) { WriteToDisc(".areaname.png.value"); /* should cache too */ }
}

std::string Overworld::SceneBase::GetText(const std::string& path) {
  return FileUtil::Read(path);
}

std::shared_ptr<sf::Texture> Overworld::SceneBase::GetTexture(const std::string& path) {
  return Textures().LoadTextureFromFile(path);
}

std::shared_ptr<sf::SoundBuffer> Overworld::SceneBase::GetAudio(const std::string& path) {
  return Audio().LoadFromFile(path);
}


void Overworld::SceneBase::LoadMap(const std::string& data)
{
  XMLElement mapElement = parseXML(data);

  if (mapElement.name != "map") {
    Logger::Log("Failed to load map homepage.txt");
    return;
  }

  // organize elements
  XMLElement propertiesElement;
  std::vector<XMLElement> layerElements;
  std::vector<XMLElement> objectLayerElements;
  std::vector<XMLElement> tilesetElements;

  for (auto& child : mapElement.children) {
    if (child.name == "layer") {
      layerElements.push_back(child);
    }
    else if (child.name == "objectgroup") {
      objectLayerElements.push_back(child);
    }
    else if (child.name == "properties") {
      propertiesElement = child;
    }
    else if (child.name == "tileset") {
      tilesetElements.push_back(child);
    }
  }

  auto tileWidth = mapElement.GetAttributeInt("tilewidth");
  auto tileHeight = mapElement.GetAttributeInt("tileheight");

  // begin building map
  auto map = Map(
    mapElement.GetAttributeInt("width"),
    mapElement.GetAttributeInt("height"),
    tileWidth,
    tileHeight);

  // read custom properties
  for (auto propertyElement : propertiesElement.children) {
    auto propertyName = propertyElement.GetAttribute("name");
    auto propertyValue = propertyElement.GetAttribute("value");

    if (propertyName == "Background") {
      map.SetBackgroundName(propertyValue);
    }
    else if (propertyName == "Name") {
      map.SetName(propertyValue);
    }
    else if (propertyName == "Song") {
      map.SetSongPath(propertyValue);
    }
  }

  // load tilesets
  for (auto& tilesetElement : tilesetElements) {
    auto firstgid = static_cast<unsigned int>(tilesetElement.GetAttributeInt("firstgid"));
    auto source = tilesetElement.GetAttribute("source");

    if (source.find("/server", 0) != 0) {
      // client path
      // todo: hardcoded path oof, this will only be fine if all of our tiles are in this folder
      source = "resources/ow/tiles" + source.substr(source.rfind('/'));
    }

    auto tilesetData = GetText(source);

    auto tileset = ParseTileset(firstgid, tilesetData);

    for (auto i = 0; i < tileset->tileCount; i++) {
      map.SetTileset(firstgid + i, i, tileset);
    }
  }

  // build layers
  auto cols = map.GetCols();

  for (int i = std::max(layerElements.size(), objectLayerElements.size()) - 1; i >= 0; i--) {
    auto& layer = map.AddLayer();

    // add tiles to layer
    if (layerElements.size() > i) {
      auto& layerElement = layerElements[i];

      auto dataIt = std::find_if(layerElement.children.begin(), layerElement.children.end(), [](XMLElement& el) {return el.name == "data";});

      if (dataIt == layerElement.children.end()) {
        Logger::Log("Map layer missing data element!");
      }

      auto& dataElement = *dataIt;

      auto dataLen = dataElement.text.length();

      auto col = 0;
      auto row = 0;
      auto sliceStart = 0;

      for (auto i = 0; i <= dataLen; i++) {
        switch (dataElement.text[i]) {
        case '\0':
        case ',': {
          auto tileId = static_cast<unsigned int>(stoul("0" + dataElement.text.substr(sliceStart, i)));

          layer.SetTile(col, row, tileId);

          sliceStart = i + 1;
          col++;
          break;
        }
        case '\n':
          sliceStart = i + 1;
          col = 0;
          row++;
          break;
        default:
          break;
        }
      }
    }

    // add objects to layer
    if (objectLayerElements.size() > i) {
      auto& objectLayerElement = objectLayerElements[i];

      for (auto& child : objectLayerElement.children) {
        if (child.name != "object") {
          continue;
        }

        auto id = child.GetAttributeInt("id");
        auto gid = static_cast<unsigned int>(stoul("0" + child.GetAttribute("gid")));
        auto name = child.GetAttribute("name");
        auto position = sf::Vector2f(
          child.GetAttributeFloat("x"),
          child.GetAttributeFloat("y")
        );
        auto size = sf::Vector2f(
          child.GetAttributeFloat("width"),
          child.GetAttributeFloat("height")
        );
        auto rotation = child.GetAttributeFloat("rotation");
        auto visible = child.GetAttribute("visible") != "0";

        if (gid > 0) {
          auto tileObject = Map::TileObject(id, gid);
          tileObject.name = name;
          tileObject.visible = visible;
          tileObject.position = position;
          tileObject.size = size;
          tileObject.rotation = rotation;
          layer.AddTileObject(tileObject);
        }
      }
    }
  }

  LoadBackground(map.GetBackgroundName());
  menuWidget.SetArea(map.GetName());

  // cleanup data from the previous map
  this->map.RemoveSprites(*this);

  // replace the previous map
  this->map = std::move(map);
}

std::shared_ptr<Overworld::Map::Tileset> Overworld::SceneBase::ParseTileset(unsigned int firstgid, const std::string& data) {
  XMLElement tilesetElement = parseXML(data);
  auto tileCount = static_cast<unsigned int>(tilesetElement.GetAttributeInt("tilecount"));
  auto tileWidth = tilesetElement.GetAttributeInt("tilewidth");
  auto tileHeight = tilesetElement.GetAttributeInt("tileheight");
  auto columns = tilesetElement.GetAttributeInt("columns");

  sf::Vector2f offset;
  std::string texturePath;

  std::vector<XMLElement> tileElements(tileCount);

  for (auto& child : tilesetElement.children) {
    if (child.name == "tileoffset") {
      offset.x = child.GetAttributeInt("x");
      offset.y = child.GetAttributeInt("y");
    }
    else if (child.name == "image") {
      texturePath = child.GetAttribute("source");

      if (texturePath.find("/server", 0) != 0) {
        // client path
        // hardcoded
        texturePath = "resources/ow/tiles/" + texturePath;
      }
    }
    else if (child.name == "tile") {
      auto tileId = child.GetAttributeInt("id");

      if (tileElements.size() <= tileId) {
        tileElements.resize(tileId + 1);
      }

      tileElements[tileId] = child;
    }
  }

  // todo: work with Animation class directly?
  std::string animationString = "imagePath=\"./" + texturePath + "\"\n\n";

  auto objectAlignment = tilesetElement.GetAttribute("objectalignment");
  // default to bottom right
  auto origin = sf::Vector2f(tileWidth, tileHeight);

  if (objectAlignment == "top") {
    origin = sf::Vector2f(tileWidth / 2, 0);
  }
  else if (objectAlignment == "topleft") {
    origin = sf::Vector2f(0, 0);
  }
  else if (objectAlignment == "topright") {
    origin = sf::Vector2f(tileWidth, 0);
  }
  else if (objectAlignment == "bottom") {
    origin = sf::Vector2f(tileWidth / 2, tileHeight);
  }
  else if (objectAlignment == "bottomleft") {
    origin = sf::Vector2f(0, tileHeight);
  }

  std::string frameOffsetString = " originx=\"" + to_string(origin.x) + "\" originy=\"" + to_string(origin.y) + '"';
  std::string frameSizeString = " w=\"" + to_string(tileWidth) + "\" h=\"" + to_string(tileHeight) + '"';

  for (auto i = 0; i < tileElements.size(); i++) {
    auto& tileElement = tileElements[i];
    animationString += "animation state=\"" + to_string(i) + "\"\n";

    bool foundAnimation = false;

    for (auto& child : tileElement.children) {
      if (child.name == "animation") {
        for (auto& frameElement : child.children) {
          if (frameElement.name != "frame") {
            continue;
          }

          foundAnimation = true;

          auto tileId = frameElement.GetAttributeInt("tileid");
          auto duration = frameElement.GetAttributeInt("duration") / 1000.0;

          auto col = tileId % columns;
          auto row = tileId / columns;

          animationString += "frame duration=\"" + to_string(duration) + '"';
          animationString += " x=\"" + to_string(col * tileWidth) + "\" y=\"" + to_string(row * tileHeight) + '"';
          animationString += frameSizeString + frameOffsetString + '\n';
        }
      }
    }

    if (!foundAnimation) {
      auto col = i % columns;
      auto row = i / columns;

      animationString += "frame duration=\"0\"";
      animationString += " x=\"" + to_string(col * tileWidth) + "\" y=\"" + to_string(row * tileHeight) + '"';
      animationString += frameSizeString + frameOffsetString + '\n';
    }

    animationString += "\n";
  }

  Animation animation;
  animation.LoadWithData(animationString);

  auto tileset = Overworld::Map::Tileset{
   .name = tilesetElement.GetAttribute("name"),
   .firstGid = firstgid,
   .tileCount = tileCount,
   .offset = offset,
   .texture = GetTexture(texturePath),
   .animation = animation,
  };

  return std::make_shared<Overworld::Map::Tileset>(tileset);
}

void Overworld::SceneBase::TeleportUponReturn(const sf::Vector2f& position)
{
  teleportedOut = true;
  returnPoint = position;
}

const bool Overworld::SceneBase::HasTeleportedAway() const
{
  return teleportedOut;
}

void Overworld::SceneBase::SetBackground(Background* background)
{
  if (this->bg) {
    delete this->bg;
    this->bg = nullptr;
  }

  this->bg = background;
}

void Overworld::SceneBase::AddSprite(std::shared_ptr<SpriteProxyNode> _sprite, int layer)
{
  sprites.push_back({ _sprite, layer });
}

void Overworld::SceneBase::RemoveSprite(const std::shared_ptr<SpriteProxyNode> _sprite) {
  auto pos = std::find_if(sprites.begin(), sprites.end(), [_sprite](WorldSprite in) { return in.node == _sprite; });

  if (pos != sprites.end())
    sprites.erase(pos);
}

void Overworld::SceneBase::GotoChipFolder()
{
  gotoNextScene = true;
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<PushIn<direction::left>, milliseconds<500>>;
  getController().push<effect::to<FolderScene>>(folders);
}

void Overworld::SceneBase::GotoNaviSelect()
{
  // Navi select
  gotoNextScene = true;
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<Checkerboard, milliseconds<250>>;
  getController().push<effect::to<SelectNaviScene>>(currentNavi);
}

void Overworld::SceneBase::GotoConfig()
{
  // Config Select on PC
  gotoNextScene = true;
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<DiamondTileSwipe<direction::right>, milliseconds<500>>;
  getController().push<effect::to<ConfigScene>>();
}

void Overworld::SceneBase::GotoMobSelect()
{
  gotoNextScene = true;

  CardFolder* folder = nullptr;

  if (folders.GetFolder(0, folder)) {
    SelectMobScene::Properties props{ currentNavi, *folder, programAdvance, GetBackground()->Clone() };
    using effect = segue<PixelateBlackWashFade, milliseconds<500>>;
    Audio().Play(AudioType::CHIP_DESC);
    getController().push<effect::to<SelectMobScene>>(props);
  }
  else {
    Audio().Play(AudioType::CHIP_ERROR);
    Logger::Log("Cannot proceed to battles. You need 1 folder minimum.");
    gotoNextScene = false;
  }
}

void Overworld::SceneBase::GotoPVP()
{
  gotoNextScene = true;
  CardFolder* folder = nullptr;

  if (folders.GetFolder(0, folder)) {
    Audio().Play(AudioType::CHIP_DESC);
    using effect = segue<PushIn<direction::down>, milliseconds<500>>;
    getController().push<effect::to<PVPScene>>(static_cast<int>(currentNavi), *folder, programAdvance);
  }
  else {
    Audio().Play(AudioType::CHIP_ERROR);
    Logger::Log("Cannot proceed to battles. You need 1 folder minimum.");
    gotoNextScene = false;
  }
}

Overworld::QuadTree& Overworld::SceneBase::GetQuadTree()
{
  return quadTree;
}

Camera& Overworld::SceneBase::GetCamera()
{
  return camera;
}

Overworld::Map& Overworld::SceneBase::GetMap()
{
  return map;
}

std::shared_ptr<Overworld::Actor> Overworld::SceneBase::GetPlayer()
{
  return playerActor;
}

Overworld::PlayerController& Overworld::SceneBase::GetPlayerController()
{
  return playerController;
}

Overworld::TeleportController& Overworld::SceneBase::GetTeleportController()
{
  return teleportController;
}

SelectedNavi& Overworld::SceneBase::GetCurrentNavi()
{
  return currentNavi;
}

Background* Overworld::SceneBase::GetBackground()
{
  return this->bg;
}

AnimatedTextBox& Overworld::SceneBase::GetTextBox()
{
  return textbox;
}

void Overworld::SceneBase::OnEmoteSelected(Emotes emote)
{
  emoteNode.Emote(emote);
}


std::pair<unsigned, unsigned> Overworld::SceneBase::PixelToRowCol(const sf::Vector2i& px, const sf::RenderWindow& window) const
{
  sf::Vector2f ortho = window.mapPixelToCoords(px);

  // consider the point on screen relative to the camera focus
  auto pos = ortho - window.getView().getCenter() - camera.GetView().getCenter();
  auto iso = map.ScreenToWorld(pos);

  auto tileSize = map.GetTileSize();

  // divide by the tile size to get the integer grid values
  unsigned x = static_cast<unsigned>(iso.x / tileSize.x / 2);
  unsigned y = static_cast<unsigned>(iso.y / tileSize.y);

  return { y, x };
}

#ifdef __ANDROID__
void Overworld::SceneBase::StartupTouchControls() {
  ui.setScale(2.f, 2.f);

  uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
  uiAnimator.SetFrame(1, ui);
  ui.setPosition(100.f, 50.f);

  auto bounds = ui.getGlobalBounds();
  auto rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));
  auto& folderBtn = TouchArea::create(rect);

  folderBtn.onRelease([this](sf::Vector2i delta) {
    Logger::Log("folder released");

    gotoNextScene = true;
    Audio().Play(AudioType::CHIP_DESC);

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
    Audio().Play(AudioType::CHIP_DESC);

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
    Audio().Play(AudioType::CHIP_DESC);
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
      Audio().Play(AudioType::CHIP_DESC);
      using segue = swoosh::intent::segue<PixelateBlackWashFade, swoosh::intent::milli<500>>::to<SelectMobScene>;
      getController().push<segue>(currentNavi, *folder);
    }
    else {
      Audio().Play(AudioType::CHIP_ERROR);
      Logger::Log("Cannot proceed to mob select. Error selecting folder 'Default'.");
      gotoNextScene = false;
    }
  });

  mobBtn.onTouch([this]() {
    menuSelectionIndex = 3;
  });
}

void Overworld::SceneBase::ShutdownTouchControls() {
  TouchArea::free();
}
#endif