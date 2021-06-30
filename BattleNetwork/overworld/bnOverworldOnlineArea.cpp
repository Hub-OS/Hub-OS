#include <Swoosh/Segue.h>
#include <Swoosh/ActivityController.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/BlackWashFade.h>
#include <Segues/WhiteWashFade.h>
#include <Poco/Net/NetException.h>

// TODO: mac os < 10.5 file system support...
#ifndef __APPLE__
#include <filesystem>
#endif 

#include <fstream>

#include "bnOverworldOnlineArea.h"
#include "bnOverworldTileType.h"
#include "bnOverworldObjectType.h"
#include "../bnXmasBackground.h"
#include "../bnNaviRegistration.h"
#include "../netplay/bnBufferWriter.h"
#include "../netplay/battlescene/bnNetworkBattleScene.h"
#include "../netplay/bnNetPlayConfig.h"
#include "../bnMessageQuestion.h"

using namespace swoosh::types;
constexpr float SECONDS_PER_MOVEMENT = 1.f / 10.f;
constexpr float DEFAULT_CONVEYOR_SPEED = 6.0f;

Overworld::OnlineArea::OnlineArea(
  swoosh::ActivityController& controller,
  const std::string& address,
  uint16_t port,
  const std::string& connectData,
  uint16_t maxPayloadSize,
  bool guestAccount) :
  SceneBase(controller, guestAccount),
  transitionText(Font::Style::small),
  nameText(Font::Style::small),
  connectData(connectData),
  maxPayloadSize(maxPayloadSize),
  serverAssetManager(address, port),
  identityManager(address, port)
{
  try {
    remoteAddress = Poco::Net::SocketAddress(address, port);
    packetProcessor = std::make_shared<Overworld::PacketProcessor>(
      remoteAddress,
      maxPayloadSize,
      [this](auto& body) { processPacketBody(body); }
    );

    Net().AddHandler(remoteAddress, packetProcessor);
  }
  catch (...) {
    // invalid remote address
    leave();
  }

  transitionText.setScale(2, 2);
  transitionText.SetString("Connecting...");

  lastFrameNavi = this->GetCurrentNavi();

  propertyAnimator.OnComplete([this] {
    // todo: this may cause issues when leaving the scene through Home and Server Warps
    GetPlayerController().ControlActor(GetPlayer());
  });
}

Overworld::OnlineArea::~OnlineArea()
{
}

void Overworld::OnlineArea::onUpdate(double elapsed)
{
  if (tryPopScene) {
    using effect = segue<PixelateBlackWashFade>;
    if (getController().pop<effect>()) {
      tryPopScene = false;
    }
  }

  if (!packetProcessor) {
    return;
  }

  if (packetProcessor->TimedOut()) {
    leave();
    return;
  }

  if (!isConnected || kicked) {
    return;
  }

  GetPersonalMenu().SetHealth(GetPlayerSession().health);

  // remove players before update, to prevent removed players from being added to sprite layers
  // players do not have a shared pointer to the emoteNode
  // a segfault would occur if this loop is placed after onUpdate due to emoteNode being deleted
  for (const auto& remove : removePlayers) {
    auto it = onlinePlayers.find(remove);

    if (it == onlinePlayers.end()) {
      Logger::Logf("Removed non existent Player %s", remove.c_str());
      continue;
    }

    auto& player = it->second;
    RemoveActor(player.actor);
    RemoveSprite(player.teleportController.GetBeam());

    onlinePlayers.erase(remove);
  }

  removePlayers.clear();

  auto currentNavi = GetCurrentNavi();
  if (lastFrameNavi != currentNavi) {
    sendAvatarChangeSignal();
    lastFrameNavi = currentNavi;
  }

  updateOtherPlayers(elapsed);
  updatePlayer(elapsed);

  movementTimer.update(sf::seconds(static_cast<float>(elapsed)));

  if (movementTimer.getElapsed().asSeconds() > SECONDS_PER_MOVEMENT) {
    movementTimer.reset();
    sendPositionSignal();
  }

  // handle camera locking and player tracking
  if (trackedPlayer && serverCameraController.IsQueueEmpty()) {
    auto it = onlinePlayers.find(*trackedPlayer);

    if (it == onlinePlayers.end()) {
      trackedPlayer = {};
    }
    else {
      auto position = it->second.actor->Get3DPosition();
      serverCameraController.MoveCamera(GetMap().WorldToScreen(position));
    }
  }

  if (serverCameraController.IsLocked() || trackedPlayer) {
    warpCameraController.UnlockCamera();
  }

  if (serverCameraController.IsLocked() || warpCameraController.IsLocked() || trackedPlayer) {
    LockCamera();
  }

  SceneBase::onUpdate(elapsed);

  auto& camera = GetCamera();
  warpCameraController.UpdateCamera(float(elapsed), camera);
  serverCameraController.UpdateCamera(float(elapsed), camera);
  UnlockCamera(); // reset lock, we'll lock it later if we need to
}

void Overworld::OnlineArea::updateOtherPlayers(double elapsed) {
  auto currentTime = CurrentTime::AsMilli();
  auto& map = GetMap();

  // update other players
  for (auto& pair : onlinePlayers) {
    auto& onlinePlayer = pair.second;
    auto& actor = onlinePlayer.actor;

    onlinePlayer.teleportController.Update(elapsed);
    onlinePlayer.emoteNode.Update(elapsed);

    onlinePlayer.propertyAnimator.Update(*actor, elapsed);

    if (onlinePlayer.propertyAnimator.IsAnimatingPosition()) {
      continue;
    }

    auto deltaTime = static_cast<double>(currentTime - onlinePlayer.timestamp) / 1000.0;
    auto delta = onlinePlayer.endBroadcastPos - onlinePlayer.startBroadcastPos;
    float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
    double expectedTime = Net().CalculateLag(onlinePlayer.packets, onlinePlayer.lagWindow, 0.0);
    float alpha = static_cast<float>(ease::linear(deltaTime, expectedTime, 1.0));

    auto newPos = onlinePlayer.startBroadcastPos + delta * alpha;
    actor->Set3DPosition(newPos);

    if (onlinePlayer.propertyAnimator.IsAnimating() && actor->IsPlayingCustomAnimation()) {
      // skip animating the player if they're being animated by the property animator
      continue;
    }

    auto tile = map.GetTileFromWorld(newPos);

    if (tile && map.GetTileMeta(tile->gid)->type == TileType::conveyor) {
      continue;
    }

    Direction newHeading = Actor::MakeDirectionFromVector({ delta.x, delta.y });
    auto oldHeading = actor->GetHeading();

    if (distance == 0.0) {
      actor->Face(onlinePlayer.idleDirection);
    }
    else if (distance <= actor->GetWalkSpeed() * expectedTime) {
      actor->Walk(newHeading, false); // Don't actually move or collide, but animate
    }
    else {
      actor->Run(newHeading, false);
    }
  }
}

void Overworld::OnlineArea::updatePlayer(double elapsed) {
  auto player = GetPlayer();
  auto playerPos = player->Get3DPosition();

  propertyAnimator.Update(*player, elapsed);

  if (!IsInputLocked()) {
    if (Input().Has(InputEvents::pressed_shoulder_right) && GetEmoteWidget().IsClosed()) {
      auto& meta = NAVIS.At(GetCurrentNavi());
      const std::string& image = meta.GetMugshotTexturePath();
      const std::string& anim = meta.GetMugshotAnimationPath();
      auto mugshot = Textures().LoadTextureFromFile(image);
      auto& menuSystem = GetMenuSystem();
      menuSystem.SetNextSpeaker(sf::Sprite(*mugshot), anim);

      menuSystem.EnqueueQuestion("Return to your homepage?", [this](bool result) {
        if (result) {
          GetTeleportController().TeleportOut(GetPlayer()).onFinish.Slot([this] {
            this->sendLogoutSignal();
            this->leave();
          });
        }
      });

      player->Face(Direction::down_right);
    }

    if (playerPos.x != lastPosition.x || playerPos.y != lastPosition.y) {
      // only need to handle this if the player has moved
      detectWarp(player);
    }
  }

  detectConveyor(player);

  lastPosition = playerPos;
}

