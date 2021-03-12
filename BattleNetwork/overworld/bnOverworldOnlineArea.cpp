#include <Swoosh/Segue.h>
#include <Swoosh/ActivityController.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/BlackWashFade.h>
#include <Poco/Net/NetException.h>

#include "bnOverworldOnlineArea.h"
#include "../bnXmasBackground.h"
#include "../bnNaviRegistration.h"
#include "../netplay/bnNetPlayConfig.h"
#include "../bnMessageQuestion.h"

using namespace swoosh::types;
constexpr float SECONDS_PER_MOVEMENT = 1.f / 10.f;
constexpr sf::Int32 MAX_TIMEOUT_SECONDS = 5;

const std::string sanitize_folder_name(std::string in) {
  // todo: use regex for multiple erroneous folder names?

  size_t pos = in.find(".");

  // Repeat till end is reached
  while (pos != std::string::npos)
  {
    in.replace(pos, 1, "_");
    pos = in.find(".", pos + 1);
  }

  pos = in.find(":");

  // Repeat till end is reached
  if (pos != std::string::npos)
  {
    in.replace(pos, 1, "_p");
  }

  return in;
}

Overworld::OnlineArea::OnlineArea(swoosh::ActivityController& controller, uint16_t maxPayloadSize, bool guestAccount) :
  SceneBase(controller, guestAccount),
  loadingText(Font::Style::small),
  nameText(Font::Style::small),
  remoteAddress(Poco::Net::SocketAddress(
    getController().CommandLineValue<std::string>("cyberworld"),
    getController().CommandLineValue<int>("remotePort")
  )),
  maxPayloadSize(maxPayloadSize),
  packetShipper(remoteAddress),
  packetSorter(remoteAddress),
  serverAssetManager("cache/" + sanitize_folder_name(remoteAddress.toString()))
{
  loadingText.setScale(2, 2);

  lastFrameNavi = this->GetCurrentNavi();
  packetResendTimer = PACKET_RESEND_RATE;

  int myPort = getController().CommandLineValue<int>("port");
  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), myPort);
  client = Poco::Net::DatagramSocket(sa);
  client.setBlocking(false);

  try {
    client.connect(remoteAddress);
  }
  catch (Poco::IOException& e) {
    Logger::Log(e.message());
  }

  SetBackground(new XmasBackground);
}

Overworld::OnlineArea::~OnlineArea()
{
  sendLogoutSignal();
}

