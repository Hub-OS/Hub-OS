#include <Swoosh/ActivityController.h>
#include <Segues/PushIn.h>
#include <Segues/Checkerboard.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/DiamondTileSwipe.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "bnOverworldSceneBase.h"

#include "../bnWebClientMananger.h"
#include "../Android/bnTouchArea.h"

#include "../bnFolderScene.h"
#include "../bnSelectNaviScene.h"
#include "../bnSelectMobScene.h"
#include "../bnLibraryScene.h"
#include "../bnConfigScene.h"
#include "../bnFolderScene.h"
#include "../bnCardFolderCollection.h"
#include "../bnCustomBackground.h"
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
auto MakeOptions = [](Overworld::SceneBase* scene) -> Overworld::PersonalMenu::OptionsList {
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
  personalMenu("Overworld", MakeOptions(this)),
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
  SetBackground(std::make_shared<LanBackground>());

  personalMenu.setScale(2.f, 2.f);
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
  playerActor->SetCollisionRadius(5);
  playerActor->AddNode(&emoteNode);

  emoteNode.SetLayer(-100);

  AddActor(playerActor);
  AddSprite(teleportController.GetBeam());

  map.setScale(2.f, 2.f);

  // emotes
  emote.OnSelect(std::bind(&Overworld::SceneBase::OnEmoteSelected, this, std::placeholders::_1));

  cameraTimer.reverse(true);
  cameraTimer.start();
}

void Overworld::SceneBase::onStart() {
#ifdef __ANDROID__
  StartupTouchControls();
#endif

  gotoNextScene = false;
}

void Overworld::SceneBase::onUpdate(double elapsed) {
  playerController.ListenToInputEvents(!IsInputLocked());

  if (!gotoNextScene) {
    HandleInput();
  }

  // handle custom tile collision
  if (!HasTeleportedAway() && teleportController.IsComplete()) {
    this->OnTileCollision();
  }

  /**
  * update all overworld objects and animations
  */

  // expecting glitches, manually update when actors move?
  spatialMap.Update();

  // update tile animations
  map.Update(*this, elapsed);

  if (gotoNextScene == false) {
    playerController.Update(elapsed);
    teleportController.Update(elapsed);
    minimap.SetPlayerPosition(map.WorldToScreen(playerActor->getPosition()));
  }

  for (auto& actor : actors) {
    actor->Update((float)elapsed, map, spatialMap);
  }

  // animations
  animElapsed += elapsed;

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
    [](const std::shared_ptr<WorldSprite>& A, const std::shared_ptr<WorldSprite>& B) {
    int A_layer_inv = -A->GetLayer();
    int B_layer_inv = -B->GetLayer();

    auto A_pos = A->getPosition();
    auto B_pos = B->getPosition();
    auto A_compare = A_pos.x + A_pos.y;
    auto B_compare = B_pos.x + B_pos.y;

    return std::tie(A_layer_inv, A_compare) < std::tie(B_layer_inv, B_compare);
  });

  // Loop the bg
  bg->Update((float)elapsed);

  // Update the widget
  personalMenu.Update((float)elapsed);

  // Update the emote widget
  emote.Update(elapsed);

  // Update the active emote
  emoteNode.Update(elapsed);

  // Update the textbox
  textbox.Update((float)elapsed);

  HandleCamera((float)elapsed);

  // Allow player to resync with remote account by pressing the pause action
  /*if (Input().Has(InputEvents::pressed_option)) {
      accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();
  }*/

  webAccountAnimator.Update((float)elapsed, webAccountIcon.getSprite());
}

void Overworld::SceneBase::HandleCamera(float elapsed) {
  camera.Update(elapsed);

  if (!cameraLocked) {
    // Follow the navi
    sf::Vector2f pos = map.WorldToScreen(playerActor->getPosition());
    camera.PlaceCamera(pos);
    return;
  }

  cameraTimer.update(sf::seconds(elapsed));

  if (cameraTimer.getElapsed().asMilliseconds() > 0 || cameraQueue.empty()) {
    return;
  }

  auto& event = cameraQueue.front();

  if (event.unlock) {
    UnlockCamera();
    cameraQueue = {};
    return;
  }

  if (event.slide) {
    camera.MoveCamera(event.position, event.duration);
  }
  else {
    camera.PlaceCamera(event.position);
  }

  cameraTimer.set(event.duration);
  cameraQueue.pop();
}