void Overworld::OnlineArea::detectWarp(std::shared_ptr<Overworld::Actor>& player) {
  auto& teleportController = GetTeleportController();

  if (!teleportController.IsComplete()) {
    return;
  }

  auto layerIndex = player->GetLayer();

  auto& map = GetMap();

  if (layerIndex < 0 || layerIndex >= warps.size()) {
    return;
  }

  auto playerPos = player->getPosition();

  for (auto* tileObjectPtr : warps[layerIndex]) {
    auto& tileObject = *tileObjectPtr;

    if (!tileObject.Intersects(map, playerPos.x, playerPos.y)) {
      continue;
    }

    auto& command = teleportController.TeleportOut(player);

    auto interpolateTime = sf::seconds(0.5f);
    auto position3 = sf::Vector3f(tileObject.position.x, tileObject.position.y, float(layerIndex));

    switch (tileObject.type) {
    case ObjectType::home_warp: {
      warpCameraController.QueueMoveCamera(map.WorldToScreen(position3), interpolateTime);

      command.onFinish.Slot([=] {
        GetPlayerController().ReleaseActor();
        sendLogoutSignal();
        getController().pop<segue<BlackWashFade>>();
      });
      break;
    }
    case ObjectType::server_warp: {
      warpCameraController.QueueMoveCamera(map.WorldToScreen(position3), interpolateTime);

      auto address = tileObject.customProperties.GetProperty("address");
      auto port = (uint16_t)tileObject.customProperties.GetPropertyInt("port");
      auto data = tileObject.customProperties.GetProperty("data");

      command.onFinish.Slot([=] {
        GetPlayerController().ReleaseActor();
        transferServer(address, port, data, false);
      });
      break;
    }
    case ObjectType::position_warp: {
      auto targetTilePos = sf::Vector2f(
        tileObject.customProperties.GetPropertyFloat("x"),
        tileObject.customProperties.GetPropertyFloat("y")
      );

      auto targetWorldPos = map.TileToWorld(targetTilePos);

      auto targetPosition = sf::Vector3f(targetWorldPos.x, targetWorldPos.y, tileObject.customProperties.GetPropertyFloat("z"));
      auto direction = FromString(tileObject.customProperties.GetProperty("direction"));

      sf::Vector2f player_pos = { player->getPosition().x, player->getPosition().y };
      float distance = std::pow(targetWorldPos.x - player_pos.x, 2.0f) + std::pow(targetWorldPos.y - player_pos.y, 2.0f);

      // this is a magic number - this is about as close to 2 warps that are 8 blocks away vertically 
      // (expression is also squared)
      if (distance < 40'000) {
        warpCameraController.QueueWaneCamera(map.WorldToScreen(targetPosition), interpolateTime, 0.55f);
      }

      command.onFinish.Slot([=] {
        teleportIn(targetPosition, Orthographic(direction));
        warpCameraController.QueueUnlockCamera();
      });
      break;
    }
    default:
      warpCameraController.QueueMoveCamera(map.WorldToScreen(position3), interpolateTime);

      command.onFinish.Slot([=] {
        sendCustomWarpSignal(tileObject.id);
      });
    }

    break;
  }
}

void Overworld::OnlineArea::detectConveyor(std::shared_ptr<Overworld::Actor>& player) {
  if (propertyAnimator.IsAnimatingPosition()) {
    // if the server is dragging the player around, ignore conveyors
    return;
  }

  auto& map = GetMap();

  auto layerIndex = player->GetLayer();

  if (layerIndex < 0 || layerIndex >= map.GetLayerCount()) {
    return;
  }

  auto playerTilePos = map.WorldToTileSpace(player->getPosition());
  auto& layer = map.GetLayer(layerIndex);
  auto tile = layer.GetTile(int(playerTilePos.x), int(playerTilePos.y));

  if (!tile) {
    return;
  }

  auto tileMeta = map.GetTileMeta(tile->gid);

  if (!tileMeta || tileMeta->type != TileType::conveyor) {
    return;
  }

  auto direction = tileMeta->direction;

  if (tile->flippedHorizontal) {
    direction = FlipHorizontal(direction);
  }

  if (tile->flippedVertical) {
    direction = FlipVertical(direction);
  }

  auto endTilePos = playerTilePos;
  float tileDistance = 0.0f;

  ActorPropertyAnimator::PropertyStep animationProperty;
  animationProperty.property = ActorProperty::animation;

  // resolve animation
  switch (direction) {
  case Direction::up_left:
    animationProperty.stringValue = "IDLE_UL";
    break;
  case Direction::up_right:
    animationProperty.stringValue = "IDLE_UR";
    break;
  case Direction::down_left:
    animationProperty.stringValue = "IDLE_DL";
    break;
  case Direction::down_right:
    animationProperty.stringValue = "IDLE_DR";
    break;
  }

  ActorPropertyAnimator::PropertyStep axisProperty;
  axisProperty.ease = Ease::linear;

  auto isConveyor = [&layer, &map](sf::Vector2f endTilePos) {
    auto tile = layer.GetTile(int(endTilePos.x), int(endTilePos.y));

    if (!tile) {
      return false;
    }

    auto tileMeta = map.GetTileMeta(tile->gid);

    return tileMeta && tileMeta->type == TileType::conveyor;
  };

  // resolve end position
  switch (direction) {
  case Direction::up_left:
  case Direction::down_right:
    endTilePos.x = std::floor(endTilePos.x);
    axisProperty.property = ActorProperty::x;
    break;
  case Direction::up_right:
  case Direction::down_left:
    axisProperty.property = ActorProperty::y;
    endTilePos.y = std::floor(endTilePos.y);
    break;
  }

  auto unprojectedDirection = Orthographic(direction);
  auto walkVector = UnitVector(unprojectedDirection);

  endTilePos += walkVector;

  bool nextTileIsConveyor = isConveyor(endTilePos);
  if (nextTileIsConveyor) {
    endTilePos += walkVector / 2.0f;
  }

  // fixing overshooting
  switch (direction) {
  case Direction::up_left:
    endTilePos.x = std::nextafter(endTilePos.x + 1.0f, -INFINITY);
    break;
  case Direction::up_right:
    endTilePos.y = std::nextafter(endTilePos.y + 1.0f, -INFINITY);
    break;
  }

  auto endPos = map.TileToWorld(endTilePos);

  switch (axisProperty.property) {
  case ActorProperty::x:
    axisProperty.value = endPos.x;
    tileDistance = std::abs(endTilePos.x - playerTilePos.x);
    break;
  case ActorProperty::y:
    axisProperty.value = endPos.y;
    tileDistance = std::abs(endTilePos.y - playerTilePos.y);
    break;
  }

  propertyAnimator.Reset();

  auto speed = tileMeta->customProperties.GetPropertyFloat("speed");

  if (speed == 0.0f) {
    speed = DEFAULT_CONVEYOR_SPEED;
  }

  auto duration = tileDistance / speed;

  ActorPropertyAnimator::PropertyStep sfxProperty;
  sfxProperty.property = ActorProperty::sound_effect_loop;
  sfxProperty.stringValue = GetPath(tileMeta->customProperties.GetProperty("sound effect"));

  ActorPropertyAnimator::KeyFrame startKeyframe;
  startKeyframe.propertySteps.push_back(animationProperty);
  startKeyframe.propertySteps.push_back(sfxProperty);
  propertyAnimator.AddKeyFrame(startKeyframe);

  ActorPropertyAnimator::KeyFrame endKeyframe;
  endKeyframe.propertySteps.push_back(axisProperty);
  endKeyframe.duration = duration;
  propertyAnimator.AddKeyFrame(endKeyframe);

  if (!nextTileIsConveyor) {
    ActorPropertyAnimator::KeyFrame waitKeyFrame;
    // reuse last property to simulate idle
    waitKeyFrame.propertySteps.push_back(axisProperty);
    waitKeyFrame.duration = 0.25;
    propertyAnimator.AddKeyFrame(waitKeyFrame);
  }

  propertyAnimator.UseKeyFrames(*player);
  GetPlayerController().ReleaseActor();
}

