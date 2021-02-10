#include <Swoosh/Segue.h>
#include <Swoosh/ActivityController.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/BlackWashFade.h>
#include <Poco/Net/NetException.h>

#include "bnOverworldOnlineArea.h"
#include "../bnXmasBackground.h"
#include "../bnNaviRegistration.h"
#include "../netplay/bnNetPlayConfig.h"

using namespace swoosh::types;
constexpr float SECONDS_PER_MOVEMENT = 1.f / 10.f;
constexpr sf::Int32 MAX_TIMEOUT_SECONDS = 5;

Overworld::OnlineArea::OnlineArea(swoosh::ActivityController& controller, bool guestAccount) :
  font(Font::Style::small),
  name(font),
  SceneBase(controller, guestAccount),
  remoteAddress(Poco::Net::SocketAddress(
    getController().CommandLineValue<std::string>("cyberworld"),
    getController().CommandLineValue<int>("remotePort")
  )),
  packetShipper(remoteAddress),
  packetSorter(remoteAddress)
{
  lastFrameNavi = this->GetCurrentNavi();
  packetResendTimer = PACKET_RESEND_RATE;

  int myPort = getController().CommandLineValue<int>("port");
  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), myPort);
  client = Poco::Net::DatagramSocket(sa);
  client.setBlocking(false);

  try {
    client.connect(remoteAddress);
  }
  catch (Poco::Net::NetException& e) {
    Logger::Log(e.message());
  }

  SetBackground(new XmasBackground);
}

Overworld::OnlineArea::~OnlineArea()
{
  auto& map = GetMap();

  for (auto player : onlinePlayers) {
    RemoveSprite(&(player.second->actor));
    delete player.second;
  }

  sendLogoutSignal();

  onlinePlayers.clear();
}

void Overworld::OnlineArea::onUpdate(double elapsed)
{
  this->processIncomingPackets(elapsed);

  auto currentTime = CurrentTime::AsMilli();

  if (mapBuffer.empty() == false) {
    SceneBase::onUpdate(elapsed);

    if (lastFrameNavi != GetCurrentNavi()) {
      sendAvatarChangeSignal();
    }

    for (auto player : onlinePlayers) {
      auto* onlinePlayer = player.second;
      auto& actor = player.second->actor;

      if (onlinePlayer->teleportController.IsComplete()) {
        auto deltaTime = static_cast<double>(currentTime - onlinePlayer->timestamp) / 1000.0;
        auto delta = player.second->endBroadcastPos - player.second->startBroadcastPos;
        float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
        double expectedTime = calculatePlayerLag(*onlinePlayer);
        float alpha = static_cast<float>(ease::linear(deltaTime, expectedTime, 1.0));
        Direction newHeading = Actor::MakeDirectionFromVector(delta, 0.01f);

        if (distance <= 0.2f) {
          actor.Face(actor.GetHeading());
        }
        else if (distance <= actor.GetWalkSpeed() * expectedTime) {
          actor.Walk(newHeading, false); // Don't actually move or collide, but animate
        }
        else {
          actor.Run(newHeading, false);
        }

        auto newPos = player.second->startBroadcastPos + sf::Vector2f(delta.x * alpha, delta.y * alpha);
        actor.setPosition(newPos);
      }

      player.second->teleportController.Update(elapsed);
      actor.Update(elapsed);
      player.second->emoteNode.Update(elapsed);
    }

    for (auto remove : removePlayers) {
      onlinePlayers.erase(remove);
    }

    removePlayers.clear();

    movementTimer.update(sf::seconds(static_cast<float>(elapsed)));

    if (movementTimer.getElapsed().asSeconds() > SECONDS_PER_MOVEMENT) {
      movementTimer.reset();
      sendPositionSignal();
    }
  }
}

void Overworld::OnlineArea::onDraw(sf::RenderTexture& surface)
{
  if (mapBuffer.empty() == false && isConnected) {
    SceneBase::onDraw(surface);
  }
  else {
    auto view = getController().getVirtualWindowSize();
    int precision = 1;

    name.setPosition(view.x * 0.5f, view.y * 0.5f);
    name.SetString("Connecting...");
    name.setOrigin(name.GetLocalBounds().width * 0.5f, name.GetLocalBounds().height * 0.5f);
    surface.draw(name);
  }

  for (auto player : onlinePlayers) {
    if (isMouseHovering(surface, player.second->actor)) {
      std::string nameStr = player.second->actor.GetName();
      auto mousei = sf::Mouse::getPosition(getController().getWindow());
      auto mousef = sf::Vector2f(static_cast<float>(mousei.x), static_cast<float>(mousei.y));
      name.setPosition(mousef);
      name.SetString(nameStr.c_str());
      name.setOrigin(-10.0f, 0);
      surface.draw(name);
      continue;
    }
  }
}