void Overworld::OnlineArea::onUpdate(double elapsed)
{
  this->processIncomingPackets(elapsed);

  auto currentTime = CurrentTime::AsMilli();

  if (!isConnected) {
    return;
  }

  SceneBase::onUpdate(elapsed);

  if (lastFrameNavi != GetCurrentNavi()) {
    sendAvatarChangeSignal();
  }

  for (auto& pair : onlinePlayers) {
    auto& onlinePlayer = pair.second;
    auto& actor = onlinePlayer.actor;

    if (onlinePlayer.teleportController.IsComplete()) {
      auto deltaTime = static_cast<double>(currentTime - onlinePlayer.timestamp) / 1000.0;
      auto delta = onlinePlayer.endBroadcastPos - onlinePlayer.startBroadcastPos;
      float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
      double expectedTime = calculatePlayerLag(onlinePlayer);
      float alpha = static_cast<float>(ease::linear(deltaTime, expectedTime, 1.0));
      Direction newHeading = Actor::MakeDirectionFromVector(delta);

      if (distance <= 0.2f) {
        actor->Face(actor->GetHeading());
      }
      else if (distance <= actor->GetWalkSpeed() * expectedTime) {
        actor->Walk(newHeading, false); // Don't actually move or collide, but animate
      }
      else {
        actor->Run(newHeading, false);
      }

      auto newPos = onlinePlayer.startBroadcastPos + sf::Vector2f(delta.x * alpha, delta.y * alpha);
      actor->setPosition(newPos);
    }

    onlinePlayer.teleportController.Update(elapsed);
    onlinePlayer.emoteNode.Update(elapsed);
  }

  for (auto remove : removePlayers) {
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

  movementTimer.update(sf::seconds(static_cast<float>(elapsed)));

  if (movementTimer.getElapsed().asSeconds() > SECONDS_PER_MOVEMENT) {
    movementTimer.reset();
    sendPositionSignal();
  }
}

void Overworld::OnlineArea::onDraw(sf::RenderTexture& surface)
{
  if (isConnected) {
    SceneBase::onDraw(surface);
  }
  else {
    auto view = getController().getVirtualWindowSize();
    int precision = 1;

    loadingText.setPosition(view.x * 0.5f, view.y * 0.5f);
    loadingText.SetString("Connecting...");
    loadingText.setOrigin(loadingText.GetLocalBounds().width * 0.5f, loadingText.GetLocalBounds().height * 0.5f);
    surface.draw(loadingText);
  }

  auto& window = getController().getWindow();
  auto viewport = window.getViewport(window.getView());
  auto viewportOffset = sf::Vector2i(viewport.left, viewport.top);
  auto cameraView = GetCamera().GetView();

  // todo: mouse position is still read incorrectly when the game is scaled
  auto mouse = surface.mapPixelToCoords(sf::Mouse::getPosition(window) - viewportOffset, cameraView);

  for (auto& pair : onlinePlayers) {
    auto& onlinePlayer = pair.second;

    if (isMouseHovering(mouse, *onlinePlayer.actor)) {
      nameText.setPosition(mouse - cameraView.getCenter() + cameraView.getSize() / 2.f);
      nameText.SetString(onlinePlayer.actor->GetName());
      nameText.setOrigin(-10.0f, 0);
      surface.draw(nameText);
      break;
    }
  }
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

void Overworld::OnlineArea::onResume()
{
  playSong(GetMap().GetSongPath());
}

void Overworld::OnlineArea::OnTileCollision()
{
  auto playerActor = GetPlayer();
  auto playerPos = playerActor->getPosition();

  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  auto tilePos = sf::Vector2f(
    std::floor(playerPos.x / tileSize.x * 2.0f),
    std::floor(playerPos.y / tileSize.y)
  );

  auto& teleportController = GetTeleportController();

  for (auto& tileObject : map.GetLayer(playerActor->GetLayer()).GetTileObjects()) {
    if (tileObject.name != "Home Warp") {
      continue;
    }

    auto homeWarpPos = tileObject.position + map.OrthoToIsometric(sf::Vector2f(0, tileObject.size.y / 2.0f));
    auto homeWarpTilePos = sf::Vector2f(
      std::floor(homeWarpPos.x / tileSize.x * 2.0f),
      std::floor(homeWarpPos.y / tileSize.y)
    );

    if (homeWarpTilePos == tilePos && teleportController.IsComplete()) {
      GetPlayerController().ReleaseActor();
      auto& command = teleportController.TeleportOut(playerActor);

      auto teleportHome = [=] {
        TeleportUponReturn(playerActor->getPosition());
        sendLogoutSignal();
        getController().pop<segue<BlackWashFade>>();
      };

      command.onFinish.Slot(teleportHome);
    }
  }
}

void Overworld::OnlineArea::OnInteract() {
  auto& map = GetMap();
  auto playerActor = GetPlayer();

  // check to see what tile we pressed talk to
  auto layerIndex = playerActor->GetLayer();
  auto& layer = map.GetLayer(layerIndex);
  auto tileSize = map.GetTileSize();

  auto frontPosition = playerActor->PositionInFrontOf();

  for (auto& tileObject : layer.GetTileObjects()) {
    if (tileObject.Intersects(map, frontPosition.x, frontPosition.y)) {
      sendObjectInteractionSignal(tileObject.id);

      // block other interactions with return
      return;
    }
  }

  auto positionInFrontOffset = frontPosition - playerActor->getPosition();

  for (auto other : GetSpatialMap().GetChunk(frontPosition.x, frontPosition.y)) {
    if (playerActor == other) continue;

    auto collision = playerActor->CollidesWith(*other, positionInFrontOffset);

    if (collision) {
      other->Interact(playerActor);

      // block other interactions with return
      return;
    }
  }

  sendTileInteractionSignal(
    frontPosition.x / (tileSize.x / 2),
    frontPosition.y / tileSize.y,
    0.0
  );
}

void Overworld::OnlineArea::OnEmoteSelected(Overworld::Emotes emote)
{
  SceneBase::OnEmoteSelected(emote);
  sendEmoteSignal(emote);
}

void Overworld::OnlineArea::processIncomingPackets(double elapsed)
{
  auto timeDifference = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - packetSorter.GetLastMessageTime()
    );

  if (timeDifference.count() > MAX_TIMEOUT_SECONDS) {
    leave();
    return;
  }

  packetResendTimer -= elapsed;

  if (packetResendTimer < 0) {
    packetShipper.ResendBackedUpPackets(client);
    packetResendTimer = PACKET_RESEND_RATE;
  }

  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };

  try {
    while (client.available()) {
      Poco::Net::SocketAddress sender;
      int read = client.receiveFrom(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN, sender);

      if (sender != remoteAddress) {
        continue;
      }

      if (read == 0) {
        break;
      }

      Poco::Buffer<char> packet{ 0 };
      packet.append(rawBuffer, size_t(read));

      auto packetBodies = packetSorter.SortPacket(client, packet);

      for (auto& data : packetBodies) {
        BufferReader reader;

        auto sig = reader.Read<ServerEvents>(data);

        switch (sig) {
        case ServerEvents::ack:
        {
          Reliability r = reader.Read<Reliability>(data);
          uint64_t id = reader.Read<uint64_t>(data);
          packetShipper.Acknowledged(r, id);
          break;
        }
        case ServerEvents::login:
          receiveLoginSignal(reader, data);
          break;
        case ServerEvents::remove_asset:
          receiveAssetRemoveSignal(reader, data);
          break;
        case ServerEvents::asset_stream:
          receiveAssetStreamSignal(reader, data);
          break;
        case ServerEvents::asset_stream_complete:
          receiveAssetStreamCompleteSignal(reader, data);
          break;
        case ServerEvents::map:
          receiveMapSignal(reader, data);
          break;
        case ServerEvents::transfer_start:
          receiveTransferStartSignal(reader, data);
          break;
        case ServerEvents::transfer_complete:
          receiveTransferCompleteSignal(reader, data);
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
        case ServerEvents::move:
          receiveMoveSignal(reader, data);
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
        case ServerEvents::navi_connected:
          receiveNaviConnectedSignal(reader, data);
          break;
        case ServerEvents::navi_disconnect:
          receiveNaviDisconnectedSignal(reader, data);
          break;
        case ServerEvents::navi_set_name:
          receiveNaviSetNameSignal(reader, data);
          break;
        case ServerEvents::navi_move_to:
          receiveNaviMoveSignal(reader, data);
          break;
        case ServerEvents::navi_set_avatar:
          receiveNaviSetAvatarSignal(reader, data);
          break;
        case ServerEvents::navi_emote:
          receiveNaviEmoteSignal(reader, data);
          break;
        }
      }
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
  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendAssetsFound() {
  for (auto [name, lastModified] : serverAssetManager.GetCachedAssetList()) {
    sendAssetFoundSignal(name, lastModified);
  }
}

void Overworld::OnlineArea::sendTextureStreamHeaders(uint16_t width, uint16_t height) {
  ClientEvents event{ ClientEvents::texture_stream };
  uint16_t size = sizeof(uint16_t) * 2;

  Poco::Buffer<char> buffer{ 0 };
  buffer.append((char*)&event, sizeof(ClientEvents));
  buffer.append((char*)&size, sizeof(uint16_t));
  buffer.append((char*)&width, sizeof(uint16_t));
  buffer.append((char*)&height, sizeof(uint16_t));
  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendAssetStreamSignal(ClientEvents event, uint16_t headerSize, const char* data, size_t size) {
  size_t remainingBytes = size;

  while (remainingBytes > 0) {
    const uint16_t availableRoom = maxPayloadSize - headerSize - 2;
    uint16_t size = remainingBytes < availableRoom ? remainingBytes : availableRoom;
    remainingBytes -= size;

    Poco::Buffer<char> buffer{ 0 };
    buffer.append((char*)&event, sizeof(ClientEvents));
    buffer.append((char*)&size, sizeof(uint16_t));
    buffer.append(data, size);
    packetShipper.Send(client, Reliability::ReliableOrdered, buffer);

    data += size;
  }
}

void Overworld::OnlineArea::sendLoginSignal()
{
  std::string username = "James";
  std::string password = ""; // No servers need passwords at this time

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::login };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append(username.data(), username.length());
  buffer.append(0);
  buffer.append(password.data(), password.length());
  buffer.append(0);
  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendLogoutSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::logout };
  buffer.append((char*)&type, sizeof(ClientEvents));
  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendRequestJoinSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::request_join };
  buffer.append((char*)&type, sizeof(ClientEvents));
  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendReadySignal()
{
  ClientEvents type{ ClientEvents::ready };

  Poco::Buffer<char> buffer{ 0 };
  buffer.append((char*)&type, sizeof(ClientEvents));

  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendPositionSignal()
{
  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  auto vec = GetPlayer()->getPosition();
  float x = vec.x / tileSize.x * 2.0f;
  float y = vec.y / tileSize.y;
  float z = 0;

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::position };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&x, sizeof(float));
  buffer.append((char*)&y, sizeof(float));
  buffer.append((char*)&z, sizeof(float));
  packetShipper.Send(client, Reliability::UnreliableSequenced, buffer);
}

void Overworld::OnlineArea::sendAvatarChangeSignal()
{
  sendAvatarAssetStream();

  // mark completion
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::avatar_change };
  buffer.append((char*)&type, sizeof(ClientEvents));
  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendAvatarAssetStream() {
  auto& naviMeta = NAVIS.At(GetCurrentNavi());

  // send texture
  auto naviImage = naviMeta.GetOverworldTexture()->copyToImage();
  auto textureDimensions = naviImage.getSize();

  size_t textureSize = textureDimensions.x * textureDimensions.y * 4;
  const char* textureData = (const char*)naviImage.getPixelsPtr();

  std::string animationData = FileUtil::Read(naviMeta.GetOverworldAnimationPath());

  // + reliability type + id + packet type
  auto packetHeaderSize = 1 + 8 + 2;
  sendTextureStreamHeaders(textureDimensions.x, textureDimensions.y);
  sendAssetStreamSignal(ClientEvents::texture_stream, packetHeaderSize, textureData, textureSize);
  sendAssetStreamSignal(ClientEvents::animation_stream, packetHeaderSize, animationData.c_str(), animationData.length());
}

void Overworld::OnlineArea::sendEmoteSignal(const Overworld::Emotes emote)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::emote };
  uint8_t val = static_cast<uint8_t>(emote);

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&val, sizeof(uint8_t));
  packetShipper.Send(client, Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendObjectInteractionSignal(unsigned int tileObjectId)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::object_interaction };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&tileObjectId, sizeof(unsigned int));
  packetShipper.Send(client, Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendNaviInteractionSignal(const std::string& ticket)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::navi_interaction };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append(ticket.c_str(), ticket.length() + 1);
  packetShipper.Send(client, Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendTileInteractionSignal(float x, float y, float z)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::tile_interaction };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&x, sizeof(x));
  buffer.append((char*)&y, sizeof(y));
  buffer.append((char*)&z, sizeof(z));
  packetShipper.Send(client, Reliability::Reliable, buffer);
}