void Overworld::OnlineArea::onDraw(sf::RenderTexture& surface)
{
  if (!isConnected) {
    auto view = getController().getVirtualWindowSize();
    int precision = 1;

    transitionText.setPosition(view.x * 0.5f, view.y * 0.5f);
    transitionText.setOrigin(transitionText.GetLocalBounds().width * 0.5f, transitionText.GetLocalBounds().height * 0.5f);
    surface.draw(transitionText);
    return;
  }

  SceneBase::onDraw(surface);

  if (GetMenuSystem().IsFullscreen()) {
    return;
  }

  auto& window = getController().getWindow();
  auto mousei = sf::Mouse::getPosition(window);
  auto mousef = window.mapPixelToCoords(mousei);

  auto cameraView = GetCamera().GetView();
  sf::Vector2f cameraCenter = cameraView.getCenter();
  sf::Vector2f mapScale = GetMap().getScale();
  cameraCenter.x = std::floor(cameraCenter.x) * mapScale.x;
  cameraCenter.y = std::floor(cameraCenter.y) * mapScale.y;
  auto offset = cameraCenter - getView().getCenter();

  auto mouseScreen = sf::Vector2f(mousef.x + offset.x, mousef.y + offset.y);

  sf::RectangleShape rect({ 2.f, 2.f });
  rect.setFillColor(sf::Color::Red);
  rect.setPosition(mousef);
  surface.draw(rect);

  auto& map = GetMap();

  auto topLayer = -1;
  auto topY = std::numeric_limits<float>::min();
  std::string topName;

  auto testActor = [&](Actor& actor) {
    auto& name = actor.GetName();
    auto layer = (int)std::ceil(actor.GetElevation());
    auto screenY = map.WorldToScreen(actor.Get3DPosition()).y;

    if (name == "" || layer < topLayer || screenY <= topY) {
      return;
    }

    if (IsMouseHovering(mouseScreen, actor)) {
      topLayer = layer;
      topName = name;
      topY = screenY;
    }
  };

  for (auto& pair : onlinePlayers) {
    auto& onlinePlayer = pair.second;
    auto& actor = *onlinePlayer.actor;

    testActor(actor);
  }

  testActor(*GetPlayer());

  nameText.setPosition(mousef);
  nameText.SetString(topName);
  nameText.setOrigin(-10.0f, 0);
  surface.draw(nameText);
}

void Overworld::OnlineArea::onStart()
{
  SceneBase::onStart();
  movementTimer.start();

  sendLoginSignal();
  sendAssetsFound();
  sendAvatarChangeSignal();
  sendRequestJoinSignal();
}

void Overworld::OnlineArea::onEnd()
{
  if (packetProcessor) {
    sendLogoutSignal();
    Net().DropProcessor(packetProcessor);
  }
}

void Overworld::OnlineArea::onLeave()
{
  SceneBase::onLeave();

  if (packetProcessor) {
    packetProcessor->SetBackground();
  }
}

void Overworld::OnlineArea::onResume()
{
  SceneBase::onResume();

  if (packetProcessor) {
    packetProcessor->SetForeground();
  }

  if (netBattleProcessor) {
    Net().DropProcessor(netBattleProcessor);
    netBattleProcessor = nullptr;
  }

  // isEnteringBattle = false;
}

void Overworld::OnlineArea::OnTileCollision() { }

void Overworld::OnlineArea::OnInteract() {
  auto& map = GetMap();
  auto playerActor = GetPlayer();

  // check to see what tile we pressed talk to
  auto layerIndex = playerActor->GetLayer();
  auto& layer = map.GetLayer(layerIndex);
  auto tileSize = map.GetTileSize();

  auto frontPosition = playerActor->PositionInFrontOf();

  for (auto& tileObject : layer.GetTileObjects()) {
    auto interactable = tileObject.visible || tileObject.solid;

    if (interactable && tileObject.Intersects(map, frontPosition.x, frontPosition.y)) {
      sendObjectInteractionSignal(tileObject.id);

      // block other interactions with return
      return;
    }
  }

  auto positionInFrontOffset = frontPosition - playerActor->getPosition();
  auto elevation = playerActor->GetElevation();

  for (const auto& other : GetSpatialMap().GetChunk(frontPosition.x, frontPosition.y)) {
    if (playerActor == other) continue;

    auto elevationDifference = std::fabs(other->GetElevation() - elevation);

    if (elevationDifference >= 1.0f) continue;

    auto collision = playerActor->CollidesWith(*other, positionInFrontOffset);

    if (collision) {
      other->Interact(playerActor);

      // block other interactions with return
      return;
    }
  }

  sendTileInteractionSignal(
    frontPosition.x / (float)(tileSize.x / 2),
    frontPosition.y / tileSize.y,
    playerActor->GetElevation()
  );
}

void Overworld::OnlineArea::OnEmoteSelected(Overworld::Emotes emote)
{
  SceneBase::OnEmoteSelected(emote);
  sendEmoteSignal(emote);
}

bool Overworld::OnlineArea::positionIsInWarp(sf::Vector3f position) {
  auto& map = GetMap();
  auto layerCount = map.GetLayerCount();

  if (position.z < 0 || position.z >= layerCount) return false;

  auto& warpLayer = warps[(size_t)position.z];

  for (auto object : warpLayer) {
    if (object->Intersects(map, position.x, position.y)) {
      return true;
    }
  }

  return false;
}

Overworld::TeleportController::Command& Overworld::OnlineArea::teleportIn(sf::Vector3f position, Direction direction)
{
  auto actor = GetPlayer();

  if (!positionIsInWarp(position)) {
    actor->Face(direction);
    direction = Direction::none;
  }

  return GetTeleportController().TeleportIn(actor, position, direction);
}

void Overworld::OnlineArea::transferServer(const std::string& address, uint16_t port, const std::string& data, bool warpOut) {
  auto transfer = [=] {
    getController().replace<segue<BlackWashFade>::to<Overworld::OnlineArea>>(address, port, data, maxPayloadSize, !WEBCLIENT.IsLoggedIn());
  };

  if (warpOut) {
    GetPlayerController().ReleaseActor();
    auto& command = GetTeleportController().TeleportOut(GetPlayer());
    command.onFinish.Slot(transfer);
  }
  else {
    transfer();
  }

  transferringServers = true;
}

void Overworld::OnlineArea::processPacketBody(const Poco::Buffer<char>& data)
{
  BufferReader reader;

  try {
    auto sig = reader.Read<ServerEvents>(data);

    switch (sig) {
    case ServerEvents::login:
      receiveLoginSignal(reader, data);
      break;
    case ServerEvents::connection_complete:
      isConnected = true;
      sendReadySignal();
      break;
    case ServerEvents::transfer_warp:
      receiveTransferWarpSignal(reader, data);
      break;
    case ServerEvents::transfer_start:
      receiveTransferStartSignal(reader, data);
      break;
    case ServerEvents::transfer_complete:
      receiveTransferCompleteSignal(reader, data);
      break;
    case ServerEvents::transfer_server:
      receiveTransferServerSignal(reader, data);
      break;
    case ServerEvents::kick:
      if (!transferringServers) {
        // ignore kick signal if we're leaving anyway
        receiveKickSignal(reader, data);
      }
      break;
    case ServerEvents::remove_asset:
      receiveAssetRemoveSignal(reader, data);
      break;
    case ServerEvents::asset_stream_start:
      receiveAssetStreamStartSignal(reader, data);
      break;
    case ServerEvents::asset_stream:
      receiveAssetStreamSignal(reader, data);
      break;
    case ServerEvents::preload:
      receivePreloadSignal(reader, data);
      break;
    case ServerEvents::custom_emotes_path:
      receiveCustomEmotesPathSignal(reader, data);
      break;
    case ServerEvents::map:
      receiveMapSignal(reader, data);
      break;
    case ServerEvents::health:
      receiveHealthSignal(reader, data);
      break;
    case ServerEvents::emotion:
      receiveEmotionSignal(reader, data);
      break;
    case ServerEvents::money:
      receiveMoneySignal(reader, data);
      break;
    case ServerEvents::add_item:
      receiveItemSignal(reader, data);
      break;
    case ServerEvents::remove_item:
      receiveRemoveItemSignal(reader, data);
      break;
    case ServerEvents::play_sound:
      receivePlaySoundSignal(reader, data);
      break;
    case ServerEvents::exclude_object:
      receiveExcludeObjectSignal(reader, data);
      break;
    case ServerEvents::include_object:
      receiveIncludeObjectSignal(reader, data);
      break;
    case ServerEvents::move_camera:
      receiveMoveCameraSignal(reader, data);
      break;
    case ServerEvents::slide_camera:
      receiveSlideCameraSignal(reader, data);
      break;
    case ServerEvents::shake_camera:
      receiveShakeCameraSignal(reader, data);
      break;
    case ServerEvents::track_with_camera:
      receiveTrackWithCameraSignal(reader, data);
      break;
    case ServerEvents::unlock_camera:
      serverCameraController.QueueUnlockCamera();
      break;
    case ServerEvents::lock_input:
      LockInput();
      break;
    case ServerEvents::unlock_input:
      UnlockInput();
      break;
    case ServerEvents::teleport:
      receiveTeleportSignal(reader, data);
      break;
    case ServerEvents::message:
      receiveMessageSignal(reader, data);
      break;
    case ServerEvents::question:
      receiveQuestionSignal(reader, data);
      break;
    case ServerEvents::quiz:
      receiveQuizSignal(reader, data);
      break;
    case ServerEvents::prompt:
      receivePromptSignal(reader, data);
      break;
    case ServerEvents::open_board:
      receiveOpenBoardSignal(reader, data);
      break;
    case ServerEvents::prepend_posts:
      receivePrependPostsSignal(reader, data);
      break;
    case ServerEvents::append_posts:
      receiveAppendPostsSignal(reader, data);
      break;
    case ServerEvents::remove_post:
      receiveRemovePostSignal(reader, data);
      break;
    case ServerEvents::post_selection_ack:
      GetMenuSystem().AcknowledgeBBSSelection();
      break;
    case ServerEvents::close_bbs:
      receiveCloseBBSSignal(reader, data);
      break;
    case ServerEvents::initiate_pvp:
      receivePVPSignal(reader, data);
      break;
    case ServerEvents::actor_connected:
      receiveActorConnectedSignal(reader, data);
      break;
    case ServerEvents::actor_disconnect:
      receiveActorDisconnectedSignal(reader, data);
      break;
    case ServerEvents::actor_set_name:
      receiveActorSetNameSignal(reader, data);
      break;
    case ServerEvents::actor_move_to:
      receiveActorMoveSignal(reader, data);
      break;
    case ServerEvents::actor_set_avatar:
      receiveActorSetAvatarSignal(reader, data);
      break;
    case ServerEvents::actor_emote:
      receiveActorEmoteSignal(reader, data);
      break;
    case ServerEvents::actor_animate:
      receiveActorAnimateSignal(reader, data);
      break;
    case ServerEvents::actor_keyframes:
      receiveActorKeyFramesSignal(reader, data);
      break;
    }
  }
  catch (Poco::IOException& e) {
    Logger::Logf("OnlineArea Network exception: %s", e.displayText().c_str());

    leave();
  }
}
void Overworld::OnlineArea::sendAssetFoundSignal(const std::string& path, uint64_t lastModified) {
  ClientEvents event{ ClientEvents::asset_found };

  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::asset_found);
  writer.WriteString<uint16_t>(buffer, path);
  writer.Write(buffer, lastModified);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendAssetsFound() {
  for (auto& [name, meta] : serverAssetManager.GetCachedAssetList()) {
    sendAssetFoundSignal(name, meta.lastModified);
  }
}

