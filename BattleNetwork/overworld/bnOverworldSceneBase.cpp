#include <Swoosh/ActivityController.h>
#include <Segues/PushIn.h>
#include <Segues/Checkerboard.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/DiamondTileSwipe.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "bnOverworldSceneBase.h"
#include "bnOverworldTiledMapLoader.h"

#include "../bnWebClientMananger.h"
#include "../Android/bnTouchArea.h"
#include "../bnCurrentTime.h"

#include "../bnFolderScene.h"
#include "../bnSelectNaviScene.h"
#include "../bnSelectMobScene.h"
#include "../bnLibraryScene.h"
#include "../bnConfigScene.h"
#include "../bnFolderScene.h"
#include "../bnKeyItemScene.h"
#include "../bnMailScene.h"
#include "../bnVendorScene.h"
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

#include "../netplay/bnMatchMakingScene.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;

using namespace swoosh::types;

/// \brief Thunk to populate menu options to callbacks
namespace {
  auto MakeOptions = [](Overworld::SceneBase* scene) -> Overworld::PersonalMenu::OptionsList {
    return {
      { "chip_folder", std::bind(&Overworld::SceneBase::GotoChipFolder, scene) },
      { "navi",        std::bind(&Overworld::SceneBase::GotoNaviSelect, scene) },
      { "mail",        std::bind(&Overworld::SceneBase::GotoMail, scene) },
      { "key_items",   std::bind(&Overworld::SceneBase::GotoKeyItems, scene) },
      { "mob_select",  std::bind(&Overworld::SceneBase::GotoMobSelect, scene) },
      { "config",      std::bind(&Overworld::SceneBase::GotoConfig, scene) },
      { "sync",        std::bind(&Overworld::SceneBase::GotoPVP, scene) }
    };
  };
}

Overworld::SceneBase::SceneBase(swoosh::ActivityController& controller) :
  lastIsConnectedState(false),
  personalMenu("Overworld", ::MakeOptions(this)),
  camera(controller.getWindow().getView()),
  Scene(controller),
  map(0, 0, 0, 0),
  time(Font::Style::thick),
  playerActor(std::make_shared<Overworld::Actor>("You"))
{
  // When we reach the menu scene we need to load the player information
  // before proceeding to next sub menus
  webAccountIcon.setTexture(LOAD_TEXTURE(WEBACCOUNT_STATUS));
  webAccountIcon.setScale(2.f, 2.f);
  webAccountIcon.setPosition(4, getController().getVirtualWindowSize().y - 44.0f);
  webAccountAnimator = Animation("resources/ui/webaccount_icon.animation");
  webAccountAnimator.Load();
  webAccountAnimator.SetAnimation("NO_CONNECTION");

  // Draws the scrolling background
  SetBackground(std::make_shared<LanBackground>());

  // set the missing texture for all actor objects
  auto missingTexture = Textures().LoadTextureFromFile("resources/ow/missing.png");
  Overworld::Actor::SetMissingTexture(missingTexture);

  personalMenu.setScale(2.f, 2.f);
  //emote.setScale(2.f, 2.f);

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
  playerActor->SetCollisionRadius(4);
  playerActor->AddNode(&emoteNode);

  emoteNode.SetLayer(-100);
  emoteNode.setScale(0.5f, 0.5f);

  AddActor(playerActor);
  AddSprite(teleportController.GetBeam());

  map.setScale(2.f, 2.f);

  // emotes
  emote.OnSelect(std::bind(&Overworld::SceneBase::OnEmoteSelected, this, std::placeholders::_1));

  // clock
  time.setPosition(480 - 4.f, 4.f);
  time.setScale(2.f, 2.f);
}