void Overworld::OnlineArea::onStart()
{
  SceneBase::onStart();
  movementTimer.start();
  sendLoginSignal();
  Audio().Stream("resources/loops/loop_overworld.ogg", false);
}

void Overworld::OnlineArea::onResume()
{
  Audio().Stream("resources/loops/loop_overworld.ogg", false);
}

// NOTE: these are super hacky to make it look like maps have more going on for them (e.g. connected warps...)
// TODO: replace
void Overworld::OnlineArea::OnTileCollision()
{
  // on collision with warps
  // static size_t next_warp = 0, next_warp_2 = 1;
  // auto* teleportController = &GetTeleportControler();

  // if (tile.token == "W" && teleportController->IsComplete()) {
  //   auto& map = GetMap();
  //   auto* playerController = &GetPlayerController();
  //   auto* playerActor = &GetPlayer();

  //   auto warps = map.FindToken("W");

  //   if (warps.size()) {
  //     if (++next_warp >= warps.size()) {
  //       next_warp = 0;
  //     }

  //     auto teleportToNextWarp = [=] {
  //       auto finishTeleport = [=] {
  //         playerController->ControlActor(*playerActor);
  //       };

  //       auto& command = teleportController->TeleportIn(*playerActor, warps[next_warp], Direction::up);
  //       command.onFinish.Slot(finishTeleport);
  //     };

  //     playerController->ReleaseActor();
  //     auto& command = teleportController->TeleportOut(*playerActor);
  //     command.onFinish.Slot(teleportToNextWarp);
  //   }
  // }

  // if (tile.token == "W2" && teleportController->IsComplete()) {
  //   auto& map = GetMap();
  //   auto* playerController = &GetPlayerController();
  //   auto* playerActor = &GetPlayer();

  //   auto warps = map.FindToken("W2");

  //   if (warps.size()) {
  //     if (++next_warp_2 >= warps.size()) {
  //       next_warp_2 = 0;
  //     }

  //     auto teleportToNextWarp = [=] {
  //       auto finishTeleport = [=] {
  //         playerController->ControlActor(*playerActor);
  //       };

  //       auto& command = teleportController->TeleportIn(*playerActor, warps[next_warp_2], Direction::up);
  //       command.onFinish.Slot(finishTeleport);
  //     };

  //     playerController->ReleaseActor();
  //     auto& command = teleportController->TeleportOut(*playerActor);
  //     command.onFinish.Slot(teleportToNextWarp);
  //   }
  // }

  // // on collision with homepage warps
  // if (tile.token == "H" && teleportController->IsComplete()) {
  //   auto& map = GetMap();
  //   auto* playerController = &GetPlayerController();
  //   auto* playerActor = &GetPlayer();

  //   playerController->ReleaseActor();
  //   auto& command = teleportController->TeleportOut(*playerActor);

  //   auto teleportHome = [=] {
  //     TeleportUponReturn(playerActor->getPosition());
  //     sendLogoutSignal();
  //     getController().pop<segue<BlackWashFade>>();
  //   };

  //   command.onFinish.Slot(teleportHome);
  // }
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

  if (!client.available()) return;

  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };

  try {
    packetResendTimer -= elapsed;

    if (packetResendTimer < 0) {
      packetShipper.ResendBackedUpPackets(client);
      packetResendTimer = PACKET_RESEND_RATE;
    }

    Poco::Net::SocketAddress sender;
    int read = client.receiveFrom(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN, sender);

    if (sender != remoteAddress || read == 0) {
      return;
    }

    Poco::Buffer<char> packet{ 0 };
    packet.append(rawBuffer, size_t(read));

    auto packetBodies = packetSorter.SortPacket(client, packet);

    for (auto& data : packetBodies) {
      BufferReader reader;

      auto sig = reader.Read<ServerEvents>(data);

      switch (sig) {
      case ServerEvents::ack:
        packetShipper.Acknowledged(reader.Read<Reliability>(data), reader.Read<uint64_t>(data));
        break;
      case ServerEvents::login:
        receiveLoginSignal(reader, data);
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
  catch (Poco::Net::NetException& e) {
    Logger::Logf("OnlineArea Network exception: %s", e.what());

    leave();
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
    const uint16_t availableRoom = NetPlayConfig::MAX_BUFFER_LEN - headerSize - 2;
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
  sendAvatarAssetStream();

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

void Overworld::OnlineArea::sendReadySignal()
{
  ClientEvents type{ ClientEvents::ready };

  Poco::Buffer<char> buffer{ 0 };
  buffer.append((char*)&type, sizeof(ClientEvents));

  packetShipper.Send(client, Reliability::ReliableOrdered, buffer);
}

void Overworld::OnlineArea::sendPositionSignal()
{
  auto vec = GetPlayer().getPosition();
  float x = vec.x;
  float y = vec.y;
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
  packetShipper.Send(client, Reliability::Reliable, buffer);
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

void Overworld::OnlineArea::receiveLoginSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  sendReadySignal();
  isConnected = true;

  this->ticket = reader.ReadString(buffer);
}

void Overworld::OnlineArea::receiveAssetStreamSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto size = reader.Read<uint16_t>(buffer);

  assetBuffer.append(buffer.begin() + reader.GetOffset() + 2, size);
}

void Overworld::OnlineArea::receiveAssetStreamCompleteSignal(BufferReader& reader, const Poco::Buffer<char>& buffer) {
  auto name = reader.ReadString(buffer);
  auto type = reader.Read<AssetType>(buffer);

  BufferReader assetReader;

  switch (type) {
  case AssetType::text:
    assetBuffer.append(0);
    serverAssetManager.EmplaceText(name, assetReader.ReadString(assetBuffer));
    break;
  case AssetType::texture:
  {
    auto texture = std::make_shared<sf::Texture>();
    texture->loadFromMemory(assetBuffer.begin(), assetBuffer.size());
    serverAssetManager.EmplaceTexture(name, texture);
    break;
  }
  case AssetType::audio:
  {
    auto audio = std::make_shared<sf::SoundBuffer>();
    audio->loadFromMemory(assetBuffer.begin(), assetBuffer.size());
    serverAssetManager.EmplaceAudio(name, audio);
    break;
  }
  case AssetType::sfml_image:
  {
    auto width = assetReader.Read<uint16_t>(assetBuffer);
    auto height = assetReader.Read<uint16_t>(assetBuffer);

    sf::Image image;
    image.create(width, height, (const sf::Uint8*)assetBuffer.begin() + assetReader.GetOffset());

    auto texture = std::make_shared<sf::Texture>();
    texture->loadFromImage(image);
    serverAssetManager.EmplaceTexture(name, texture);
    break;
  }
  }

  assetBuffer.setCapacity(0);
}


void Overworld::OnlineArea::receiveMapSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  auto path = reader.ReadString(buffer);
  mapBuffer = GetText(path);

  // If we are still invalid after this, there's a problem
  if (mapBuffer.empty()) {
    Logger::Logf("Server sent empty map data");
  }

  LoadMap(mapBuffer);
}

void Overworld::OnlineArea::receiveNaviConnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  std::string name = reader.ReadString(buffer);
  std::string texturePath = reader.ReadString(buffer);
  std::string animationPath = reader.ReadString(buffer);
  float x = reader.Read<float>(buffer);
  float y = reader.Read<float>(buffer);
  float z = reader.Read<float>(buffer);
  bool warp_in = reader.Read<bool>(buffer);

  auto pos = sf::Vector2f(x, y);

  if (user == ticket) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) return;

  auto [pair, success] = onlinePlayers.emplace(user, new Overworld::OnlinePlayer{ user });

  if (!success) return;

  auto* onlinePlayer = pair->second;
  onlinePlayer->timestamp = CurrentTime::AsMilli();
  onlinePlayer->startBroadcastPos = pos;
  onlinePlayer->endBroadcastPos = pos;

  auto& actor = onlinePlayer->actor;
  actor.AddNode(&onlinePlayer->emoteNode);
  actor.Rename(name);
  actor.setPosition(pos);
  actor.setTexture(GetTexture(texturePath));

  Animation animation;
  animation.LoadWithData(GetText(animationPath));
  actor.LoadAnimations(animation);

  AddSprite(&actor, 0);

  auto& teleportController = onlinePlayer->teleportController;
  teleportController.EnableSound(false);
  AddSprite(&teleportController.GetBeam(), 0);

  if (warp_in) {
    teleportController.TeleportIn(actor, pos, Direction::none);
  }
}