void Overworld::OnlineArea::sendDialogResponseSignal(char response)
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::dialog_response };

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&response, sizeof(response));
  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::receiveLoginSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  sendReadySignal();
  isConnected = true;

  this->ticket = reader.ReadString(buffer);

  auto& map = GetMap();
  sf::Vector2f spawnPos;

  for (auto& tileObject : map.GetLayer(0).GetTileObjects()) {
    if (tileObject.name == "Home Warp") {
      spawnPos = tileObject.position + map.OrthoToIsometric(sf::Vector2f(0, tileObject.size.y / 2.0f));
    }
  }

  auto& command = GetTeleportController().TeleportIn(GetPlayer(), spawnPos, Direction::up);
  command.onFinish.Slot([=] {
    GetPlayerController().ControlActor(GetPlayer());
  });
}

void Overworld::OnlineArea::receiveAssetRemoveSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto path = reader.ReadString(buffer);

  serverAssetManager.RemoveAsset(path);
}

void Overworld::OnlineArea::receiveAssetStreamSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto size = reader.Read<uint16_t>(buffer);

  assetBuffer.append(buffer.begin() + reader.GetOffset() + 2, size);
}

void Overworld::OnlineArea::receiveAssetStreamCompleteSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto name = reader.ReadString(buffer);
  auto lastModified = reader.Read<uint64_t>(buffer);
  auto cachable = reader.Read<bool>(buffer);
  auto type = reader.Read<AssetType>(buffer);

  BufferReader assetReader;

  switch (type) {
  case AssetType::text:
    assetBuffer.append(0);
    serverAssetManager.SetText(name, lastModified, assetReader.ReadString(assetBuffer), cachable);
    break;
  case AssetType::texture:
    serverAssetManager.SetTexture(name, lastModified, assetBuffer.begin(), assetBuffer.size(), cachable);
    break;
  case AssetType::audio:
    serverAssetManager.SetAudio(name, lastModified, assetBuffer.begin(), assetBuffer.size(), cachable);
    break;
  case AssetType::sfml_image:
  {
    if (assetBuffer.size() < sizeof(uint16_t) * 2) {
      break;
    }
    auto width = assetReader.Read<uint16_t>(assetBuffer);
    auto height = assetReader.Read<uint16_t>(assetBuffer);

    sf::Image image;
    image.create(width, height, (const sf::Uint8*)assetBuffer.begin() + assetReader.GetOffset());

    auto texture = std::make_shared<sf::Texture>();
    texture->loadFromImage(image);
    serverAssetManager.SetTextureDirect(name, texture);
    break;
  }
  }

  assetBuffer.setCapacity(0);
}