void Overworld::SceneBase::onStart() {
#ifdef __ANDROID__
  StartupTouchControls();
#endif

  gotoNextScene = false;

  // TODO: Take out after endpoints are added to server @Konst
  Inbox& inbox = playerSession.inbox;

  sf::Texture mugshot = *Textures().LoadTextureFromFile("resources/ow/prog/prog_mug.png");
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::announcement, "Welcome", "NO-TITLE", "This is your first email!", mugshot });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::dm, "HELLO", "KERISTERO", "try gravy" });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::dm_w_attachment, "ELLO", "DESTROYED", "ello govna" });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::important, "FIRE", "NO-TITLE", "There's a fire in the undernet!", mugshot });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::mission, "MISSING", "ANON", "Can you find my missing data? It would really help me out right now... Or don't if it's too hard, I understand..." });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::dm, "Test", "NO-TITLE", "Just another test.", mugshot });
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

  // animations
  animElapsed += elapsed;

  // expecting glitches, manually update when actors move?
  spatialMap.Update();

  // update tile animations
  map.Update(*this, animElapsed);

  if (gotoNextScene == false) {
    playerController.Update(elapsed);
    teleportController.Update(elapsed);

    // factor in player's position for the minimap
    sf::Vector2f screenPos = map.WorldToScreen(playerActor->getPosition());
    screenPos.y -= playerActor->GetElevation() * map.GetTileSize().y * 0.5f;
    screenPos.x = std::floor(screenPos.x);
    screenPos.y = std::floor(screenPos.y);

    // update minimap
    auto playerTilePos = map.WorldToTileSpace(playerActor->getPosition());
    bool isConcealed = map.IsConcealed(sf::Vector2i(playerTilePos), playerActor->GetLayer());
    minimap.SetPlayerPosition(screenPos, isConcealed);
  }

  for (auto& actor : actors) {
    actor->Update((float)elapsed, map, spatialMap);
  }

#ifdef __ANDROID__
  if (gotoNextScene)
    return; // keep the screen looking the same when we come back
#endif

  if (WEBCLIENT.IsLoggedIn() && accountCommandResponse.valid() && is_ready(accountCommandResponse)) {
    try {
      webAccount = accountCommandResponse.get();
      Logger::Logf("You have %i folders on your account", webAccount.folders.size());
      WEBCLIENT.CacheTextureData(webAccount);
      folders = CardFolderCollection::ReadFromWebAccount(webAccount);
      programAdvance = PA::ReadFromWebAccount(webAccount);

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

  auto layerCount = map.GetLayerCount() + 1;

  if (spriteLayers.size() != layerCount) {
    spriteLayers.resize(layerCount);
  }

  for (auto i = 0; i < layerCount; i++) {
    auto& spriteLayer = spriteLayers[i];
    auto elevation = (float)i;
    bool isTopLayer = elevation + 1 == layerCount;
    bool isBottomLayer = elevation == 0;

    spriteLayer.clear();

    // match sprites to layer
    for (auto& sprite : sprites) {
      // use ceil(elevation) + 1 instead of GetLayer to prevent sorting issues with stairs
      auto spriteElevation = std::ceil(sprite->GetElevation()) + 1;
      if (spriteElevation == elevation || (isTopLayer && spriteElevation >= layerCount) || (isBottomLayer && spriteElevation < 0)) {
        spriteLayer.push_back(sprite);
      }
    }

    // sort sprites within the layer
    std::sort(spriteLayer.begin(), spriteLayer.end(),
      [](const std::shared_ptr<WorldSprite>& A, const std::shared_ptr<WorldSprite>& B) {
      auto& A_pos = A->getPosition();
      auto& B_pos = B->getPosition();
      auto A_compare = A_pos.x + A_pos.y;
      auto B_compare = B_pos.x + B_pos.y;

      return A_compare < B_compare;
    });
  }

  // Loop the bg
  bg->Update((float)elapsed);

  // Update the widget
  personalMenu.Update((float)elapsed);

  // Update the emote widget
  emote.Update(elapsed);

  // Update the active emote
  emoteNode.Update(elapsed);

  // Update the textbox
  menuSystem.Update((float)elapsed);

  HandleCamera((float)elapsed);

  // Allow player to resync with remote account by pressing the pause action
  /*if (Input().Has(InputEvents::pressed_option)) {
      accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();
  }*/

  webAccountAnimator.Update((float)elapsed, webAccountIcon.getSprite());
}

void Overworld::SceneBase::HandleCamera(float elapsed) {
  if (!cameraLocked) {
    // Follow the navi
    sf::Vector2f pos = playerActor->getPosition();

    if (teleportedOut) {
      pos = { returnPoint.x, returnPoint.y };
    }

    pos = map.WorldToScreen(pos);
    pos.y -= playerActor->GetElevation() * map.GetTileSize().y / 2.0f;

    teleportedOut ? camera.MoveCamera(pos, sf::seconds(0.5)) : camera.PlaceCamera(pos);
    camera.Update(elapsed);
    return;
  }

  camera.Update(elapsed);
}

void Overworld::SceneBase::HandleInput() {
  if (showMinimap) {
    sf::Vector2f panning = {};

    if (Input().Has(InputEvents::held_ui_left)) {
      panning.x -= 1.f;
    }

    if (Input().Has(InputEvents::held_ui_right)) {
      panning.x += 1.f;
    }

    if (Input().Has(InputEvents::held_ui_up)) {
      panning.y -= 1.f;
    }

    if (Input().Has(InputEvents::held_ui_down)) {
      panning.y += 1.f;
    }

    if (Input().GetConfigSettings().GetInvertMinimap()) {
      panning.x *= -1.f;
      panning.y *= -1.f;
    }

    if (Input().Has(InputEvents::pressed_map)) {
      showMinimap = false;
      minimap.ResetPanning();
    }

    minimap.Pan(panning);
    return;
  }

  if (!emote.IsClosed()) {
    if (Input().Has(InputEvents::pressed_option)) {
      emote.Close();
    }
    return;
  }

  if (!menuSystem.IsClosed()) {
    menuSystem.HandleInput(Input(), getController().getWindow());
    return;
  }

  if (!personalMenu.IsClosed()) {
    personalMenu.HandleInput(Input(), Audio());
    return;
  }

  // check to see if talk button was pressed
  if (!IsInputLocked()) {
    if (Input().Has(InputEvents::pressed_interact)) {
      OnInteract(Interaction::action);
    }
    if (Input().Has(InputEvents::pressed_shoulder_left)) {
      OnInteract(Interaction::inspect);
    }
  }

  if (Input().Has(InputEvents::pressed_pause) && !Input().Has(InputEvents::pressed_cancel)) {
    personalMenu.Open();
    Audio().Play(AudioType::CHIP_DESC);
  }
  else if (Input().Has(InputEvents::pressed_option)) {
    emote.Open();
  }
  else if (Input().Has(InputEvents::pressed_map)) {
    showMinimap = true;
    minimap.ResetPanning();
  }
}

void Overworld::SceneBase::onLeave() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif

  gotoNextScene = true;
}