void Overworld::SceneBase::HandleInput() {

  // check to see if talk button was pressed
  if (emote.IsClosed() && !IsInputLocked()) {
    if (Input().Has(InputEvents::pressed_interact)) {
      OnInteract();
    }
  }

  if (emote.IsOpen()) {
    if (Input().Has(InputEvents::pressed_option)) {
      emote.Close();
    }
    return;
  }

  if (textbox.IsOpen()) {
    textbox.HandleInput(Input());
    return;
  }

  personalMenu.HandleInput(Input(), Audio());

  if (personalMenu.IsOpen()) {
    return;
  }

  if (Input().GetAnyKey() == sf::Keyboard::M) {
    showMinimap = !showMinimap;
    return;
  }

  if (Input().Has(InputEvents::pressed_option)) {
    emote.Open();
  }
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

  Audio().Stream(GetPath(map.GetSongPath()));

#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void Overworld::SceneBase::onDraw(sf::RenderTexture& surface) {
  surface.draw(*bg);

  DrawWorld(surface, sf::RenderStates::Default);

  if (showMinimap) {
    surface.draw(minimap);
    return;
  }

  surface.draw(emote);
  surface.draw(personalMenu);

  // Add the web account connection symbol
  surface.draw(webAccountIcon);

  surface.draw(textbox);
}

void Overworld::SceneBase::DrawWorld(sf::RenderTarget& target, sf::RenderStates states) {
  auto mapScale = GetMap().getScale();
  auto cameraCenter = camera.GetView().getCenter();
  cameraCenter.x = std::floor(cameraCenter.x) * mapScale.x;
  cameraCenter.y = std::floor(cameraCenter.y) * mapScale.y;

  auto offset = getView().getCenter() - cameraCenter;

  // prevents stitching artifacts between tiles
  offset.x = std::floor(offset.x);
  offset.y = std::floor(offset.y);

  states.transform.translate(offset);
  states.transform *= map.getTransform();

  DrawMap(target, states);
  DrawSprites(target, states);
}

void Overworld::SceneBase::DrawMap(sf::RenderTarget& target, sf::RenderStates states) {
  auto tileSize = map.GetTileSize();

  for (auto i = 0; i < map.GetLayerCount(); i++) {
    DrawLayer(target, states, i);

    // translate next layer
    states.transform.translate(0.f, -tileSize.y * 0.5f);
  }
}

