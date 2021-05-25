#include <Swoosh/Segue.h>
#include <Swoosh/ActivityController.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/BlackWashFade.h>
#include <Segues/WhiteWashFade.h>
#include <Poco/Net/NetException.h>
#include <filesystem>
#include <fstream>

#include "bnOverworldOnlineArea.h"
#include "../bnXmasBackground.h"
#include "../bnNaviRegistration.h"
#include "../netplay/battlescene/bnNetworkBattleScene.h"
#include "../netplay/bnNetPlayConfig.h"
#include "../bnMessageQuestion.h"

using namespace swoosh::types;
constexpr float SECONDS_PER_MOVEMENT = 1.f / 10.f;

static Direction resolveDirectionString(const std::string& direction) {
  if (direction == "Left") {
    return Direction::left;
  }
  else if (direction == "Right") {
    return Direction::right;
  }
  else if (direction == "Up") {
    return Direction::up;
  }
  else if (direction == "Down") {
    return Direction::down;
  }
  else if (direction == "Up Left") {
    return Direction::up_left;
  }
  else if (direction == "Up Right") {
    return Direction::up_right;
  }
  else if (direction == "Down Left") {
    return Direction::down_left;
  }
  else if (direction == "Down Right") {
    return Direction::down_right;
  }

  return Direction::none;
}

static const std::string sanitize_folder_name(std::string in) {
  // todo: use regex for multiple erroneous folder names?

  size_t pos = in.find('.');

  // Repeat till end is reached
  while (pos != std::string::npos)
  {
    in.replace(pos, 1, "_");
    pos = in.find('.', pos + 1);
  }

  pos = in.find(':');

  // find port
  if (pos != std::string::npos)
  {
    in.replace(pos, 1, "_p");
  }

  return in;
}

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
  remoteAddress(Poco::Net::SocketAddress(address, port)),
  packetProcessor(
    std::make_shared<Overworld::PacketProcessor>(
      remoteAddress,
      [this](auto& body) { processPacketBody(body); }
      )
  ),
  connectData(connectData),
  maxPayloadSize(maxPayloadSize),
  serverAssetManager("cache/" + sanitize_folder_name(remoteAddress.toString()))
{
  transitionText.setScale(2, 2);
  transitionText.SetString("Connecting...");

  lastFrameNavi = this->GetCurrentNavi();

  SetBackground(std::make_shared<XmasBackground>());

  Net().AddHandler(remoteAddress, packetProcessor);
}

Overworld::OnlineArea::~OnlineArea()
{
}

void Overworld::OnlineArea::onUpdate(double elapsed)
{
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

  if (!IsInputLocked()) {
    detectWarp();
  }

  if (Input().Has(InputEvents::pressed_shoulder_right) && !IsInputLocked() && emote.IsClosed()) {
    auto& meta = NAVIS.At(currentNavi);
    const std::string& image = meta.GetMugshotTexturePath();
    const std::string& anim = meta.GetMugshotAnimationPath();
    auto mugshot = Textures().LoadTextureFromFile(image);
    menuSystem.SetNextSpeaker(sf::Sprite(*mugshot), anim);

    menuSystem.EnqueueQuestion("Return to your homepage?", [this](bool result) {
      if (result) {
        teleportController.TeleportOut(playerActor).onFinish.Slot([this] {
          this->sendLogoutSignal();
          this->leave();
        });
      }
    });

    playerActor->Face(Direction::down_right);
  }

  auto currentNavi = GetCurrentNavi();
  if (lastFrameNavi != currentNavi) {
    sendAvatarChangeSignal();
    lastFrameNavi = currentNavi;
  }

  auto currentTime = CurrentTime::AsMilli();

  for (auto& pair : onlinePlayers) {
    auto& onlinePlayer = pair.second;
    auto& actor = onlinePlayer.actor;

    auto deltaTime = static_cast<double>(currentTime - onlinePlayer.timestamp) / 1000.0;
    auto delta = onlinePlayer.endBroadcastPos - onlinePlayer.startBroadcastPos;
    float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
    double expectedTime = calculatePlayerLag(onlinePlayer);
    float alpha = static_cast<float>(ease::linear(deltaTime, expectedTime, 1.0));
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

    auto newPos = onlinePlayer.startBroadcastPos + delta * alpha;
    actor->Set3DPosition(newPos);

    onlinePlayer.teleportController.Update(elapsed);
    onlinePlayer.emoteNode.Update(elapsed);
  }

  movementTimer.update(sf::seconds(static_cast<float>(elapsed)));

  if (movementTimer.getElapsed().asSeconds() > SECONDS_PER_MOVEMENT) {
    movementTimer.reset();
    sendPositionSignal();
  }

  lastPosition = GetPlayer()->Get3DPosition();
  SceneBase::onUpdate(elapsed);
}