void Overworld::OnlineArea::sendAssetStreamSignal(ClientAssetType assetType, uint16_t headerSize, const char* data, size_t size) {
  size_t remainingBytes = size;

  // - 1 for asset type, - 2 for size
  const uint16_t availableRoom = maxPayloadSize - headerSize - 3;

  while (remainingBytes > 0) {
    uint16_t size = remainingBytes < availableRoom ? (uint16_t)remainingBytes : availableRoom;
    remainingBytes -= size;

    BufferWriter writer;
    Poco::Buffer<char> buffer{ 0 };
    writer.Write(buffer, ClientEvents::asset_stream);
    writer.Write(buffer, assetType);
    writer.Write(buffer, size);
    writer.WriteBytes(buffer, data, size);
    packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);

    data += size;
  }
}

void Overworld::OnlineArea::sendLoginSignal()
{
  std::string username = WEBCLIENT.GetUserName();

  if (username.empty()) {
    username = "Anon";
  }

  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::login);
  writer.WriteString<uint8_t>(buffer, username);
  writer.WriteString<uint8_t>(buffer, identityManager.GetIdentity());
  writer.WriteString<uint16_t>(buffer, connectData);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendLogoutSignal()
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::logout);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendRequestJoinSignal()
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::request_join);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendReadySignal()
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::ready);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendTransferredOutSignal()
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::transferred_out);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendCustomWarpSignal(unsigned int tileObjectId)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::custom_warp);
  writer.Write(buffer, tileObjectId);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendPositionSignal()
{
  uint64_t creationTime = CurrentTime::AsMilli();

  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  auto player = GetPlayer();
  sf::Vector2f vec = player->getPosition();
  float x = vec.x / tileSize.x * 2.0f;
  float y = vec.y / tileSize.y;
  float z = player->GetElevation();
  auto direction = Isometric(player->GetHeading());

  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::position);
  writer.Write(buffer, creationTime);
  writer.Write(buffer, x);
  writer.Write(buffer, y);
  writer.Write(buffer, z);
  writer.Write(buffer, direction);
  packetProcessor->SendKeepAlivePacket(Reliability::UnreliableSequenced, buffer);
}

void Overworld::OnlineArea::sendAvatarChangeSignal()
{
  sendAvatarAssetStream();

  auto& naviMeta = NAVIS.At(GetCurrentNavi());
  auto naviName = naviMeta.GetName();
  auto maxHP = naviMeta.GetHP();

  // mark completion
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::avatar_change);
  writer.WriteString<uint16_t>(buffer, naviName);
  writer.Write(buffer, maxHP);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

static std::vector<char> readBytes(std::string texturePath) {
  size_t textureLength;
  std::vector<char> textureData;

#ifndef __APPLE__
  try {
    textureLength = std::filesystem::file_size(texturePath);
  }
  catch (std::filesystem::filesystem_error& e) {
    Logger::Logf("Failed to read texture \"%s\": %s", texturePath.c_str(), e.what());
    return textureData;
  }

  try {
    std::ifstream fin(texturePath, std::ios::binary);

    // prevents newlines from being skipped
    fin.unsetf(std::ios::skipws);

    textureData.reserve(textureLength);
    textureData.insert(textureData.begin(), std::istream_iterator<char>(fin), std::istream_iterator<char>());
  }
  catch (std::ifstream::failure& e) {
    Logger::Logf("Failed to read texture \"%s\": %s", texturePath.c_str(), e.what());
  }
#endif

  return textureData;
}

void Overworld::OnlineArea::sendAvatarAssetStream() {
  // + reliability type + id + packet type
  auto packetHeaderSize = 1 + 8 + 2;

  auto& naviMeta = NAVIS.At(GetCurrentNavi());

  auto texturePath = naviMeta.GetOverworldTexturePath();
  auto textureData = readBytes(texturePath);
  sendAssetStreamSignal(ClientAssetType::texture, packetHeaderSize, textureData.data(), textureData.size());

  const auto& animationPath = naviMeta.GetOverworldAnimationPath();
  std::string animationData = FileUtil::Read(animationPath);
  sendAssetStreamSignal(ClientAssetType::animation, packetHeaderSize, animationData.c_str(), animationData.length());

  auto mugshotTexturePath = naviMeta.GetMugshotTexturePath();
  auto mugshotTextureData = readBytes(mugshotTexturePath);
  sendAssetStreamSignal(ClientAssetType::mugshot_texture, packetHeaderSize, mugshotTextureData.data(), mugshotTextureData.size());

  const auto& mugshotAnimationPath = naviMeta.GetMugshotAnimationPath();
  std::string mugshotAnimationData = FileUtil::Read(mugshotAnimationPath);
  sendAssetStreamSignal(ClientAssetType::mugshot_animation, packetHeaderSize, mugshotAnimationData.c_str(), mugshotAnimationData.length());
}