void Overworld::OnlineArea::receiveNaviDisconnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto* actor = &userIter->second->actor;
    auto* teleport = &userIter->second->teleportController;
    teleport->TeleportOut(*actor).onFinish.Slot([=] {
      RemoveSprite(actor);
      RemoveSprite(&teleport->GetBeam());
      removePlayers.push_back(user);
    });
  }
}

void Overworld::OnlineArea::receiveNaviSetNameSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  std::string name = reader.ReadString(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    userIter->second->actor.Rename(name);
  }
}

void Overworld::OnlineArea::receiveNaviMoveSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  if (mapBuffer.empty()) return;

  std::string user = reader.ReadString(buffer);

  // ignore our ip update
  if (user == ticket) {
    return;
  }

  float x = reader.Read<float>(buffer);
  float y = reader.Read<float>(buffer);
  float z = reader.Read<float>(buffer);

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {

    // Calculcate the NEXT  frame and see if we're moving too far
    auto* onlinePlayer = userIter->second;
    auto currentTime = CurrentTime::AsMilli();
    auto newPos = sf::Vector2f(x, y);
    auto deltaTime = static_cast<double>(currentTime - onlinePlayer->timestamp) / 1000.0;
    auto delta = onlinePlayer->endBroadcastPos - newPos;
    float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
    double incomingLag = (currentTime - static_cast<double>(onlinePlayer->timestamp)) / 1000.0;

    // Adjust the lag time by the lag of this incoming frame
    double expectedTime = calculatePlayerLag(*onlinePlayer, incomingLag);

    Direction newHeading = Actor::MakeDirectionFromVector(delta, 0.01f);

    // Do not attempt to animate the teleport over quick movements if already teleporting
    if (onlinePlayer->teleportController.IsComplete() && onlinePlayer->packets > 1) {
      // we can't possibly have moved this far away without teleporting
      if (distance >= (onlinePlayer->actor.GetRunSpeed() * 2) * expectedTime) {
        onlinePlayer->actor.setPosition(onlinePlayer->endBroadcastPos);
        auto& action = onlinePlayer->teleportController.TeleportOut(onlinePlayer->actor);
        action.onFinish.Slot([=] {
          onlinePlayer->teleportController.TeleportIn(onlinePlayer->actor, onlinePlayer->endBroadcastPos, Direction::none);
        });
      }
    }

    // update our records
    onlinePlayer->startBroadcastPos = onlinePlayer->endBroadcastPos;
    onlinePlayer->endBroadcastPos = sf::Vector2f(x, y);
    auto previous = onlinePlayer->timestamp;
    onlinePlayer->timestamp = currentTime;
    onlinePlayer->packets++;
    onlinePlayer->lagWindow[onlinePlayer->packets % Overworld::LAG_WINDOW_LEN] = incomingLag;
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

  auto& player = userIter->second;;
  auto& actor = player->actor;

  actor.setTexture(GetTexture(texturePath));

  Animation animation;
  animation.LoadWithData(GetText(animationPath));
  actor.LoadAnimations(animation);
}