void Overworld::OnlineArea::receiveMapSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto path = reader.ReadString(buffer);
  auto mapBuffer = GetText(path);

  auto lastSongPath = GetMap().GetSongPath();

  LoadMap(mapBuffer);

  auto newSongPath = GetMap().GetSongPath();

  if (lastSongPath != newSongPath) {
    playSong(newSongPath);
  }
}

void Overworld::OnlineArea::receiveTransferStartSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  LockInput();
  isConnected = false;
  removePlayers.clear();

  for (auto& [key, _] : onlinePlayers) {
    removePlayers.push_back(key);
  }
}

void Overworld::OnlineArea::receiveTransferCompleteSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  UnlockInput();
  isConnected = true;
  sendReadySignal();
}

void Overworld::OnlineArea::receiveMoveCameraSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = map.GetTileSize();

  auto x = reader.Read<float>(buffer)* tileSize.x / 2.0f;
  auto y = reader.Read<float>(buffer)* tileSize.y;
  auto z = reader.Read<float>(buffer);

  auto position = sf::Vector2f(x, y);

  auto ortho = map.WorldToScreen(position);

  auto duration = reader.Read<float>(buffer);

  QueuePlaceCamera(ortho, sf::seconds(duration));
}

void Overworld::OnlineArea::receiveSlideCameraSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = map.GetTileSize();

  auto x = reader.Read<float>(buffer)* tileSize.x / 2.0f;
  auto y = reader.Read<float>(buffer)* tileSize.y;
  auto z = reader.Read<float>(buffer);

  auto position = sf::Vector2f(x, y);

  auto ortho = map.WorldToScreen(position);

  auto duration = reader.Read<float>(buffer);

  QueueMoveCamera(ortho, sf::seconds(duration));
}