void Overworld::OnlineArea::detectWarp() {
  auto& teleportController = GetTeleportController();

  if (!teleportController.IsComplete()) {
    return;
  }

  auto player = GetPlayer();
  auto layerIndex = player->GetLayer();

  auto& map = GetMap();

  if (layerIndex < 0 || layerIndex >= warps.size()) {
    return;
  }

  auto playerPos = player->getPosition();

  if (playerPos.x == lastPosition.x && playerPos.y == lastPosition.y) {
    // don't warp if the player hasn't moved
    return;
  }

  for (auto* tileObjectPtr : warps[layerIndex]) {
    auto& tileObject = *tileObjectPtr;

    if (!tileObject.Intersects(map, playerPos.x, playerPos.y)) {
      continue;
    }

    auto& command = teleportController.TeleportOut(player);
    auto type = tileObject.type;

    if (type == "Home Warp") {
      command.onFinish.Slot([=] {
        GetPlayerController().ReleaseActor();
        TeleportUponReturn(player->Get3DPosition());
        sendLogoutSignal();
        getController().pop<segue<BlackWashFade>>();
      });
    }
    else if (type == "Server Warp") {
      auto address = tileObject.customProperties.GetProperty("Address");
      auto port = (uint16_t)tileObject.customProperties.GetPropertyInt("Port");
      auto data = tileObject.customProperties.GetProperty("Data");

      command.onFinish.Slot([=] {
        GetPlayerController().ReleaseActor();
        transferServer(address, port, data, false);
      });
    }
    else if (type == "Position Warp") {
      auto targetTilePos = sf::Vector2f(
        tileObject.customProperties.GetPropertyFloat("X"),
        tileObject.customProperties.GetPropertyFloat("Y")
      );

      auto targetWorldPos = map.TileToWorld(targetTilePos);
      auto targetPosition = sf::Vector3f(targetWorldPos.x, targetWorldPos.y, tileObject.customProperties.GetPropertyFloat("Z"));
      auto direction = resolveDirectionString(tileObject.customProperties.GetProperty("Direction"));

      command.onFinish.Slot([=] {
        teleportIn(targetPosition, Orthographic(direction));
      });
    }
    else if (type == "Custom Warp" || type == "Custom Server Warp") {
      command.onFinish.Slot([=] {
        sendCustomWarpSignal(tileObject.id);
      });
    }

    break;
  }
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

  if (menuSystem.IsFullscreen()) {
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

  testActor(*playerActor);

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
    0.0
  );
}

void Overworld::OnlineArea::OnEmoteSelected(Overworld::Emotes emote)
{
  SceneBase::OnEmoteSelected(emote);
  sendEmoteSignal(emote);
}

bool Overworld::OnlineArea::positionIsInWarp(sf::Vector3f position) {
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

  return teleportController.TeleportIn(actor, position, direction);
}