void Overworld::SceneBase::onExit()
{
  emoteNode.Reset();
}

void Overworld::SceneBase::onEnter()
{
  gotoNextScene = true;
  RefreshNaviSprite();
}

void Overworld::SceneBase::onResume() {

  auto guestAccount = !WEBCLIENT.IsLoggedIn();

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

    teleportedOut = false;
    command.onFinish.Slot([=] {
      playerController.ControlActor(playerActor);
    });
  }

  Audio().Stream(GetPath(map.GetSongPath()), true);

#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void Overworld::SceneBase::onDraw(sf::RenderTexture& surface) {
  if (showMinimap) {
    surface.draw(minimap);
    return;
  }

  if (menuSystem.IsFullscreen()) {
    surface.draw(menuSystem);
    return;
  }

  if (bg) {
    surface.draw(*bg);
  }

  DrawWorld(surface, sf::RenderStates::Default);

  surface.draw(emote);
  surface.draw(personalMenu);

  // Add the web account connection symbol
  surface.draw(webAccountIcon);

  PrintTime(surface);

  // This will mask everything before this line with camera fx
  surface.draw(camera.GetLens());

  // always see menus
  surface.draw(menuSystem);
}

void Overworld::SceneBase::DrawWorld(sf::RenderTarget& target, sf::RenderStates states) {
  const auto& mapScale = GetMap().getScale();
  sf::Vector2f cameraCenter = camera.GetView().getCenter();
  cameraCenter.x = std::floor(cameraCenter.x) * mapScale.x;
  cameraCenter.y = std::floor(cameraCenter.y) * mapScale.y;

  auto offset = getView().getCenter() - cameraCenter;

  // prevents stitching artifacts between tiles
  offset.x = std::floor(offset.x);
  offset.y = std::floor(offset.y);

  states.transform.translate(offset);
  states.transform *= map.getTransform();

  auto tileSize = map.GetTileSize();
  auto mapLayerCount = map.GetLayerCount();

  // there should be mapLayerCount + 1 sprite layers
  for (auto i = 0; i < mapLayerCount + 1; i++) {

    // loop is based on expected sprite layers
    // make sure we dont try to draw an extra map layer and segfault
    if (i < mapLayerCount) {
      DrawMapLayer(target, states, i, mapLayerCount);
    }

    // save from possible map layer count change after OverworldSceneBase::Update
    if (i < spriteLayers.size()) {
      DrawSpriteLayer(target, states, i);
    }

    // translate next layer
    states.transform.translate(0.f, -tileSize.y * 0.5f);
  }
}