void Overworld::OnlineArea::sendEmoteSignal(const Overworld::Emotes emote)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::emote);
  writer.Write(buffer, emote);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendObjectInteractionSignal(unsigned int tileObjectId)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::object_interaction);
  writer.Write(buffer, tileObjectId);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendNaviInteractionSignal(const std::string& ticket)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::actor_interaction);
  writer.WriteString<uint16_t>(buffer, ticket);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendTileInteractionSignal(float x, float y, float z)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::tile_interaction);
  writer.Write(buffer, x);
  writer.Write(buffer, y);
  writer.Write(buffer, z);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendTextBoxResponseSignal(char response)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::textbox_response);
  writer.Write(buffer, response);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendPromptResponseSignal(const std::string& response)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::prompt_response);
  writer.WriteString<uint16_t>(buffer, response);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendBoardOpenSignal()
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::board_open);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendBoardCloseSignal()
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::board_close);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendPostRequestSignal()
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::post_request);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendPostSelectSignal(const std::string& postId)
{
  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::post_selection);
  writer.WriteString<uint16_t>(buffer, postId);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendBattleResultsSignal(const BattleResults& battleResults) {
  auto time = battleResults.battleLength.asSeconds();

  BufferWriter writer;
  Poco::Buffer<char> buffer{ 0 };
  writer.Write(buffer, ClientEvents::battle_results);
  writer.Write(buffer, (int)battleResults.playerHealth);
  writer.Write(buffer, (int)battleResults.score);
  writer.Write(buffer, time);
  writer.Write(buffer, battleResults.runaway);
  writer.Write(buffer, battleResults.finalEmotion);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::receiveLoginSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = map.GetTileSize();

  this->ticket = reader.ReadString<uint16_t>(buffer);
  auto warpIn = reader.Read<bool>(buffer);
  auto x = reader.Read<float>(buffer) * tileSize.x / 2.0f;
  auto y = reader.Read<float>(buffer) * tileSize.y;
  auto z = reader.Read<float>(buffer);
  auto direction = reader.Read<Direction>(buffer);

  auto spawnPos = sf::Vector3f(x, y, z);

  auto player = GetPlayer();

  if (warpIn) {
    auto& command = teleportIn(spawnPos, Orthographic(direction));
    command.onFinish.Slot([=] {
      GetPlayerController().ControlActor(player);
    });
  }
  else {
    player->Set3DPosition(spawnPos);
    player->Face(Orthographic(direction));
    GetPlayerController().ControlActor(player);
  }
}

void Overworld::OnlineArea::receiveTransferWarpSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& command = GetTeleportController().TeleportOut(GetPlayer());

  command.onFinish.Slot([this] {
    sendTransferredOutSignal();
  });
}

void Overworld::OnlineArea::receiveTransferStartSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  isConnected = false;
  transitionText.SetString("");
  excludedObjects.clear();
  removePlayers.clear();

  for (auto& [key, _] : onlinePlayers) {
    removePlayers.push_back(key);
  }
}

void Overworld::OnlineArea::receiveTransferCompleteSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  bool warpIn = reader.Read<bool>(buffer);
  auto direction = reader.Read<Direction>(buffer);
  auto worldDirection = Orthographic(direction);

  auto player = GetPlayer();
  player->Face(worldDirection);

  if (warpIn) {
    teleportIn(player->Get3DPosition(), worldDirection);
  }

  isConnected = true;
  sendReadySignal();
}

void Overworld::OnlineArea::receiveTransferServerSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto address = reader.ReadString<uint16_t>(buffer);
  auto port = reader.Read<uint16_t>(buffer);
  auto data = reader.ReadString<uint16_t>(buffer);
  auto warpOut = reader.Read<bool>(buffer);

  transferServer(address, port, data, warpOut);
}

void Overworld::OnlineArea::receiveKickSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string kickReason = reader.ReadString<uint16_t>(buffer);
  std::string kickText = "kicked for";

  // insert padding to center the text
  auto lengthDifference = (int)kickReason.length() - (int)kickText.length();

  if (lengthDifference > 0) {
    kickText.insert(kickText.begin(), lengthDifference / 2, ' ');
  }
  else {
    kickReason.insert(kickReason.begin(), -lengthDifference / 2, ' ');
  }

  transitionText.SetString(kickText + "\n\n" + kickReason);
  isConnected = false;
  kicked = true;

  // bool kicked will block incoming packets, so we'll leave in update from a timeout
}

void Overworld::OnlineArea::receiveAssetRemoveSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto path = reader.ReadString<uint16_t>(buffer);

  serverAssetManager.RemoveAsset(path);
}

void Overworld::OnlineArea::receiveAssetStreamStartSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto name = reader.ReadString<uint16_t>(buffer);
  auto lastModified = reader.Read<uint64_t>(buffer);
  auto cachable = reader.Read<bool>(buffer);
  auto type = reader.Read<AssetType>(buffer);
  auto size = reader.Read<size_t>(buffer);

  auto slashIndex = name.rfind("/");
  std::string shortName;

  if (slashIndex != std::string::npos) {
    shortName = name.substr(slashIndex + 1);
  }
  else {
    shortName = name;
  }

  if (shortName.length() > 20) {
    shortName = shortName.substr(0, 17) + "...";
  }

  incomingAsset = {
    name,
    shortName,
    lastModified,
    cachable,
    type,
    size,
  };

  transitionText.SetString("Downloading " + shortName + ": 0%");
}

void Overworld::OnlineArea::receiveAssetStreamSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto size = reader.Read<uint16_t>(buffer);

  incomingAsset.buffer.append(buffer.begin() + reader.GetOffset() + 2, size);

  auto progress = (float)incomingAsset.buffer.size() / (float)incomingAsset.size * 100;

  std::stringstream transitionTextStream;
  transitionTextStream << "Downloading " << incomingAsset.shortName << ": ";
  transitionTextStream << std::floor(progress);
  transitionTextStream << '%';
  transitionText.SetString(transitionTextStream.str());

  if (incomingAsset.buffer.size() < incomingAsset.size) return;

  const std::string& name = incomingAsset.name;
  auto lastModified = incomingAsset.lastModified;
  auto cachable = incomingAsset.cachable;

  BufferReader assetReader;

  switch (incomingAsset.type) {
  case AssetType::text:
    serverAssetManager.SetText(name, lastModified, assetReader.ReadString(incomingAsset.buffer, incomingAsset.buffer.size()), cachable);
    break;
  case AssetType::texture:
    serverAssetManager.SetTexture(name, lastModified, incomingAsset.buffer.begin(), incomingAsset.size, cachable);
    break;
  case AssetType::audio:
    serverAssetManager.SetAudio(name, lastModified, incomingAsset.buffer.begin(), incomingAsset.size, cachable);
    break;
  }

  incomingAsset.buffer.setCapacity(0);
}

void Overworld::OnlineArea::receivePreloadSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto name = reader.ReadString<uint16_t>(buffer);

  serverAssetManager.Preload(name);
}

void Overworld::OnlineArea::receiveCustomEmotesPathSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto path = reader.ReadString<uint16_t>(buffer);

  SetCustomEmotesTexture(serverAssetManager.GetTexture(path));
}

void Overworld::OnlineArea::receiveMapSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto path = reader.ReadString<uint16_t>(buffer);
  auto mapBuffer = GetText(path);

  LoadMap(mapBuffer);

  auto& map = GetMap();
  auto layerCount = map.GetLayerCount();

  for (auto& [objectId, excludedData] : excludedObjects) {
    for (auto i = 0; i < layerCount; i++) {
      auto& layer = map.GetLayer(i);

      auto optional_object_ref = layer.GetTileObject(objectId);

      if (optional_object_ref) {
        auto object_ref = *optional_object_ref;
        auto& object = object_ref.get();

        excludedData.visible = object.visible;
        excludedData.solid = object.solid;

        object.visible = false;
        object.solid = false;
        break;
      }
    }
  }

  // storing special tile objects in minimap and other variables
  auto& minimap = GetMinimap();
  warps.resize(layerCount);

  auto tileSize = map.GetTileSize();

  for (auto i = 0; i < layerCount; i++) {
    auto& warpLayer = warps[i];
    warpLayer.clear();

    for (auto& tileObject : map.GetLayer(i).GetTileObjects()) {
      if (ObjectType::IsWarp(tileObject.type)) {
        tileObject.solid = false;
        warpLayer.push_back(&tileObject);
      }
    }
  }
}

void Overworld::OnlineArea::receiveHealthSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto health = reader.Read<int>(buffer);
  auto maxHealth = reader.Read<int>(buffer);

  GetPlayerSession().health = health;
  GetPersonalMenu().SetHealth(health);
  GetPersonalMenu().SetMaxHealth(maxHealth);
}

void Overworld::OnlineArea::receiveEmotionSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto emotion = reader.Read<Emotion>(buffer);

  if (emotion >= Emotion::COUNT) {
    emotion = Emotion::normal;
  }

  GetPlayerSession().emotion = emotion;
}

void Overworld::OnlineArea::receiveMoneySignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto balance = reader.Read<int>(buffer);
  GetPersonalMenu().SetMonies(balance);
}

void Overworld::OnlineArea::receiveItemSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto name = reader.ReadString<uint8_t>(buffer);
  auto description = reader.ReadString<uint16_t>(buffer);

  AddItem(name, description);
}

void Overworld::OnlineArea::receiveRemoveItemSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto name = reader.ReadString<uint8_t>(buffer);

  RemoveItem(name);
}