void Overworld::OnlineArea::transferServer(const std::string& address, uint16_t port, const std::string& data, bool warpOut) {
  auto transfer = [=] {
    getController().replace<segue<BlackWashFade>::to<Overworld::OnlineArea>>(address, port, data, maxPayloadSize, !WEBCLIENT.IsLoggedIn());
  };

  if (warpOut) {
    GetPlayerController().ReleaseActor();
    auto& command = teleportController.TeleportOut(GetPlayer());
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
    case ServerEvents::map:
      receiveMapSignal(reader, data);
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
    case ServerEvents::unlock_camera:
      QueueUnlockCamera();
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
      receivePostSelectionAckSignal(reader, data);
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
    }
  }
  catch (Poco::IOException& e) {
    Logger::Logf("OnlineArea Network exception: %s", e.displayText().c_str());

    leave();
  }
}
void Overworld::OnlineArea::sendAssetFoundSignal(const std::string& path, uint64_t lastModified) {
  ClientEvents event{ ClientEvents::asset_found };

  Poco::Buffer<char> buffer{ 0 };
  buffer.append((char*)&event, sizeof(ClientEvents));
  buffer.append(path.c_str(), path.size() + 1);
  buffer.append((char*)&lastModified, sizeof(lastModified));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendAssetsFound() {
  for (auto& [name, meta] : serverAssetManager.GetCachedAssetList()) {
    sendAssetFoundSignal(name, meta.lastModified);
  }
}

void Overworld::OnlineArea::sendAssetStreamSignal(ClientAssetType assetType, uint16_t headerSize, const char* data, size_t size) {
  size_t remainingBytes = size;
  auto event = ClientEvents::asset_stream;

  // - 1 for asset type, - 2 for size
  const uint16_t availableRoom = maxPayloadSize - headerSize - 3;

  while (remainingBytes > 0) {
    uint16_t size = remainingBytes < availableRoom ? (uint16_t)remainingBytes : availableRoom;
    remainingBytes -= size;

    Poco::Buffer<char> buffer{ 0 };
    buffer.append((char*)&event, sizeof(ClientEvents));
    buffer.append(assetType);
    buffer.append((char*)&size, sizeof(uint16_t));
    buffer.append(data, size);
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

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::login };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append(username.data(), username.length());
  buffer.append(0);
  buffer.append(connectData.data(), connectData.length());
  buffer.append(0);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendLogoutSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::logout };
  buffer.append((char*)&type, sizeof(ClientEvents));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendRequestJoinSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::request_join };
  buffer.append((char*)&type, sizeof(ClientEvents));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendReadySignal()
{
  ClientEvents type{ ClientEvents::ready };

  Poco::Buffer<char> buffer{ 0 };
  buffer.append((char*)&type, sizeof(ClientEvents));

  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendTransferredOutSignal()
{
  auto type = ClientEvents::transferred_out;

  Poco::Buffer<char> buffer{ 0 };
  buffer.append((char*)&type, sizeof(ClientEvents));

  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendCustomWarpSignal(unsigned int tileObjectId)
{
  auto type = ClientEvents::custom_warp;

  Poco::Buffer<char> buffer{ 0 };
  buffer.append((char*)&type, sizeof(type));
  buffer.append((char*)&tileObjectId, sizeof(tileObjectId));
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

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::position };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&creationTime, sizeof(creationTime));
  buffer.append((char*)&x, sizeof(float));
  buffer.append((char*)&y, sizeof(float));
  buffer.append((char*)&z, sizeof(float));
  buffer.append((char*)&direction, sizeof(Direction));
  packetProcessor->SendKeepAlivePacket(Reliability::UnreliableSequenced, buffer);
}

void Overworld::OnlineArea::sendAvatarChangeSignal()
{
  sendAvatarAssetStream();

  // mark completion
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::avatar_change };
  buffer.append((char*)&type, sizeof(ClientEvents));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

static std::vector<char> readBytes(std::string texturePath) {
  size_t textureLength;
  std::vector<char> textureData;

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
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::emote };
  uint8_t val = static_cast<uint8_t>(emote);

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&val, sizeof(uint8_t));
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendObjectInteractionSignal(unsigned int tileObjectId)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::object_interaction };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&tileObjectId, sizeof(unsigned int));
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendNaviInteractionSignal(const std::string& ticket)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::actor_interaction };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append(ticket.c_str(), ticket.length() + 1);
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendTileInteractionSignal(float x, float y, float z)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::tile_interaction };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&x, sizeof(x));
  buffer.append((char*)&y, sizeof(y));
  buffer.append((char*)&z, sizeof(z));
  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendTextBoxResponseSignal(char response)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::textbox_response };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&response, sizeof(response));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}


void Overworld::OnlineArea::sendBoardOpenSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::board_open };

  buffer.append((char*)&type, sizeof(ClientEvents));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendBoardCloseSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::board_close };

  buffer.append((char*)&type, sizeof(ClientEvents));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendPostRequestSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::post_request };

  buffer.append((char*)&type, sizeof(ClientEvents));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendPostSelectSignal(const std::string& postId)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::post_selection };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append(postId.c_str(), postId.length() + 1);
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::receiveLoginSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = map.GetTileSize();

  this->ticket = reader.ReadString(buffer);
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

  isConnected = true;
  sendReadySignal();
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
  auto address = reader.ReadString(buffer);
  auto port = reader.Read<uint16_t>(buffer);
  auto data = reader.ReadString(buffer);
  auto warpOut = reader.Read<bool>(buffer);

  transferServer(address, port, data, warpOut);
}

void Overworld::OnlineArea::receiveKickSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string kickReason = reader.ReadString(buffer);
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
  auto path = reader.ReadString(buffer);

  serverAssetManager.RemoveAsset(path);
}

void Overworld::OnlineArea::receiveAssetStreamStartSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto name = reader.ReadString(buffer);
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
    incomingAsset.buffer.append(0);
    serverAssetManager.SetText(name, lastModified, assetReader.ReadString(incomingAsset.buffer), cachable);
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
  auto name = reader.ReadString(buffer);

  serverAssetManager.Preload(name);
}