void Overworld::OnlineArea::receiveMoveSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  // todo: add warp option, add no beam teleport option, interpolate in update?

  float x = reader.Read<float>(buffer);
  float y = reader.Read<float>(buffer);

  auto tileSize = GetMap().GetTileSize();
  auto position = sf::Vector2f(
     x * tileSize.x / 2.0f,
     y * tileSize.y
  );

  auto z = reader.Read<float>(buffer);

  auto player = GetPlayer();
  player->setPosition(position);
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

  auto& textbox = GetTextBox();
  textbox.SetNextSpeaker(face, animation);
  textbox.EnqueueMessage(message,
    [=]() { sendDialogResponseSignal(0); }
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

  auto& textbox = GetTextBox();
  textbox.SetNextSpeaker(face, animation);
  textbox.EnqueueQuestion(message,
    [=](int response) { sendDialogResponseSignal(response); }
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

  auto& textbox = GetTextBox();
  textbox.SetNextSpeaker(face, animation);
  textbox.EnqueueQuiz(optionA, optionB, optionC,
    [=](int response) { sendDialogResponseSignal(response); }
  );
}

void Overworld::OnlineArea::receiveNaviConnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  std::string user = reader.ReadString(buffer);
  std::string name = reader.ReadString(buffer);
  std::string texturePath = reader.ReadString(buffer);
  std::string animationPath = reader.ReadString(buffer);
  float x = reader.Read<float>(buffer) * tileSize.x / 2.0f;
  float y = reader.Read<float>(buffer) * tileSize.y;
  float z = reader.Read<float>(buffer);
  bool solid = reader.Read<bool>(buffer);
  bool warpIn = reader.Read<bool>(buffer);

  auto pos = sf::Vector2f(x, y);

  if (user == ticket) return;

  auto [pair, success] = onlinePlayers.emplace(user, name);

  if (!success) return;

  auto& ticket = pair->first;
  auto& onlinePlayer = pair->second;
  onlinePlayer.timestamp = CurrentTime::AsMilli();
  onlinePlayer.startBroadcastPos = pos;
  onlinePlayer.endBroadcastPos = pos;

  auto actor = onlinePlayer.actor;
  actor->setPosition(pos);
  actor->setTexture(GetTexture(texturePath));

  Animation animation;
  animation.LoadWithData(GetText(animationPath));
  actor->LoadAnimations(animation);

  auto& emoteNode = onlinePlayer.emoteNode;
  float emoteY = -actor->getOrigin().y - emoteNode.getSprite().getLocalBounds().height / 2;
  emoteNode.setPosition(0, emoteY);
  actor->AddNode(&emoteNode);

  actor->SetSolid(solid);
  actor->CollideWithMap(false);
  actor->SetCollisionRadius(6);
  actor->SetInteractCallback([=](std::shared_ptr<Actor> with) {
    sendNaviInteractionSignal(ticket);
  });

  AddActor(actor);

  auto& teleportController = onlinePlayer.teleportController;
  teleportController.EnableSound(false);
  AddSprite(teleportController.GetBeam());

  if (warpIn) {
    teleportController.TeleportIn(actor, pos, Direction::none);
  }
}