void Overworld::OnlineArea::receiveNaviEmoteSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  uint8_t emote = reader.Read<uint8_t>(buffer);
  std::string user = reader.ReadString(buffer);

  if (user == ticket) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto& player = userIter->second;
    player->emoteNode.Emote(static_cast<Overworld::Emotes>(emote));
  }
}

void Overworld::OnlineArea::leave() {
  using effect = segue<PixelateBlackWashFade>;
  getController().pop<effect>();
}

const bool Overworld::OnlineArea::isMouseHovering(const sf::RenderTarget& target, const SpriteProxyNode& src)
{
  auto mouse = target.mapPixelToCoords(sf::Mouse::getPosition(getController().getWindow()), GetCamera().GetView());

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
  if (path.rfind("/server", 0) == 0) {
    return serverAssetManager.GetText(path);
  }
  return Overworld::SceneBase::GetText(path);
}

std::shared_ptr<sf::Texture> Overworld::OnlineArea::GetTexture(const std::string& path) {
  if (path.rfind("/server", 0) == 0) {
    return serverAssetManager.GetTexture(path);
  }
  return Overworld::SceneBase::GetTexture(path);
}

std::shared_ptr<sf::SoundBuffer> Overworld::OnlineArea::GetAudio(const std::string& path) {
  if (path.rfind("/server", 0) == 0) {
    return serverAssetManager.GetAudio(path);
  }
  return Overworld::SceneBase::GetAudio(path);
}