void Overworld::OnlineArea::receivePlaySoundSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto name = reader.ReadString<uint16_t>(buffer);

  Audio().Play(GetAudio(name), AudioPriority::highest);
}

void Overworld::OnlineArea::receiveExcludeObjectSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto objectId = reader.Read<uint32_t>(buffer);

  if (excludedObjects.find(objectId) != excludedObjects.end()) {
    return;
  }

  auto& map = GetMap();

  for (auto i = 0; i < map.GetLayerCount(); i++) {
    auto& layer = map.GetLayer(i);

    auto optional_object_ref = layer.GetTileObject(objectId);

    if (optional_object_ref) {
      auto object_ref = *optional_object_ref;
      auto& object = object_ref.get();

      ExcludedObjectData excludedData{};
      excludedData.visible = object.visible;
      excludedData.solid = object.solid;

      excludedObjects.emplace(objectId, excludedData);

      object.visible = false;
      object.solid = false;
      break;
    }
  }
}

void Overworld::OnlineArea::receiveIncludeObjectSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto objectId = reader.Read<uint32_t>(buffer);
  auto& map = GetMap();

  if (excludedObjects.erase(objectId) == 0) {
    return;
  }

  for (auto i = 0; i < map.GetLayerCount(); i++) {
    auto& layer = map.GetLayer(i);

    auto optional_object_ref = layer.GetTileObject(objectId);

    if (optional_object_ref) {
      auto object_ref = *optional_object_ref;
      auto& object = object_ref.get();

      object.visible = true;
      object.solid = true;
      break;
    }
  }
}

void Overworld::OnlineArea::receiveMoveCameraSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = map.GetTileSize();

  auto x = reader.Read<float>(buffer) * tileSize.x / 2.0f;
  auto y = reader.Read<float>(buffer) * tileSize.y;
  auto z = reader.Read<float>(buffer);

  auto position = sf::Vector2f(x, y);

  auto screenPos = map.WorldToScreen(position);
  screenPos.y -= z * tileSize.y / 2.0f;

  auto duration = reader.Read<float>(buffer);

  serverCameraController.QueuePlaceCamera(screenPos, sf::seconds(duration));
}

void Overworld::OnlineArea::receiveSlideCameraSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = map.GetTileSize();

  auto x = reader.Read<float>(buffer) * tileSize.x / 2.0f;
  auto y = reader.Read<float>(buffer) * tileSize.y;
  auto z = reader.Read<float>(buffer);

  auto position = sf::Vector2f(x, y);

  auto screenPos = map.WorldToScreen(position);
  screenPos.y -= z * tileSize.y / 2.0f;

  auto duration = reader.Read<float>(buffer);

  serverCameraController.QueueMoveCamera(screenPos, sf::seconds(duration));
}

void Overworld::OnlineArea::receiveShakeCameraSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto strength = reader.Read<float>(buffer);
  auto duration = sf::seconds(reader.Read<float>(buffer));

  if (serverCameraController.IsQueueEmpty()) {
    // adding shake to the queue can break tracking players
    // no need to add it to the queue if it's empty, just apply directly
    GetCamera().ShakeCamera(strength, duration);
  }
  else {
    serverCameraController.QueueShakeCamera(strength, duration);
  }
}

void Overworld::OnlineArea::receiveTrackWithCameraSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto some = reader.Read<bool>(buffer);

  if (some) {
    trackedPlayer = reader.ReadString<uint16_t>(buffer);
  }
  else {
    trackedPlayer = {};
  }
}

void Overworld::OnlineArea::receiveTeleportSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  // todo: prevent warp animation for other clients if warp = false?
  // maybe add a warped signal we send to the server?

  auto warp = reader.Read<bool>(buffer);
  auto x = reader.Read<float>(buffer);
  auto y = reader.Read<float>(buffer);
  auto z = reader.Read<float>(buffer);
  auto direction = reader.Read<Direction>(buffer);

  auto tileSize = GetMap().GetTileSize();
  auto position = sf::Vector3f(
    x * tileSize.x / 2.0f,
    y * tileSize.y,
    z
  );

  auto player = GetPlayer();

  if (warp) {
    auto& action = GetTeleportController().TeleportOut(player);
    action.onFinish.Slot([this, player, position, direction] {
      teleportIn(position, Orthographic(direction));
    });
  }
  else {
    player->Face(Orthographic(direction));
    player->Set3DPosition(position);
  }
}

void Overworld::OnlineArea::receiveMessageSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto message = reader.ReadString<uint16_t>(buffer);
  auto mugTexturePath = reader.ReadString<uint16_t>(buffer);
  auto mugAnimationPath = reader.ReadString<uint16_t>(buffer);

  sf::Sprite face;
  face.setTexture(*GetTexture(mugTexturePath));

  Animation animation;
  animation.LoadWithData(GetText(mugAnimationPath));

  auto& menuSystem = GetMenuSystem();
  menuSystem.SetNextSpeaker(face, animation);
  menuSystem.EnqueueMessage(message,
    [=]() { sendTextBoxResponseSignal(0); }
  );
}

void Overworld::OnlineArea::receiveQuestionSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto message = reader.ReadString<uint16_t>(buffer);
  auto mugTexturePath = reader.ReadString<uint16_t>(buffer);
  auto mugAnimationPath = reader.ReadString<uint16_t>(buffer);

  sf::Sprite face;
  face.setTexture(*GetTexture(mugTexturePath));

  Animation animation;
  animation.LoadWithData(GetText(mugAnimationPath));

  auto& menuSystem = GetMenuSystem();
  menuSystem.SetNextSpeaker(face, animation);
  menuSystem.EnqueueQuestion(message,
    [=](int response) { sendTextBoxResponseSignal((char)response); }
  );
}

void Overworld::OnlineArea::receiveQuizSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto optionA = reader.ReadString<uint16_t>(buffer);
  auto optionB = reader.ReadString<uint16_t>(buffer);
  auto optionC = reader.ReadString<uint16_t>(buffer);
  auto mugTexturePath = reader.ReadString<uint16_t>(buffer);
  auto mugAnimationPath = reader.ReadString<uint16_t>(buffer);

  sf::Sprite face;
  face.setTexture(*GetTexture(mugTexturePath));
  Animation animation;

  animation.LoadWithData(GetText(mugAnimationPath));

  auto& menuSystem = GetMenuSystem();
  menuSystem.SetNextSpeaker(face, animation);
  menuSystem.EnqueueQuiz(optionA, optionB, optionC,
    [=](int response) { sendTextBoxResponseSignal((char)response); }
  );
}

void Overworld::OnlineArea::receivePromptSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto characterLimit = reader.Read<uint16_t>(buffer);
  auto defaultText = reader.ReadString<uint16_t>(buffer);

  auto& menuSystem = GetMenuSystem();
  menuSystem.EnqueueTextInput(
    defaultText,
    characterLimit,
    [=](auto& response) { sendPromptResponseSignal(response); }
  );
}

static std::vector<BBS::Post> ReadPosts(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto total = reader.Read<uint16_t>(buffer);

  std::vector<BBS::Post> posts;
  posts.reserve(total);

  for (auto i = 0; i < total; i++) {
    auto id = reader.ReadString<uint16_t>(buffer);
    auto read = reader.Read<bool>(buffer);
    auto title = reader.ReadString<uint16_t>(buffer);
    auto author = reader.ReadString<uint16_t>(buffer);

    posts.push_back({
      id,
      read,
      title,
      author
      });
  }

  return posts;
}

void Overworld::OnlineArea::receiveOpenBoardSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  sendBoardOpenSignal();
  auto& menuSystem = GetMenuSystem();

  auto depth = reader.Read<unsigned char>(buffer);

  if (depth != menuSystem.CountBBS()) {
    // player closed the board this packet is referencing
    sendBoardCloseSignal();
    return;
  }

  auto topic = reader.ReadString<uint16_t>(buffer);
  auto r = reader.Read<unsigned char>(buffer);
  auto g = reader.Read<unsigned char>(buffer);
  auto b = reader.Read<unsigned char>(buffer);
  auto posts = ReadPosts(reader, buffer);

  menuSystem.EnqueueBBS(
    topic,
    sf::Color(r, g, b, 255),
    [=](auto id) { sendPostSelectSignal(id); },
    [=] { sendBoardCloseSignal(); }
  );

  auto& bbs = menuSystem.GetBBS()->get();
  bbs.AppendPosts(posts);

  bbs.SetLastPageCallback([=] { sendPostRequestSignal(); });
}