void Overworld::SceneBase::DrawMapLayer(sf::RenderTarget& target, sf::RenderStates states, size_t index, size_t maxLayers) {
  auto& layer = map.GetLayer(index);

  if (!layer.IsVisible()) {
    return;
  }

  auto rows = map.GetRows();
  auto cols = map.GetCols();
  auto tileSize = map.GetTileSize();

  const auto TILE_PADDING = 3;
  auto screenSize = sf::Vector2f(target.getSize()) / map.getScale().x;

  auto verticalTileCount = (int)std::ceil((screenSize.y / (float)tileSize.y) * 2.0f);
  auto horizontalTileCount = (int)std::ceil(screenSize.x / (float)tileSize.x);

  auto screenTopLeft = camera.GetView().getCenter() - screenSize / 2.0f;
  auto tileSpaceStart = sf::Vector2i(map.WorldToTileSpace(map.ScreenToWorld(screenTopLeft) * map.getScale().x));
  auto verticalOffset = (int)index;

  for (int i = -TILE_PADDING; i < verticalTileCount + TILE_PADDING; i++) {
    auto verticalStart = (verticalOffset + i) / 2;
    auto rowOffset = (verticalOffset + i) % 2;

    for (int j = -TILE_PADDING; j < horizontalTileCount + rowOffset + TILE_PADDING; j++) {
      int row = tileSpaceStart.y + verticalStart - j + rowOffset;
      int col = tileSpaceStart.x + verticalStart + j;

      auto tile = layer.GetTile(col, row);
      if (!tile || tile->gid == 0) continue;

      auto tileMeta = map.GetTileMeta(tile->gid);

      // failed to load tile
      if (tileMeta == nullptr) continue;

      auto& tileSprite = tileMeta->sprite;
      auto spriteBounds = tileSprite.getLocalBounds();

      const auto& originalOrigin = tileSprite.getOrigin();
      tileSprite.setOrigin(sf::Vector2f(sf::Vector2i(
        (int)spriteBounds.width / 2,
        tileSize.y / 2
      )));

      sf::Vector2i pos((col * tileSize.x) / 2, row * tileSize.y);
      auto ortho = map.WorldToScreen(sf::Vector2f(pos));
      auto tileOffset = sf::Vector2f(sf::Vector2i(
        -tileSize.x / 2 + (int)spriteBounds.width / 2,
        tileSize.y + tileSize.y / 2 - (int)spriteBounds.height
      ));

      tileSprite.setPosition(ortho + tileMeta->drawingOffset + tileOffset);
      tileSprite.setRotation(tile->rotated ? 90.0f : 0.0f);
      tileSprite.setScale(
        tile->flippedHorizontal ? -1.0f : 1.0f,
        tile->flippedVertical ? -1.0f : 1.0f
      );

      sf::Color originalColor = tileSprite.getColor();
      if (map.HasShadow({ col, row }, (int)index)) {
        sf::Uint8 r, g, b;
        r = sf::Uint8(originalColor.r * 0.65);
        b = sf::Uint8(originalColor.b * 0.65);
        g = sf::Uint8(originalColor.g * 0.65);

        tileSprite.setColor(sf::Color(r, g, b, originalColor.a));
      }
      target.draw(tileSprite, states);
      tileSprite.setColor(originalColor);

      tileSprite.setOrigin(originalOrigin);
    }
  }
}


