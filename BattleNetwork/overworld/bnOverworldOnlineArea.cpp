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
constexpr int MAX_ERROR_COUNT = 10;
constexpr float SECONDS_PER_MOVEMENT = 1.f / 5.f;
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
    map.RemoveSprite(&(player.second->actor));
    delete player.second;
  }

  sendLogoutSignal();

  onlinePlayers.clear();
}

void Overworld::OnlineArea::onUpdate(double elapsed)
{
  this->processIncomingPackets();

  auto currentTime = CurrentTime::AsMilli();

  if (mapBuffer.empty() == false) {
    SceneBase::onUpdate(elapsed);

    if (lastFrameNavi != GetCurrentNavi()) {
      lastFrameNavi = GetCurrentNavi();
      sendAvatarChangeSignal(lastFrameNavi);
    }

    for (auto player : onlinePlayers) {
      auto* onlinePlayer = player.second;
      auto& actor = player.second->actor;

      if (onlinePlayer->teleportController.IsComplete()) {
        auto deltaTime = static_cast<double>(currentTime - onlinePlayer->timestamp) / 1000.0;
        auto delta = player.second->endBroadcastPos - player.second->startBroadcastPos;
        float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
        double expectedTime = CalculatePlayerLag(*onlinePlayer);
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
    if (IsMouseHovering(surface, player.second->actor)) {
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

const std::pair<bool, Overworld::Map::Tile**> Overworld::OnlineArea::FetchMapData()
{
  std::istringstream iss{ mapBuffer };
  return Map::LoadFromStream(GetMap(), iss);
}

// NOTE: these are super hacky to make it look like maps have more going on for them (e.g. connected warps...)
// TODO: replace
void Overworld::OnlineArea::OnTileCollision(const Overworld::Map::Tile& tile)
{
  // on collision with warps
  static size_t next_warp = 0, next_warp_2 = 1;
  auto* teleportController = &GetTeleportControler();

  if (tile.token == "W" && teleportController->IsComplete()) {
    auto& map = GetMap();
    auto* playerController = &GetPlayerController();
    auto* playerActor = &GetPlayer();

    auto warps = map.FindToken("W");

    if (warps.size()) {
      if (++next_warp >= warps.size()) {
        next_warp = 0;
      }

      auto teleportToNextWarp = [=] {
        auto finishTeleport = [=] {
          playerController->ControlActor(*playerActor);
        };

        auto& command = teleportController->TeleportIn(*playerActor, warps[next_warp], Direction::up);
        command.onFinish.Slot(finishTeleport);
      };

      playerController->ReleaseActor();
      auto& command = teleportController->TeleportOut(*playerActor);
      command.onFinish.Slot(teleportToNextWarp);
    }
  }

  if (tile.token == "W2" && teleportController->IsComplete()) {
    auto& map = GetMap();
    auto* playerController = &GetPlayerController();
    auto* playerActor = &GetPlayer();

    auto warps = map.FindToken("W2");

    if (warps.size()) {
      if (++next_warp_2 >= warps.size()) {
        next_warp_2 = 0;
      }

      auto teleportToNextWarp = [=] {
        auto finishTeleport = [=] {
          playerController->ControlActor(*playerActor);
        };

        auto& command = teleportController->TeleportIn(*playerActor, warps[next_warp_2], Direction::up);
        command.onFinish.Slot(finishTeleport);
      };

      playerController->ReleaseActor();
      auto& command = teleportController->TeleportOut(*playerActor);
      command.onFinish.Slot(teleportToNextWarp);
    }
  }

  // on collision with homepage warps
  if (tile.token == "H" && teleportController->IsComplete()) {
    auto& map = GetMap();
    auto* playerController = &GetPlayerController();
    auto* playerActor = &GetPlayer();

    playerController->ReleaseActor();
    auto& command = teleportController->TeleportOut(*playerActor);

    auto teleportHome = [=] {
      TeleportUponReturn(playerActor->getPosition());
      sendLogoutSignal();
      getController().pop<segue<BlackWashFade>>();
    };

    command.onFinish.Slot(teleportHome);
  }
}

void Overworld::OnlineArea::OnEmoteSelected(Overworld::Emotes emote)
{
  SceneBase::OnEmoteSelected(emote);
  sendEmoteSignal(emote);
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
  buffer.append((char*)&GetCurrentNavi(), sizeof(uint16_t));
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

void Overworld::OnlineArea::sendAvatarChangeSignal(const SelectedNavi& navi)
{
  Poco::Buffer<char> buffer{ 0 };
  uint16_t form = navi;
  ClientEvents type{ ClientEvents::avatar_change };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&form, sizeof(uint16_t));
  packetShipper.Send(client, Reliability::Reliable, buffer);
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

void Overworld::OnlineArea::receiveMapSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  mapBuffer = reader.ReadString(buffer);

  // If we are still invalid after this, there's a problem
  if (mapBuffer.empty()) {
    Logger::Logf("Server sent empty map data");
  }
}

void Overworld::OnlineArea::receiveNaviConnectedSignal(BufferReader& reader, const Poco::Buffer<char>& buffer)
{
  std::string user = reader.ReadString(buffer);
  std::string name = reader.ReadString(buffer);
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
  RefreshOnlinePlayerSprite(*onlinePlayer, SelectedNavi{ 0 });
  GetMap().AddSprite(&actor, 0);

  auto& teleportController = onlinePlayer->teleportController;
  teleportController.EnableSound(false);
  GetMap().AddSprite(&teleportController.GetBeam(), 0);

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
      GetMap().RemoveSprite(actor);
      GetMap().RemoveSprite(&teleport->GetBeam());
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
    double expectedTime = CalculatePlayerLag(*onlinePlayer, incomingLag);

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
  uint16_t form = reader.Read<uint16_t>(buffer);
  std::string user = reader.ReadString(buffer);

  if (user == ticket) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    RefreshOnlinePlayerSprite(*userIter->second, form);
  }
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

void Overworld::OnlineArea::processIncomingPackets()
{
  auto timeDifference = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - packetSorter.GetLastMessageTime()
    );

  if (timeDifference.count() > 5) {
    Leave();
    return;
  }

  if (!client.available()) return;

  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };

  try {
    packetShipper.ResendBackedUpPackets(client);

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

    Leave();
  }
}

void Overworld::OnlineArea::Leave() {
  using effect = segue<PixelateBlackWashFade>;
  getController().pop<effect>();
}

void Overworld::OnlineArea::RefreshOnlinePlayerSprite(OnlinePlayer& player, SelectedNavi navi)
{
  if (player.currNavi == navi) return;

  auto& meta = NAVIS.At(navi);
  auto owPath = meta.GetOverworldAnimationPath();

  if (owPath.size()) {
    player.actor.setTexture(meta.GetOverworldTexture());
    player.actor.LoadAnimations(owPath);
    player.currNavi = navi;

    // move the emote above the player's head
    float h = player.actor.getLocalBounds().height;
    float y = h - (h - player.actor.getOrigin().y);
    player.emoteNode.setPosition(sf::Vector2f(0, -y));
  }
}

const bool Overworld::OnlineArea::IsMouseHovering(const sf::RenderTarget& target, const SpriteProxyNode& src)
{

  // convert it to world coordinates
  sf::Vector2f world = target.mapPixelToCoords(sf::Mouse::getPosition(getController().getWindow()), GetCamera().GetView());

  // consider the point on screen relative to the camera focus
  //auto mouse = GetMap().WorldToScreen(world);

  auto mouse = world;

  sf::FloatRect bounds = src.getSprite().getLocalBounds();
  bounds.left += src.getPosition().x - GetCamera().GetView().getCenter().x;
  bounds.top += src.getPosition().y - GetCamera().GetView().getCenter().y;
  bounds.left *= GetMap().getScale().x;
  bounds.top *= GetMap().getScale().y;;
  bounds.width = bounds.width * GetMap().getScale().x;
  bounds.height = bounds.height * GetMap().getScale().y;

  // Logger::Logf("mouse: {%f, %f} ... bounds: {%f, %f, %f, %f}", mouse.x, mouse.y, bounds.left, bounds.top, bounds.width, bounds.height);

  return (mouse.x >= bounds.left && mouse.x <= bounds.left + bounds.width && mouse.y >= bounds.top && mouse.y <= bounds.top + bounds.height);
}

const double Overworld::OnlineArea::CalculatePlayerLag(OnlinePlayer& player, double nextLag)
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