void Overworld::OnlineArea::receivePrependPostsSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& menuSystem = GetMenuSystem();

  auto depth = reader.Read<unsigned char>(buffer);

  if (depth != menuSystem.CountBBS()) {
    // player closed the board this packet is referencing
    return;
  }

  bool hasReference = reader.Read<bool>(buffer);
  std::string reference = hasReference ? reader.ReadString<uint16_t>(buffer) : "";
  auto posts = ReadPosts(reader, buffer);

  auto optionalBbs = menuSystem.GetBBS();

  if (!optionalBbs) {
    return;
  }

  auto& bbs = optionalBbs->get();

  if (!hasReference) {
    bbs.PrependPosts(posts);
  }
  else {
    bbs.PrependPosts(reference, posts);
  }
}

void Overworld::OnlineArea::receiveAppendPostsSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& menuSystem = GetMenuSystem();

  auto depth = reader.Read<unsigned char>(buffer);

  if (depth != menuSystem.CountBBS()) {
    // player closed the board this packet is referencing
    return;
  }

  auto hasReference = reader.Read<bool>(buffer);
  auto reference = hasReference ? reader.ReadString<uint16_t>(buffer) : "";
  auto posts = ReadPosts(reader, buffer);

  auto optionalBbs = menuSystem.GetBBS();

  if (!optionalBbs) {
    return;
  }

  auto& bbs = optionalBbs->get();

  if (!hasReference) {
    bbs.AppendPosts(posts);
  }
  else {
    bbs.AppendPosts(reference, posts);
  }
}

void Overworld::OnlineArea::receiveRemovePostSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& menuSystem = GetMenuSystem();

  auto depth = reader.Read<unsigned char>(buffer);

  if (depth != menuSystem.CountBBS()) {
    // player closed the board this packet is referencing
    return;
  }

  auto postId = reader.ReadString<uint16_t>(buffer);

  auto optionalBbs = menuSystem.GetBBS();

  if (!optionalBbs) {
    return;
  }

  auto& bbs = optionalBbs->get();

  bbs.RemovePost(postId);
}

void  Overworld::OnlineArea::receiveCloseBBSSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  GetMenuSystem().ClearBBS();
}

void Overworld::OnlineArea::receivePVPSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto addressString = reader.ReadString<uint16_t>(buffer);

  std::optional<CardFolder*> selectedFolder = GetSelectedFolder();
  CardFolder* folder;

  if (selectedFolder) {
    folder = (*selectedFolder)->Clone();

    // Shuffle our folder
    folder->Shuffle();
  }
  else {
    // use a new blank folder if we dont have a folder selected
    folder = new CardFolder();
  }

  NetPlayConfig config;

  // Play the pre battle sound
  Audio().Play(AudioType::PRE_BATTLE, AudioPriority::high);

  // Stop music and go to battle screen
  Audio().StopStream();

  // Configure the session
  config.myNavi = GetCurrentNavi();

  auto& meta = NAVIS.At(config.myNavi);
  const std::string& image = meta.GetMugshotTexturePath();
  const std::string& mugshotAnim = meta.GetMugshotAnimationPath();
  const std::string& emotionsTexture = meta.GetEmotionsTexturePath();
  auto mugshot = Textures().LoadTextureFromFile(image);
  auto emotions = Textures().LoadTextureFromFile(emotionsTexture);
  Player* player = meta.GetNavi();

  int fullHealth = player->GetHealth();
  player->SetHealth(GetPlayerSession().health);
  player->SetEmotion(GetPlayerSession().emotion);

  try {
    auto remote = Poco::Net::SocketAddress(addressString);
    netBattleProcessor = std::make_shared<Netplay::PacketProcessor>(remote, Net().GetMaxPayloadSize());
    Net().AddHandler(remote, netBattleProcessor);
  }
  catch (...) {
    return;
  }

  NetworkBattleSceneProps props = {
    { *player, GetProgramAdvance(), folder, new Field(6, 3), GetBackground() },
    sf::Sprite(*mugshot),
    mugshotAnim,
    emotions,
    config,
    netBattleProcessor
  };

  // isEnteringBattle = true;
  // 
  // EXAMPLE AND TEMP DEMO PURPOSES BELOW:
  BattleResultsFunc callback = [this](const BattleResults& results) {
    sendBattleResultsSignal(results);
  };

  getController().push<segue<WhiteWashFade>::to<NetworkBattleScene>>(props, callback);
}

void Overworld::OnlineArea::receiveActorConnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  std::string user = reader.ReadString<uint16_t>(buffer);
  std::string name = reader.ReadString<uint16_t>(buffer);
  std::string texturePath = reader.ReadString<uint16_t>(buffer);
  std::string animationPath = reader.ReadString<uint16_t>(buffer);
  auto direction = reader.Read<Direction>(buffer);
  float x = reader.Read<float>(buffer) * tileSize.x / 2.0f;
  float y = reader.Read<float>(buffer) * tileSize.y;
  float z = reader.Read<float>(buffer);
  bool solid = reader.Read<bool>(buffer);
  bool warpIn = reader.Read<bool>(buffer);
  float scaleX = reader.Read<float>(buffer);
  float scaleY = reader.Read<float>(buffer);
  float rotation = reader.Read<float>(buffer);
  std::optional<std::string> current_animation;

  if (reader.Read<bool>(buffer)) {
    current_animation = reader.ReadString<uint16_t>(buffer);
  }

  auto pos = sf::Vector3f(x, y, z);

  if (user == ticket) return;

  // ignore success, update if this player already exists
  auto [pair, isNew] = onlinePlayers.emplace(user, name);

  auto& ticket = pair->first;
  auto& onlinePlayer = pair->second;

  // cancel possible removal since we're trying to add this user back
  removePlayers.remove(user);
  onlinePlayer.disconnecting = false;

  // update
  onlinePlayer.timestamp = CurrentTime::AsMilli();
  onlinePlayer.startBroadcastPos = pos;
  onlinePlayer.endBroadcastPos = pos;
  onlinePlayer.idleDirection = Orthographic(direction);
  onlinePlayer.propertyAnimator.ToggleAudio(false);

  auto actor = onlinePlayer.actor;
  actor->Set3DPosition(pos);
  actor->Face(direction);
  actor->setTexture(GetTexture(texturePath));
  actor->scale(scaleX, scaleY);
  actor->rotate(rotation);

  Animation animation;
  animation.LoadWithData(GetText(animationPath));
  actor->LoadAnimations(animation);

  if (current_animation) {
    actor->PlayAnimation(*current_animation, true);
  }

  auto& emoteNode = onlinePlayer.emoteNode;
  float emoteY = -actor->getSprite().getOrigin().y - 10;
  emoteNode.setPosition(0, emoteY);
  emoteNode.setScale(0.5f, 0.5f);
  emoteNode.LoadCustomEmotes(GetCustomEmotesTexture());

  auto& teleportController = onlinePlayer.teleportController;

  if (isNew) {
    // add nodes to the scene base
    teleportController.EnableSound(false);
    AddSprite(teleportController.GetBeam());

    actor->AddNode(&emoteNode);
    actor->SetSolid(solid);
    actor->CollideWithMap(false);
    actor->SetCollisionRadius(6);
    actor->SetInteractCallback([=](const std::shared_ptr<Actor>& with) {
      sendNaviInteractionSignal(ticket);
    });

    AddActor(actor);
  }

  if (warpIn) {
    teleportController.TeleportIn(actor, pos, Direction::none);
  }
}

void Overworld::OnlineArea::receiveActorDisconnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString<uint16_t>(buffer);
  bool warpOut = reader.Read<bool>(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter == onlinePlayers.end()) {
    return;
  }

  auto& onlinePlayer = userIter->second;

  onlinePlayer.disconnecting = true;

  auto actor = onlinePlayer.actor;

  if (warpOut) {
    auto& teleport = onlinePlayer.teleportController;
    teleport.TeleportOut(actor).onFinish.Slot([=] {

      // search for the player again, in case they were already removed
      auto userIter = this->onlinePlayers.find(user);

      if (userIter == this->onlinePlayers.end()) {
        return;
      }

      auto& onlinePlayer = userIter->second;

      // make sure this player is still disconnecting
      if (onlinePlayer.disconnecting) {
        removePlayers.push_back(user);
      }
    });
  }
  else {
    removePlayers.push_back(user);
  }
}