void Overworld::SceneBase::DrawSpriteLayer(sf::RenderTarget& target, sf::RenderStates states, size_t index) {
  auto rows = map.GetRows();
  auto cols = map.GetCols();
  auto tileSize = map.GetTileSize();
  auto mapLayerCount = map.GetLayerCount();
  auto elevation = (float)index;

  for (auto& sprite : spriteLayers[index]) {
    const sf::Vector2f worldPos = sprite->getPosition();
    sf::Vector2f screenPos = map.WorldToScreen(worldPos);
    screenPos.y -= (sprite->GetElevation() - elevation) * tileSize.y * 0.5f;

    // prevents blurring and camera jittering with the player
    screenPos.x = std::floor(screenPos.x);
    screenPos.y = std::floor(screenPos.y);

    sprite->setPosition(screenPos);

    sf::Vector2i gridPos = sf::Vector2i(map.WorldToTileSpace(worldPos));

    if (/*cam && cam->IsInView(sprite->getSprite())*/ true) {
      sf::Color originalColor = sprite->getColor();
      auto layer = int(index) - 1;
      // NOTE: we snap players so elevations with floating decimals, even if not precise, will 
      //       let us know if we're on the correct elevation or not
      if (sprite->GetElevation() != elevation - 1.f) {
        layer -= 1;
      }

      if (map.HasShadow(gridPos, layer)) {
        sf::Uint8 r, g, b;
        r = sf::Uint8(originalColor.r * 0.5);
        b = sf::Uint8(originalColor.b * 0.5);
        g = sf::Uint8(originalColor.g * 0.5);
        sprite->setColor(sf::Color(r, g, b, originalColor.a));
      }

      target.draw(*sprite, states);
      sprite->setColor(originalColor);
    }

    sprite->setPosition(worldPos);
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
  GetPlayerSession().health = hp;
  personalMenu.SetHealth(hp);
  personalMenu.SetMaxHealth(hp);

  // If coming back from navi select, the navi has changed, update it
  const auto& owPath = meta.GetOverworldAnimationPath();

  if (owPath.size()) {
    if (auto tex = Textures().LoadTextureFromFile(meta.GetOverworldTexturePath())) {
      playerActor->setTexture(tex);
    }
    playerActor->LoadAnimations(owPath);

    // move the emote above the player's head
    float emoteY = -playerActor->getSprite().getOrigin().y - 10;
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
  if (value == "undernet") {
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
  else if (value == "lan") {
    SetBackground(std::make_shared<LanBackground>());
  }
  else {
    const auto& texture = GetTexture(map.GetBackgroundCustomTexturePath());
    const auto& animationData = GetText(map.GetBackgroundCustomAnimationPath());
    const auto& velocity = map.GetBackgroundCustomVelocity();

    Animation animation;
    animation.LoadWithData(animationData);

    SetBackground(std::make_shared<CustomBackground>(texture, animation, velocity));
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
  auto optionalMap = LoadTiledMap(*this, data);

  if (!optionalMap) {
    Logger::Log("Failed to load map");
    return;
  }

  auto map = std::move(optionalMap.value());

  bool backgroundDiffers = map.GetBackgroundName() != this->map.GetBackgroundName() ||
    map.GetBackgroundCustomTexturePath() != this->map.GetBackgroundCustomTexturePath() ||
    map.GetBackgroundCustomAnimationPath() != this->map.GetBackgroundCustomAnimationPath() ||
    map.GetBackgroundCustomVelocity() != this->map.GetBackgroundCustomVelocity();

  if (backgroundDiffers) {
    LoadBackground(map, map.GetBackgroundName());
  }

  if (map.GetSongPath() != this->map.GetSongPath()) {
    Audio().Stream(GetPath(map.GetSongPath()), true);
  }

  personalMenu.SetArea(map.GetName());

  // cleanup data from the previous map
  this->map.RemoveSprites(*this);

  // replace the previous map
  this->map = std::move(map);

  // scale to the game resolution
  this->map.setScale(2.f, 2.f);

  // update map to trigger recalculating shadows for minimap
  this->map.Update(*this, 0.0f);

  minimap = Minimap::CreateFrom(this->map.GetName(), this->map);
  minimap.setScale(2.f, 2.f);
}

void Overworld::SceneBase::TeleportUponReturn(const sf::Vector3f& position)
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
  return
    inputLocked || gotoNextScene || showMinimap ||
    !personalMenu.IsClosed() ||
    !menuSystem.IsClosed() ||
    !teleportController.IsComplete() || teleportController.TeleportedOut();
}

void Overworld::SceneBase::LockInput() {
  inputLocked = true;
}

void Overworld::SceneBase::UnlockInput() {
  inputLocked = false;
}

void Overworld::SceneBase::LockCamera() {
  cameraLocked = true;
}

void Overworld::SceneBase::UnlockCamera() {
  cameraLocked = false;
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
    getController().push<effect::to<MatchMakingScene>>(static_cast<int>(currentNavi), *folder, programAdvance);
  }
  else {
    Audio().Play(AudioType::CHIP_ERROR);
    Logger::Log("Cannot proceed to battles. You need 1 folder minimum.");
    gotoNextScene = false;
  }
}

void Overworld::SceneBase::GotoMail()
{
  gotoNextScene = true;
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<BlackWashFade, milliseconds<500>>;
  getController().push<effect::to<MailScene>>(playerSession.inbox);
}

void Overworld::SceneBase::GotoKeyItems()
{
  // Config Select on PC
  gotoNextScene = true;
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<BlackWashFade, milliseconds<500>>;

  getController().push<effect::to<KeyItemScene>>(items);
}

Overworld::PersonalMenu& Overworld::SceneBase::GetPersonalMenu()
{
  return personalMenu;
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

Overworld::PlayerSession& Overworld::SceneBase::GetPlayerSession()
{
  return playerSession;
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

Overworld::EmoteNode& Overworld::SceneBase::GetEmoteNode()
{
  return emoteNode;
}

Overworld::EmoteWidget& Overworld::SceneBase::GetEmoteWidget()
{
  return emote;
}

std::shared_ptr<Background> Overworld::SceneBase::GetBackground()
{
  return this->bg;
}

PA& Overworld::SceneBase::GetProgramAdvance() {
  return programAdvance;
}

std::optional<CardFolder*> Overworld::SceneBase::GetSelectedFolder() {
  CardFolder* folder;

  if (folders.GetFolder(0, folder)) {
    return folder;
  }
  else {
    return {};
  }
}

Overworld::MenuSystem& Overworld::SceneBase::GetMenuSystem()
{
  return menuSystem;
}

const std::shared_ptr<sf::Texture>& Overworld::SceneBase::GetCustomEmotesTexture() const {
  return customEmotesTexture;
}

void Overworld::SceneBase::SetCustomEmotesTexture(const std::shared_ptr<sf::Texture>& texture) {
  emoteNode.LoadCustomEmotes(texture);
}

void Overworld::SceneBase::OnEmoteSelected(Emotes emote)
{
  emoteNode.Emote(emote);
}

void Overworld::SceneBase::OnCustomEmoteSelected(unsigned emote)
{
  emoteNode.CustomEmote(emote);
}

void Overworld::SceneBase::AddItem(const std::string& name, const std::string& description)
{
  items.push_back({ name, description });
}

void Overworld::SceneBase::RemoveItem(const std::string& name)
{
  auto iter = std::find_if(items.begin(), items.end(), [&name](auto& item) { return item.name == name; });

  if (iter != items.end()) {
    items.erase(iter);
  }
}

bool Overworld::SceneBase::IsInFocus() const
{
  return this->gotoNextScene == false;
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

const bool Overworld::SceneBase::IsMouseHovering(const sf::Vector2f& mouse, const WorldSprite& src)
{
  auto textureRect = src.getSprite().getTextureRect();
  auto origin = src.getSprite().getOrigin();

  auto& map = GetMap();
  auto tileSize = map.GetTileSize();
  auto& scale = map.getScale();

  auto position = src.getPosition();
  auto screenPosition = map.WorldToScreen(position);
  screenPosition.y -= src.GetElevation() * tileSize.y / 2.0f;

  auto bounds = sf::FloatRect(
    (screenPosition.x - origin.x) * scale.x,
    (screenPosition.y - origin.y) * scale.y,
    std::abs(textureRect.width) * scale.x,
    std::abs(textureRect.height) * scale.y
  );

  return (mouse.x >= bounds.left && mouse.x <= bounds.left + bounds.width && mouse.y >= bounds.top && mouse.y <= bounds.top + bounds.height);
}

void Overworld::SceneBase::PrintTime(sf::RenderTarget& target)
{
  auto shadowColor = sf::Color(105, 105, 105);
  std::string timeStr = CurrentTime::AsFormattedString("%OI:%OM %p");
  time.SetString(timeStr);
  time.setOrigin(time.GetLocalBounds().width, 0.f);
  auto origin = time.GetLocalBounds().width;

  auto pos = time.getPosition();
  time.SetColor(shadowColor);
  time.setPosition(pos.x + 2.f, pos.y + 2.f);
  target.draw(time);

  time.SetString(timeStr.substr(0, 5));
  time.setPosition(pos);
  time.SetColor(sf::Color::White);
  target.draw(time);

  auto pColor = sf::Color::Red;
  time.SetString("AM");

  if (timeStr[6] != 'A') {
    pColor = sf::Color::Green;
    time.SetString("PM");
  }

  time.setOrigin(time.GetLocalBounds().width, 0.f);
  time.SetColor(pColor);
  target.draw(time);
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