void Overworld::OnlineArea::receiveNaviDisconnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  bool warpOut = reader.Read<bool>(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter == onlinePlayers.end()) {
    return;
  }

  auto& onlinePlayer = userIter->second;
  auto actor = onlinePlayer.actor;

  if (warpOut) {
    auto& teleport = onlinePlayer.teleportController;
    teleport.TeleportOut(actor).onFinish.Slot([=] {
      removePlayers.push_back(user);
    });
  }
  else {
    removePlayers.push_back(user);
  }
}

void Overworld::OnlineArea::receiveNaviSetNameSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  std::string name = reader.ReadString(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    userIter->second.actor->Rename(name);
  }
}

void Overworld::OnlineArea::receiveNaviMoveSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
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

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {

    // Calculcate the NEXT  frame and see if we're moving too far
    auto& onlinePlayer = userIter->second;
    auto currentTime = CurrentTime::AsMilli();
    auto endBroadcastPos = onlinePlayer.endBroadcastPos;
    auto newPos = sf::Vector2f(x, y);
    auto deltaTime = static_cast<double>(currentTime - onlinePlayer.timestamp) / 1000.0;
    auto delta = endBroadcastPos - newPos;
    float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
    double incomingLag = (currentTime - static_cast<double>(onlinePlayer.timestamp)) / 1000.0;

    // Adjust the lag time by the lag of this incoming frame
    double expectedTime = calculatePlayerLag(onlinePlayer, incomingLag);

    Direction newHeading = Actor::MakeDirectionFromVector(delta);
    auto teleportController = &onlinePlayer.teleportController;
    auto actor = onlinePlayer.actor;

    // Do not attempt to animate the teleport over quick movements if already teleporting
    if (teleportController->IsComplete() && onlinePlayer.packets > 1) {
      // we can't possibly have moved this far away without teleporting
      if (distance >= (onlinePlayer.actor->GetRunSpeed() * 2) * expectedTime) {
        actor->setPosition(endBroadcastPos);
        auto& action = teleportController->TeleportOut(actor);
        action.onFinish.Slot([=] {
          teleportController->TeleportIn(actor, endBroadcastPos, Direction::none);
        });
      }
    }

    // update our records
    onlinePlayer.startBroadcastPos = endBroadcastPos;
    onlinePlayer.endBroadcastPos = sf::Vector2f(x, y);
    auto previous = onlinePlayer.timestamp;
    onlinePlayer.timestamp = currentTime;
    onlinePlayer.packets++;
    onlinePlayer.lagWindow[onlinePlayer.packets % Overworld::LAG_WINDOW_LEN] = incomingLag;
  }
}