void Overworld::OnlineArea::receiveMapSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto path = reader.ReadString(buffer);
  auto mapBuffer = GetText(path);

  LoadMap(mapBuffer);

  auto& map = GetMap();
  auto layerCount = map.GetLayerCount();

  for (auto& [objectId, excludedData] : excludedObjects) {
    for (auto i = 0; i < layerCount; i++) {
      auto& layer = map.GetLayer(i);

      auto optional_object_ref = layer.GetTileObject(objectId);

      if (optional_object_ref) {
        auto object_ref = optional_object_ref.value();
        auto& object = object_ref.get();

        excludedData.visible = object.visible;
        excludedData.solid = object.solid;

        object.visible = false;
        object.solid = false;
        break;
      }
    }
  }

  auto& minimap = GetMinimap();
  minimap.ClearIcons();
  warps.resize(layerCount);

  auto tileSize = map.GetTileSize();

  for (auto i = 0; i < layerCount; i++) {
    auto& warpLayer = warps[i];
    warpLayer.clear();

    for (auto& tileObject : map.GetLayer(i).GetTileObjects()) {
      const std::string& type = tileObject.type;

      if (type == "") continue;

      auto tileMeta = map.GetTileMeta(tileObject.tile.gid);

      if (!tileMeta) continue;

      auto screenOffset = tileMeta->drawingOffset;
      screenOffset.y += tileObject.size.y / 2.0f;

      auto objectCenterPos = tileObject.position + map.OrthoToIsometric(screenOffset);
      auto zOffset = sf::Vector2f(0, (float)(-i * tileSize.y / 2));

      if (type == "Home Warp") {
        minimap.SetHomepagePosition(map.WorldToScreen(objectCenterPos) + zOffset);
        tileObject.solid = false;
        warpLayer.push_back(&tileObject);
      }
      else if (type == "Server Warp" || type == "Custom Server Warp" || type == "Position Warp" || type == "Custom Warp") {
        minimap.AddWarpPosition(map.WorldToScreen(objectCenterPos) + zOffset);
        tileObject.solid = false;
        warpLayer.push_back(&tileObject);
      }
      else if (type == "Board") {
        minimap.AddBoardPosition(map.WorldToScreen(tileObject.position) + zOffset, tileObject.tile.flippedHorizontal);
      }
      else if (type == "Shop") {
        minimap.AddShopPosition(map.WorldToScreen(tileObject.position) + zOffset);
      }
    }
  }
}

void Overworld::OnlineArea::receivePlaySoundSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto name = reader.ReadString(buffer);

  Audio().Play(GetAudio(name));
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
      auto object_ref = optional_object_ref.value();
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
      auto object_ref = optional_object_ref.value();
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

  QueuePlaceCamera(screenPos, sf::seconds(duration));
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

  QueueMoveCamera(screenPos, sf::seconds(duration));
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
  auto message = reader.ReadString(buffer);
  auto mugTexturePath = reader.ReadString(buffer);
  auto mugAnimationPath = reader.ReadString(buffer);

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
  auto message = reader.ReadString(buffer);
  auto mugTexturePath = reader.ReadString(buffer);
  auto mugAnimationPath = reader.ReadString(buffer);

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
  auto optionA = reader.ReadString(buffer);
  auto optionB = reader.ReadString(buffer);
  auto optionC = reader.ReadString(buffer);
  auto mugTexturePath = reader.ReadString(buffer);
  auto mugAnimationPath = reader.ReadString(buffer);

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

static std::vector<BBS::Post> ReadPosts(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto total = reader.Read<uint16_t>(buffer);

  std::vector<BBS::Post> posts;
  posts.reserve(total);

  for (auto i = 0; i < total; i++) {
    auto id = reader.ReadString(buffer);
    auto read = reader.Read<bool>(buffer);
    auto title = reader.ReadString(buffer);
    auto author = reader.ReadString(buffer);

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

  auto topic = reader.ReadString(buffer);
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
  std::string reference = hasReference ? reader.ReadString(buffer) : "";
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
  auto reference = hasReference ? reader.ReadString(buffer) : "";
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

  auto postId = reader.ReadString(buffer);

  auto optionalBbs = menuSystem.GetBBS();

  if (!optionalBbs) {
    return;
  }

  auto& bbs = optionalBbs->get();

  bbs.RemovePost(postId);
}

void Overworld::OnlineArea::receivePostSelectionAckSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  GetMenuSystem().AcknowledgeBBSSelection();
}