void Overworld::OnlineArea::receiveActorSetNameSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString<uint16_t>(buffer);
  std::string name = reader.ReadString<uint16_t>(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    userIter->second.actor->Rename(name);
  }
}

void Overworld::OnlineArea::receiveActorMoveSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString<uint16_t>(buffer);

  // ignore our ip update
  if (user == ticket) {
    return;
  }


  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  float x = reader.Read<float>(buffer) * tileSize.x / 2.0f;
  float y = reader.Read<float>(buffer) * tileSize.y;
  float z = reader.Read<float>(buffer);
  auto direction = reader.Read<Direction>(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    // Calculate the NEXT frame and see if we're moving too far
    auto& onlinePlayer = userIter->second;
    auto currentTime = CurrentTime::AsMilli();
    bool animatingPos = onlinePlayer.propertyAnimator.IsAnimatingPosition();
    auto endBroadcastPos = onlinePlayer.endBroadcastPos;
    auto newPos = sf::Vector3f(x, y, z);
    auto delta = endBroadcastPos - newPos;
    float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
    double incomingLag = (currentTime - static_cast<double>(onlinePlayer.timestamp)) / 1000.0;

    // Adjust the lag time by the lag of this incoming frame
    double expectedTime = Net().CalculateLag(onlinePlayer.packets, onlinePlayer.lagWindow, incomingLag);

    auto teleportController = &onlinePlayer.teleportController;
    auto actor = onlinePlayer.actor;

    // Do not attempt to animate the teleport over quick movements if already teleporting or animating position
    if (teleportController->IsComplete() && onlinePlayer.packets > 1 && !animatingPos) {
      // we can't possibly have moved this far away without teleporting
      if (distance >= (onlinePlayer.actor->GetRunSpeed() * 2.f) * float(expectedTime)) {
        actor->Set3DPosition(endBroadcastPos);
        auto& action = teleportController->TeleportOut(actor);
        action.onFinish.Slot([=] {
          teleportController->TeleportIn(actor, newPos, Direction::none);
        });
      }
    }

    // update our records
    onlinePlayer.startBroadcastPos = animatingPos ? actor->Get3DPosition() : endBroadcastPos;
    onlinePlayer.endBroadcastPos = newPos;
    onlinePlayer.timestamp = currentTime;
    onlinePlayer.packets++;
    onlinePlayer.lagWindow[onlinePlayer.packets % NetManager::LAG_WINDOW_LEN] = incomingLag;
    onlinePlayer.idleDirection = Orthographic(direction);
  }
}

void Overworld::OnlineArea::receiveActorSetAvatarSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString<uint16_t>(buffer);
  std::string texturePath = reader.ReadString<uint16_t>(buffer);
  std::string animationPath = reader.ReadString<uint16_t>(buffer);

  EmoteNode* emoteNode;
  std::shared_ptr<Actor> actor;

  if (user == ticket) {
    actor = GetPlayer();
    emoteNode = &GetEmoteNode();
  }
  else {
    auto userIter = onlinePlayers.find(user);

    if (userIter == onlinePlayers.end()) return;

    auto& onlinePlayer = userIter->second;
    actor = onlinePlayer.actor;
    emoteNode = &onlinePlayer.emoteNode;
  }

  actor->setTexture(GetTexture(texturePath));

  Animation animation;
  animation.LoadWithData(GetText(animationPath));
  actor->LoadAnimations(animation);

  float emoteY = -actor->getSprite().getOrigin().y - emoteNode->getSprite().getLocalBounds().height / 2;
  emoteNode->setPosition(0, emoteY);
}

void Overworld::OnlineArea::receiveActorEmoteSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto user = reader.ReadString<uint16_t>(buffer);
  auto emote = reader.Read<uint8_t>(buffer);
  auto custom = reader.Read<bool>(buffer);

  if (user == ticket) {
    if (custom) {
      SceneBase::OnCustomEmoteSelected(emote);
    }
    else {
      SceneBase::OnEmoteSelected((Emotes)emote);
    }
    return;
  }

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto& onlinePlayer = userIter->second;

    if (custom) {
      onlinePlayer.emoteNode.CustomEmote(emote);
    }
    else {
      onlinePlayer.emoteNode.Emote((Emotes)emote);
    }
  }
}

void Overworld::OnlineArea::receiveActorAnimateSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto user = reader.ReadString<uint16_t>(buffer);
  auto state = reader.ReadString<uint16_t>(buffer);
  auto loop = reader.Read<bool>(buffer);

  if (user == ticket) {
    GetPlayer()->PlayAnimation(state, loop);
    return;
  }

  auto userIter = onlinePlayers.find(user);

  if (userIter == onlinePlayers.end()) return;

  auto& onlinePlayer = userIter->second;

  onlinePlayer.actor->PlayAnimation(state, loop);
}

void Overworld::OnlineArea::receiveActorKeyFramesSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto user = reader.ReadString<uint16_t>(buffer);

  // resolve target
  auto actor = GetPlayer();
  auto propertyAnimator = &this->propertyAnimator;

  if (user != ticket) {
    auto userIter = onlinePlayers.find(user);

    if (userIter == onlinePlayers.end()) return;

    auto& onlinePlayer = userIter->second;

    actor = onlinePlayer.actor;
    propertyAnimator = &onlinePlayer.propertyAnimator;
  }

  // reading keyframes
  auto tail = reader.Read<bool>(buffer);
  auto keyframeCount = reader.Read<uint16_t>(buffer);

  auto tileSize = GetMap().GetTileSize();
  auto xScale = tileSize.x / 2.0f;
  auto yScale = tileSize.y;

  for (auto i = 0; i < keyframeCount; i++) {
    ActorPropertyAnimator::KeyFrame keyframe;
    keyframe.duration = reader.Read<float>(buffer);

    auto propertyCount = reader.Read<uint16_t>(buffer);

    // resolving properties for this keyframe
    for (auto j = 0; j < propertyCount; j++) {
      ActorPropertyAnimator::PropertyStep propertyStep;

      propertyStep.ease = reader.Read<Ease>(buffer);
      propertyStep.property = reader.Read<ActorProperty>(buffer);

      switch (propertyStep.property) {
      case ActorProperty::animation:
        propertyStep.stringValue = reader.ReadString<uint16_t>(buffer);
        break;
      case ActorProperty::x:
        propertyStep.value = reader.Read<float>(buffer) * xScale;
        break;
      case ActorProperty::y:
        propertyStep.value = reader.Read<float>(buffer) * yScale;
        break;
      case ActorProperty::direction:
        propertyStep.value = (float)Orthographic(reader.Read<Direction>(buffer));
        break;
      case ActorProperty::sound_effect:
        propertyStep.stringValue = GetPath(reader.ReadString<uint16_t>(buffer));
        break;
      case ActorProperty::sound_effect_loop:
        propertyStep.stringValue = GetPath(reader.ReadString<uint16_t>(buffer));
        break;
      default:
        propertyStep.value = reader.Read<float>(buffer);
        break;
      }

      keyframe.propertySteps.push_back(propertyStep);
    }

    propertyAnimator->AddKeyFrame(keyframe);
  }

  if (tail) {
    propertyAnimator->UseKeyFrames(*actor);

    if (actor == GetPlayer() && propertyAnimator->IsAnimatingPosition()) {
      // release to block the controller from attempting to animate the player
      GetPlayerController().ReleaseActor();
    }
  }
}

void Overworld::OnlineArea::leave() {
  if (packetProcessor) {
    Net().DropProcessor(packetProcessor);
    packetProcessor = nullptr;
  }

  tryPopScene = true;
}

std::string Overworld::OnlineArea::GetText(const std::string& path) {
  if (path.find("/server", 0) == 0) {
    return serverAssetManager.GetText(path);
  }
  return Overworld::SceneBase::GetText(path);
}

std::shared_ptr<sf::Texture> Overworld::OnlineArea::GetTexture(const std::string& path) {
  if (path.find("/server", 0) == 0) {
    return serverAssetManager.GetTexture(path);
  }
  return Overworld::SceneBase::GetTexture(path);
}

std::shared_ptr<sf::SoundBuffer> Overworld::OnlineArea::GetAudio(const std::string& path) {
  if (path.find("/server", 0) == 0) {
    return serverAssetManager.GetAudio(path);
  }
  return Overworld::SceneBase::GetAudio(path);
}

std::string Overworld::OnlineArea::GetPath(const std::string& path) {
  if (path.find("/server", 0) == 0) {
    return serverAssetManager.GetPath(path);
  }

  return path;
}