void Overworld::SceneBase::DrawLayer(sf::RenderTarget& target, sf::RenderStates states, size_t index) {
  auto& layer = map.GetLayer(index);

  if (!layer.IsVisible()) return;

  auto rows = map.GetRows();
  auto cols = map.GetCols();
  auto tileSize = map.GetTileSize();

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      auto& tile = layer.GetTile(j, i);
      if (tile.gid == 0) continue;

      auto tileMeta = map.GetTileMeta(tile.gid);

      // failed to load tile
      if (tileMeta == nullptr) continue;

      auto& tileSprite = tileMeta->sprite;
      auto spriteBounds = tileSprite.getLocalBounds();

      auto originalOrigin = tileSprite.getOrigin();
      tileSprite.setOrigin(sf::Vector2f(sf::Vector2i(
        (int)spriteBounds.width / 2,
        tileSize.y / 2
      )));

      sf::Vector2i pos((j * tileSize.x) / 2, i * tileSize.y);
      auto ortho = map.WorldToScreen(sf::Vector2f(pos));
      auto tileOffset = sf::Vector2f(sf::Vector2i(
        -tileSize.x / 2 + (int)spriteBounds.width / 2,
        tileSize.y + tileSize.y / 2 - (int)spriteBounds.height
      ));

      tileSprite.setPosition(ortho + tileMeta->drawingOffset + tileOffset);
      tileSprite.setRotation(tile.rotated ? 90.0f : 0.0f);
      tileSprite.setScale(
        tile.flippedHorizontal ? -1.0f : 1.0f,
        tile.flippedVertical ? -1.0f : 1.0f
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
    auto iso = sprite->getPosition();
    auto ortho = map.WorldToScreen(iso);

    // prevents blurring and camera jittering with the player
    ortho.x = std::floor(ortho.x);
    ortho.y = std::floor(ortho.y);

    sprite->setPosition(ortho);

    if (/*cam && cam->IsInView(sprite->getSprite())*/ true) {
      target.draw(*sprite, states);
    }

    sprite->setPosition(iso);
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
  personalMenu.SetHealth(hp);
  personalMenu.SetMaxHealth(hp);

  // If coming back from navi select, the navi has changed, update it
  const auto& owPath = meta.GetOverworldAnimationPath();

  if (owPath.size()) {
    if (auto tex = Textures().LoadTextureFromFile(meta.GetOverworldTexturePath())) {
      playerActor->setTexture(tex);
    }
    playerActor->LoadAnimations(Animation(owPath));

    // move the emote above the player's head
    float emoteY = -playerActor->getOrigin().y - emoteNode.getSprite().getLocalBounds().height / 2;
    emoteNode.setPosition(0, emoteY);

    auto iconTexture = meta.GetIconTexture();

    if (iconTexture) {
      personalMenu.UseIconTexture(iconTexture);
    }
    else {
      personalMenu.ResetIconTexture();
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

void Overworld::SceneBase::LoadBackground(const Map& map, const std::string& value)
{
  if (value == "custom") {
    const auto& texture = GetTexture(map.GetBackgroundCustomTexturePath());
    const auto& animationData = GetText(map.GetBackgroundCustomAnimationPath());
    const auto& velocity = map.GetBackgroundCustomVelocity();

    Animation animation;
    animation.LoadWithData(animationData);

    SetBackground(std::make_shared<CustomBackground>(texture, animation, velocity));
  }
  else if (value == "undernet") {
    SetBackground(std::make_shared<UndernetBackground>());
  }
  else if (value == "robot") {
    SetBackground(std::make_shared<RobotBackground>());
  }
  else if (value == "misc") {
    SetBackground(std::make_shared<MiscBackground>());
  }
  else if (value == "grave") {
    SetBackground(std::make_shared<GraveyardBackground>());
  }
  else if (value == "weather") {
    SetBackground(std::make_shared<WeatherBackground>());
  }
  else if (value == "medical") {
    SetBackground(std::make_shared<MedicalBackground>());
  }
  else if (value == "acdc") {
    SetBackground(std::make_shared<ACDCBackground>());
  }
  else if (value == "virus") {
    SetBackground(std::make_shared<VirusBackground>());
  }
  else if (value == "judge") {
    SetBackground(std::make_shared<JudgeTreeBackground>());
  }
  else if (value == "secret") {
    SetBackground(std::make_shared<SecretBackground>());
  }
  else {
    SetBackground(std::make_shared<LanBackground>());
  }
}

std::string Overworld::SceneBase::GetPath(const std::string& path) {
  return path;
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
  for (const auto& propertyElement : propertiesElement.children) {
    auto propertyName = propertyElement.GetAttribute("name");
    auto propertyValue = propertyElement.GetAttribute("value");

    if (propertyName == "Background") {
      std::transform(propertyValue.begin(), propertyValue.end(), propertyValue.begin(), [](auto in) {
        return std::tolower(in);
      });

      map.SetBackgroundName(propertyValue);
    }
    else if (propertyName == "Background Texture") {
      map.SetBackgroundCustomTexturePath(propertyValue);
    }
    else if (propertyName == "Background Animation") {
      map.SetBackgroundCustomAnimationPath(propertyValue);
    }
    else if (propertyName == "Background Vel X") {
      auto velocity = map.GetBackgroundCustomVelocity();
      velocity.x = propertyElement.GetAttributeFloat("value");

      map.SetBackgroundCustomVelocity(velocity);
    }
    else if (propertyName == "Background Vel Y") {
      auto velocity = map.GetBackgroundCustomVelocity();
      velocity.y = propertyElement.GetAttributeFloat("value");

      map.SetBackgroundCustomVelocity(velocity);
    }
    else if (propertyName == "Name") {
      map.SetName(propertyValue);
    }
    else if (propertyName == "Song") {
      map.SetSongPath(propertyValue);
    }
  }

  // load tilesets
  for (auto& mapTilesetElement : tilesetElements) {
    auto firstgid = static_cast<unsigned int>(mapTilesetElement.GetAttributeInt("firstgid"));
    auto source = mapTilesetElement.GetAttribute("source");

    if (source.find("/server", 0) != 0) {
      // client path
      // todo: hardcoded path oof, this will only be fine if all of our tiles are in this folder
      size_t pos = source.rfind('/');

      if (pos != std::string::npos) {
        source = "resources/ow/tiles" + source.substr(pos);
      }
    }

    XMLElement tilesetElement = parseXML(GetText(source));
    auto tileset = ParseTileset(tilesetElement, firstgid);
    auto tileMetas = ParseTileMetas(tilesetElement, *tileset);

    for (auto& tileMeta : tileMetas) {
      map.SetTileset(tileset, tileMeta);
    }
  }

  int layerCount = (int)std::max(layerElements.size(), objectLayerElements.size());

  // build layers
  for (int i = 0; i < layerCount; i++) {
    auto& layer = map.AddLayer();

    // add tiles to layer
    if (layerElements.size() > i) {
      auto& layerElement = layerElements[i];

      layer.SetVisible(layerElement.attributes["visible"] != "0");

      auto dataIt = std::find_if(layerElement.children.begin(), layerElement.children.end(), [](XMLElement& el) {return el.name == "data";});
      if (dataIt == layerElement.children.end()) {
        Logger::Log("Map layer missing data element!");
        continue;
      }

      auto& dataElement = *dataIt;

      auto dataLen = dataElement.text.length();

      auto col = 0;
      auto row = 0;
      auto sliceStart = 0;

      for (size_t i = 0; i <= dataLen; i++) {
        switch (dataElement.text[i]) {
        case '\0':
        case ',': {
          auto tileId = static_cast<unsigned int>(stoul("0" + dataElement.text.substr(sliceStart, i - sliceStart)));

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

        if (child.HasAttribute("gid")) {
          auto tileObject = TileObject::From(child);
          layer.AddTileObject(tileObject);
        }
        else {
          auto shapeObject = ShapeObject::From(child);

          if (shapeObject) {
            layer.AddShapeObject(std::move(shapeObject.value()));
          }
        }
      }
    }
  }

  bool backgroundDiffers = map.GetBackgroundName() != this->map.GetBackgroundName() || (
    map.GetBackgroundName() == "custom" && (
      map.GetBackgroundCustomTexturePath() != this->map.GetBackgroundCustomTexturePath() ||
      map.GetBackgroundCustomAnimationPath() != this->map.GetBackgroundCustomAnimationPath() ||
      map.GetBackgroundCustomVelocity() != this->map.GetBackgroundCustomVelocity()
      )
    );

  if (backgroundDiffers) {
    LoadBackground(map, map.GetBackgroundName());
  }

  if (map.GetSongPath() != this->map.GetSongPath()) {
    Audio().Stream(GetPath(map.GetSongPath()));
  }

  personalMenu.SetArea(map.GetName());

  // cleanup data from the previous map
  this->map.RemoveSprites(*this);

  // replace the previous map
  this->map = std::move(map);

  // scale to the game resolution
  this->map.setScale(2.f, 2.f);

  minimap = Minimap::CreateFrom(this->map.GetName(), this->map);
  minimap.setScale(2.f, 2.f);
}

std::shared_ptr<Overworld::Tileset> Overworld::SceneBase::ParseTileset(const XMLElement& tilesetElement, unsigned int firstgid) {
  auto tileCount = static_cast<unsigned int>(tilesetElement.GetAttributeInt("tilecount"));
  auto tileWidth = tilesetElement.GetAttributeInt("tilewidth");
  auto tileHeight = tilesetElement.GetAttributeInt("tileheight");
  auto columns = tilesetElement.GetAttributeInt("columns");

  sf::Vector2f drawingOffset;
  std::string texturePath;
  Projection orientation = Projection::Orthographic;
  CustomProperties customProperties;

  std::vector<XMLElement> tileElements(tileCount);

  for (auto& child : tilesetElement.children) {
    if (child.name == "tileoffset") {
      drawingOffset.x = child.GetAttributeFloat("x");
      drawingOffset.y = child.GetAttributeFloat("y");
    }
    else if (child.name == "grid") {
      orientation =
        child.GetAttribute("orientation") == "isometric" ?
        Projection::Isometric
        : Projection::Orthographic;
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
        size_t sz = static_cast<size_t>(tileId) + 1;
        tileElements.resize(sz);
      }

      tileElements[tileId] = child;
    }
    else if (child.name == "properties") {
      customProperties = CustomProperties::From(child);
    }
  }

  // todo: work with Animation class directly?
  std::string animationString = "imagePath=\"./" + texturePath + "\"\n\n";

  auto objectAlignment = tilesetElement.GetAttribute("objectalignment");
  // default to bottom right
  auto alignmentOffset = sf::Vector2i(-tileWidth, -tileHeight);

  if (objectAlignment == "top") {
    alignmentOffset = sf::Vector2i(-tileWidth / 2, 0);
  }
  else if (objectAlignment == "topleft") {
    alignmentOffset = sf::Vector2i(0, 0);
  }
  else if (objectAlignment == "topright") {
    alignmentOffset = sf::Vector2i(-tileWidth, 0);
  }
  else if (objectAlignment == "center") {
    alignmentOffset = sf::Vector2i(-tileWidth / 2, -tileHeight / 2);
  }
  else if (objectAlignment == "left") {
    alignmentOffset = sf::Vector2i(0, -tileHeight / 2);
  }
  else if (objectAlignment == "right") {
    alignmentOffset = sf::Vector2i(-tileWidth, -tileHeight / 2);
  }
  else if (objectAlignment == "bottom") {
    alignmentOffset = sf::Vector2i(-tileWidth / 2, -tileHeight);
  }
  else if (objectAlignment == "bottomleft") {
    alignmentOffset = sf::Vector2i(0, -tileHeight);
  }

  std::string frameOffsetString = " originx=\"" + to_string(tileWidth / 2) + "\" originy=\"" + to_string(tileHeight / 2) + '"';
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

  auto tileset = Overworld::Tileset{
    tilesetElement.GetAttribute("name"),
    firstgid,
    tileCount,
    drawingOffset,
    sf::Vector2f(alignmentOffset),
    orientation,
    customProperties,
    GetTexture(texturePath),
    animation
  };

  return std::make_shared<Overworld::Tileset>(tileset);
}

std::vector<std::shared_ptr<Overworld::TileMeta>>
Overworld::SceneBase::ParseTileMetas(const XMLElement& tilesetElement, const Overworld::Tileset& tileset) {
  auto tileCount = static_cast<unsigned int>(tilesetElement.GetAttributeInt("tilecount"));

  std::vector<XMLElement> tileElements(tileCount);

  for (auto& child : tilesetElement.children) {
    if (child.name == "tile") {
      auto tileId = child.GetAttributeInt("id");

      if (tileElements.size() <= tileId) {
        size_t sz = static_cast<size_t>(tileId) + 1;
        tileElements.resize(sz);
      }

      tileElements[tileId] = child;
    }
  }

  std::vector<std::shared_ptr<Overworld::TileMeta>> tileMetas;
  auto tileId = 0;
  auto tileGid = tileset.firstGid;

  for (auto& tileElement : tileElements) {
    std::vector<std::unique_ptr<Shape>> collisionShapes;
    CustomProperties customProperties;

    for (auto& child : tileElement.children) {
      if (child.name == "properties") {
        customProperties = CustomProperties::From(child);
        continue;
      }

      if (child.name != "objectgroup") {
        continue;
      }

      for (auto& objectElement : child.children) {
        auto shape = Overworld::Shape::From(objectElement);

        if (shape) {
          collisionShapes.push_back(std::move(shape.value()));
        }
      }
    }

    auto tileMeta = std::make_shared<Overworld::TileMeta>(
      tileset,
      tileId,
      tileGid,
      tileset.drawingOffset,
      tileset.alignmentOffset,
      tileElement.GetAttribute("type"),
      customProperties,
      std::move(collisionShapes)
      );

    tileMetas.push_back(tileMeta);
    tileId += 1;
    tileGid += 1;
  }

  return tileMetas;
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

void Overworld::SceneBase::SetBackground(const std::shared_ptr<Background>& background)
{
  this->bg = background;
}

void Overworld::SceneBase::AddSprite(const std::shared_ptr<WorldSprite>& sprite)
{
  sprites.push_back(sprite);
}

void Overworld::SceneBase::RemoveSprite(const std::shared_ptr<WorldSprite>& sprite) {
  auto pos = std::find(sprites.begin(), sprites.end(), sprite);

  if (pos != sprites.end())
    sprites.erase(pos);
}

void Overworld::SceneBase::AddActor(const std::shared_ptr<Actor>& actor) {
  actors.push_back(actor);
  spatialMap.AddActor(actor);
  AddSprite(actor);
}

void Overworld::SceneBase::RemoveActor(const std::shared_ptr<Actor>& actor) {
  spatialMap.RemoveActor(actor);
  RemoveSprite(actor);

  auto pos = std::find(actors.begin(), actors.end(), actor);

  if (pos != actors.end())
    actors.erase(pos);
}

bool Overworld::SceneBase::IsInputLocked() {
  return inputLocked || !personalMenu.IsClosed() || !textbox.IsClosed() || gotoNextScene;
}

void Overworld::SceneBase::LockInput() {
  inputLocked = true;
}

void Overworld::SceneBase::UnlockInput() {
  inputLocked = false;
}

bool Overworld::SceneBase::IsCameraLocked() {
  return cameraLocked;
}

void Overworld::SceneBase::LockCamera() {
  cameraLocked = true;
}

void Overworld::SceneBase::UnlockCamera() {
  cameraLocked = false;
}

void Overworld::SceneBase::QueuePlaceCamera(sf::Vector2f position, sf::Time holdTime) {
  LockCamera();

  QueuedCameraEvent event{
    false,
    false,
    position,
    holdTime
  };

  cameraQueue.push(event);
}

void Overworld::SceneBase::QueueMoveCamera(sf::Vector2f position, sf::Time duration) {
  LockCamera();

  QueuedCameraEvent event{
    false,
    true,
    position,
    duration
  };

  cameraQueue.push(event);
}

void Overworld::SceneBase::QueueUnlockCamera() {
  LockCamera();

  QueuedCameraEvent event{
    true // unlock
  };

  cameraQueue.push(event);
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
    SelectMobScene::Properties props{ currentNavi, *folder, programAdvance, bg };
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

Overworld::Minimap& Overworld::SceneBase::GetMinimap()
{
  return minimap;
}

Overworld::SpatialMap& Overworld::SceneBase::GetSpatialMap()
{
  return spatialMap;
}

std::vector<std::shared_ptr<Overworld::Actor>>& Overworld::SceneBase::GetActors()
{
  return actors;
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

std::shared_ptr<Background> Overworld::SceneBase::GetBackground()
{
  return this->bg;
}

Overworld::TextBox& Overworld::SceneBase::GetTextBox()
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