void Overworld::OnlineArea::receivePVPSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto addressString = reader.ReadString(buffer);

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
  config.remote = addressString;
  config.myNavi = GetCurrentNavi();

  Player* player = NAVIS.At(config.myNavi).GetNavi();

  NetworkBattleSceneProps props = {
    { *player, GetProgramAdvance(), folder, new Field(6, 3), GetBackground() },
    config
  };

  getController().push<segue<WhiteWashFade>::to<NetworkBattleScene>>(props);
}

void Overworld::OnlineArea::receiveActorConnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  std::string user = reader.ReadString(buffer);
  std::string name = reader.ReadString(buffer);
  std::string texturePath = reader.ReadString(buffer);
  std::string animationPath = reader.ReadString(buffer);
  auto direction = reader.Read<Direction>(buffer);
  float x = reader.Read<float>(buffer) * tileSize.x / 2.0f;
  float y = reader.Read<float>(buffer) * tileSize.y;
  float z = reader.Read<float>(buffer);
  bool solid = reader.Read<bool>(buffer);
  bool warpIn = reader.Read<bool>(buffer);

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

  auto actor = onlinePlayer.actor;
  actor->Set3DPosition(pos);
  actor->Face(direction);
  actor->setTexture(GetTexture(texturePath));

  Animation animation;
  animation.LoadWithData(GetText(animationPath));
  actor->LoadAnimations(animation);

  auto& emoteNode = onlinePlayer.emoteNode;
  float emoteY = -actor->getSprite().getOrigin().y - emoteNode.getSprite().getLocalBounds().height / 2 - 1;
  emoteNode.setPosition(0, emoteY);

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
  std::string user = reader.ReadString(buffer);
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
  std::string user = reader.ReadString(buffer);
  std::string name = reader.ReadString(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    userIter->second.actor->Rename(name);
  }
}

void Overworld::OnlineArea::receiveActorMoveSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);

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

    // Calculcate the NEXT  frame and see if we're moving too far
    auto& onlinePlayer = userIter->second;
    auto currentTime = CurrentTime::AsMilli();
    auto endBroadcastPos = onlinePlayer.endBroadcastPos;
    auto newPos = sf::Vector3f(x, y, z);
    auto delta = endBroadcastPos - newPos;
    float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
    double incomingLag = (currentTime - static_cast<double>(onlinePlayer.timestamp)) / 1000.0;

    // Adjust the lag time by the lag of this incoming frame
    double expectedTime = calculatePlayerLag(onlinePlayer, incomingLag);

    auto teleportController = &onlinePlayer.teleportController;
    auto actor = onlinePlayer.actor;

    // Do not attempt to animate the teleport over quick movements if already teleporting
    if (teleportController->IsComplete() && onlinePlayer.packets > 1) {
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
    onlinePlayer.startBroadcastPos = endBroadcastPos;
    onlinePlayer.endBroadcastPos = newPos;
    onlinePlayer.timestamp = currentTime;
    onlinePlayer.packets++;
    onlinePlayer.lagWindow[onlinePlayer.packets % Overworld::LAG_WINDOW_LEN] = incomingLag;
    onlinePlayer.idleDirection = Orthographic(direction);
  }
}

void Overworld::OnlineArea::receiveActorSetAvatarSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  std::string texturePath = reader.ReadString(buffer);
  std::string animationPath = reader.ReadString(buffer);

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
  auto emote = reader.Read<Emotes>(buffer);
  auto user = reader.ReadString(buffer);

  if (user == ticket) {
    SceneBase::OnEmoteSelected(emote);
    return;
  }

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto& onlinePlayer = userIter->second;
    onlinePlayer.emoteNode.Emote(emote);
  }
}

void Overworld::OnlineArea::receiveActorAnimateSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto user = reader.ReadString(buffer);
  auto state = reader.ReadString(buffer);
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

void Overworld::OnlineArea::leave() {
  if (packetProcessor) {
    Net().DropProcessor(packetProcessor);
    packetProcessor = nullptr;
  }

  using effect = segue<PixelateBlackWashFade>;
  getController().pop<effect>();
}

const double Overworld::OnlineArea::calculatePlayerLag(OnlinePlayer& player, double nextLag)
{
  size_t window_len = std::min(player.packets, player.lagWindow.size());

  double avg{ 0 };
  for (size_t i = 0; i < window_len; i++) {
    avg = avg + player.lagWindow[i];
  }

  if (nextLag != 0.0) {
    avg = nextLag + avg;
    window_len++;
  }

  avg = avg / static_cast<double>(window_len);

  return avg;
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