void Overworld::OnlineArea::receiveNaviSetAvatarSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  std::string texturePath = reader.ReadString(buffer);
  std::string animationPath = reader.ReadString(buffer);

  if (user == ticket) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter == onlinePlayers.end()) return;

  auto& onlinePlayer = userIter->second;
  auto& actor = onlinePlayer.actor;

  actor->setTexture(GetTexture(texturePath));

  Animation animation;
  animation.LoadWithData(GetText(animationPath));
  actor->LoadAnimations(animation);

  auto& emoteNode = onlinePlayer.emoteNode;
  float emoteY = -actor->getOrigin().y - emoteNode.getSprite().getLocalBounds().height / 2;
  emoteNode.setPosition(0, emoteY);
}

void Overworld::OnlineArea::receiveNaviEmoteSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  uint8_t emote = reader.Read<uint8_t>(buffer);
  std::string user = reader.ReadString(buffer);

  if (user == ticket) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto& onlinePlayer = userIter->second;
    onlinePlayer.emoteNode.Emote(static_cast<Overworld::Emotes>(emote));
  }
}

void Overworld::OnlineArea::leave() {
  using effect = segue<PixelateBlackWashFade>;
  getController().pop<effect>();
}

const bool Overworld::OnlineArea::isMouseHovering(const sf::Vector2f& mouse, const SpriteProxyNode& src)
{
  auto textureRect = src.getSprite().getTextureRect();

  auto& map = GetMap();
  auto& scale = map.getScale();

  auto position = src.getPosition();
  auto screenPosition = map.WorldToScreen(position);
  auto bounds = sf::FloatRect(
    (screenPosition.x - textureRect.width / 2) * scale.x,
    (screenPosition.y - textureRect.height) * scale.y,
    textureRect.width * scale.x,
    textureRect.height * scale.y
  );

  return (mouse.x >= bounds.left && mouse.x <= bounds.left + bounds.width && mouse.y >= bounds.top && mouse.y <= bounds.top + bounds.height);
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

void Overworld::OnlineArea::playSong(const std::string& path) {
  auto songPath = path;

  if (songPath.find("/server", 0) == 0) {
    songPath = serverAssetManager.GetPath(path);
  }

  Audio().Stream(songPath, true);
}
