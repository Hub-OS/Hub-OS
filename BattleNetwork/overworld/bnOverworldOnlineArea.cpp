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
constexpr float MAX_TIMEOUT_SECONDS = 5.f;

Overworld::OnlineArea::OnlineArea(swoosh::ActivityController& controller, bool guestAccount) :
  SceneBase(controller, guestAccount)
{
  lastFrameNavi = this->GetCurrentNavi();

  int myPort = ENGINE.CommandLineValue<int>("port");
  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), myPort);
  client = Poco::Net::DatagramSocket(sa);
  client.setBlocking(false);

  int remotePort = ENGINE.CommandLineValue<int>("remotePort");
  std::string cyberworld = ENGINE.CommandLineValue<std::string>("cyberworld");
  remoteAddress = Poco::Net::SocketAddress(cyberworld, remotePort);

  try {
    client.connect(remoteAddress);
  }
  catch (Poco::Net::NetException& e) {
    Logger::Log(e.message());
    errorCount = MAX_ERROR_COUNT+1;
  }

  SetBackground(new XmasBackground);

  // load name font stuff
  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthin_regular.ttf");
  name = sf::Text("", *font);

  loadMapTime.reverse(true);
  loadMapTime.set(MAX_TIMEOUT_SECONDS*1000);
}

Overworld::OnlineArea::~OnlineArea()
{
  auto& map = GetMap();

  for (auto player : onlinePlayers) {
    map.RemoveSprite(&(player.second->actor));
    delete player.second;
  }

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
      sendNaviChangeSignal(lastFrameNavi);
    }

    for (auto player : onlinePlayers) {
      player.second->teleportController.Update(elapsed);
      auto& actor = player.second->actor;
      auto deltaTime = static_cast<double>(currentTime - player.second->timestamp)/1000.0;
      auto delta = player.second->endBroadcastPos - player.second->startBroadcastPos;
      float distance = std::sqrt(std::pow(delta.x, 2.0f) + std::pow(delta.y, 2.0f));
      double expectedTime = (player.second->avgLagTime / static_cast<double>(player.second->packets + 1));
      auto alpha = ease::linear(deltaTime, expectedTime, 1.0);

      auto newHeading = Actor::MakeDirectionFromVector(delta, 0.01f);

      if (distance <= 0.2f) {
        actor.Face(actor.GetHeading());
      } else if (distance <= actor.GetWalkSpeed()*expectedTime) {
        actor.Walk(newHeading, false); // Don't actually move or collide, but animate
      } else {
        actor.Run(newHeading, false);
      }

      auto newPos = player.second->startBroadcastPos + sf::Vector2f(delta.x*alpha, delta.y*alpha);
      //auto newPos = sf::Vector2f{ ease::interpolate((float)0.5f, lastBroadcastPos.x, lastPosition.x),
      //               ease::interpolate((float)0.5f, lastBroadcastPos.y, lastPosition.y) };
      //auto newPos = lastPosition + sf::Vector2f(delta.x * elapsed, delta.y * elapsed);

      actor.setPosition(newPos);
      actor.Update(elapsed);

      player.second->emoteNode.Update(elapsed);
    }

    for (auto remove : removePlayers) {
      onlinePlayers.erase(remove);
    }

    removePlayers.clear();

    loadMapTime.reset();
    loadMapTime.pause();

    movementTimer.update(elapsed);

    if (movementTimer.getElapsed().asSeconds() > SECONDS_PER_MOVEMENT) {
      movementTimer.reset();
      sendXYZSignal();
    }
  }
  else {
    loadMapTime.update(elapsed);

    if (loadMapTime.getElapsed().asSeconds() == 0) {
      using effect = segue<PixelateBlackWashFade>;
      getController().pop<effect>();
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

    name.setPosition(view.x*0.5f, view.y*0.5f);
    std::string secondsStr = std::to_string(loadMapTime.getElapsed().asSeconds());
    std::string trimmed = secondsStr.substr(0, secondsStr.find(".") + precision + 1);
    name.setString(sf::String("Connecting " + trimmed + "s..."));
    name.setOrigin(name.getLocalBounds().width * 0.5f, name.getLocalBounds().height * 0.5f);
    ENGINE.Draw(name);
  }

  for (auto player : onlinePlayers) {
    if (IsMouseHovering(surface, player.second->actor)) {
      std::string nameStr = player.second->actor.GetName();
      auto mousei = sf::Mouse::getPosition(*ENGINE.GetWindow());
      auto mousef = sf::Vector2f(mousei.x, mousei.y);
      name.setPosition(mousef);
      name.setString(sf::String(nameStr.c_str()));
      name.setOrigin(-10.0f, 0);
      ENGINE.Draw(name);
      continue;
    }
  }
}

void Overworld::OnlineArea::onStart()
{
  SceneBase::onStart();
  loadMapTime.start();
  movementTimer.start();
  sendLoginSignal();
}

const std::pair<bool, Overworld::Map::Tile**> Overworld::OnlineArea::FetchMapData()
{
  std::istringstream iss{ mapBuffer };
  return Map::LoadFromStream(GetMap(), iss);
}

void Overworld::OnlineArea::OnTileCollision(const Overworld::Map::Tile& tile)
{
  if (errorCount > MAX_ERROR_COUNT) return;

  // on collision with warps
  static size_t next_warp = 0;
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

void Overworld::OnlineArea::sendXYZSignal()
{
  if (errorCount > MAX_ERROR_COUNT) return;

  auto vec = GetPlayer().getPosition();
  double x = static_cast<double>(vec.x);
  double y = static_cast<double>(vec.y);
  double z = 0;

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::user_xyz };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&x, sizeof(double));
  buffer.append((char*)&y, sizeof(double));
  buffer.append((char*)&z, sizeof(double));
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::sendNaviChangeSignal(const SelectedNavi& navi)
{
  if (errorCount > MAX_ERROR_COUNT) return;

  Poco::Buffer<char> buffer{ 0 };
  uint16_t form = navi;
  ClientEvents type{ ClientEvents::avatar_change };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&form, sizeof(uint16_t));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void Overworld::OnlineArea::sendLoginSignal()
{
  if (errorCount > MAX_ERROR_COUNT) return;

  std::string username("James\0", 9);
  std::string password("\0", 1); // No servers need passwords at this time

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::login };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)username.data(), username.length());
  buffer.append((char*)password.data(), password.length());
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::sendLogoutSignal()
{
  if (errorCount > MAX_ERROR_COUNT) return;

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::logout };
  buffer.append((char*)&type, sizeof(ClientEvents));
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::sendMapRefreshSignal()
{
  if (errorCount > MAX_ERROR_COUNT) return;

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::loaded_map };
  size_t mapID{};

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&mapID, sizeof(size_t));
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::sendEmoteSignal(const Overworld::Emotes emote)
{
  if (errorCount > MAX_ERROR_COUNT) return;

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::emote };
  uint8_t val = static_cast<uint8_t>(emote);

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&val, sizeof(uint8_t));
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::recieveXYZSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  std::string user = std::string(buffer.begin());

  // ignore our ip update
  if (user == this->client.address().toString()) {
    return;
  }

  double x{}, y{}, z{};
  std::memcpy(&x, buffer.begin()+user.size(), sizeof(double));
  std::memcpy(&y, buffer.begin()+user.size()+sizeof(double),    sizeof(double));
  std::memcpy(&z, buffer.begin()+user.size()+(sizeof(double)*2), sizeof(double));

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto* onlinePlayer = userIter->second;
    onlinePlayer->startBroadcastPos = onlinePlayer->endBroadcastPos;
    onlinePlayer->endBroadcastPos = sf::Vector2f(x,y);
    auto previous = onlinePlayer->timestamp;
    onlinePlayer->timestamp = CurrentTime::AsMilli();
    onlinePlayer->packets++;
    onlinePlayer->avgLagTime = onlinePlayer->avgLagTime + (static_cast<double>(onlinePlayer->timestamp - previous) / 1000.0);
  }
  else {
    auto [pair, success] = onlinePlayers.emplace(user, new Overworld::OnlinePlayer{ user });

    if (success) {
      auto& actor = pair->second->actor;
      pair->second->actor.AddNode(&pair->second->emoteNode);
      pair->second->timestamp = CurrentTime::AsMilli();
      pair->second->startBroadcastPos = sf::Vector2f(x, y);
      pair->second->endBroadcastPos = sf::Vector2f(x, y);
      RefreshOnlinePlayerSprite(*pair->second, SelectedNavi{ 0 });
      actor.setPosition(pair->second->endBroadcastPos);
      GetMap().AddSprite(&actor, 0);
    }
  }
}

void Overworld::OnlineArea::recieveNameSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  SelectedNavi form{};
  std::string user = std::string(buffer.begin(), buffer.size());
  std::string name = std::string(buffer.begin() + user.size(), buffer.size() - user.size());

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    userIter->second->actor.Rename(name);
  }
}

void Overworld::OnlineArea::recieveNaviChangeSignal(const Poco::Buffer<char>& buffer) 
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  uint16_t form{};
  std::memcpy(&form, buffer.begin(), sizeof(uint16_t));
  std::string user = std::string(buffer.begin()+sizeof(uint16_t), buffer.size()-sizeof(uint16_t));

  if (user == client.address().toString()) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    RefreshOnlinePlayerSprite(*userIter->second, form);
  }
}

void Overworld::OnlineArea::recieveLoginSignal(const Poco::Buffer<char>& buffer)
{
  if (errorCount > MAX_ERROR_COUNT) return;

  if (!isConnected) {
    sendMapRefreshSignal();
    sendNaviChangeSignal(GetCurrentNavi());
    isConnected = true;
  }

  return;
}

void Overworld::OnlineArea::recieveAvatarJoinSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  // Ignore error codes for login signals
  std::string user = std::string(buffer.begin() + sizeof(uint16_t), buffer.size() - sizeof(uint16_t));

  auto userIter = onlinePlayers.find(user);

  if (userIter == onlinePlayers.end()) {

    auto [pair, success] = onlinePlayers.emplace(user, new Overworld::OnlinePlayer{ user });

    if (success) {
      auto& actor = pair->second->actor;
      auto& teleport = pair->second->teleportController;
      pair->second->actor.AddNode(&pair->second->emoteNode);
      pair->second->timestamp = CurrentTime::AsMilli();
      pair->second->startBroadcastPos = sf::Vector2f(0, 0); // TODO emit (x,y) from server itself
      pair->second->endBroadcastPos = sf::Vector2f(0, 0); // TODO emit (x,y) from server itself
      RefreshOnlinePlayerSprite(*pair->second, SelectedNavi{ 0 });
      actor.setPosition(pair->second->endBroadcastPos);
      GetMap().AddSprite(&actor, 0);
      GetMap().AddSprite(&teleport.GetBeam(), 0);
    }
  }
}

void Overworld::OnlineArea::recieveLogoutSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  std::string user = std::string(buffer.begin(), buffer.size());
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

void Overworld::OnlineArea::recieveMapSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  mapBuffer = std::string(buffer.begin(), buffer.size());

  // If we are still invalid after this, there's a problem
  if (mapBuffer.empty()) {
    Logger::Logf("Server sent empty map data");
  }
}

void Overworld::OnlineArea::recieveEmoteSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  uint8_t emote{};
  std::memcpy(&emote, buffer.begin(), sizeof(uint8_t));
  std::string user = std::string(buffer.begin() + sizeof(uint8_t), buffer.size() - sizeof(uint8_t));

  if (user == client.address().toString()) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto& player = userIter->second;
    player->emoteNode.Emote(static_cast<Overworld::Emotes>(emote));
  }
}

void Overworld::OnlineArea::processIncomingPackets()
{
  if (!client.available()) return;

  auto* teleportController = &GetTeleportControler();
  if (errorCount > MAX_ERROR_COUNT && teleportController->IsComplete()) {
    auto& map = GetMap();
    auto* playerController = &GetPlayerController();
    auto* playerActor = &GetPlayer();

    playerController->ReleaseActor();
    auto& command = teleportController->TeleportOut(*playerActor);

    auto teleportHome = [=] {
      TeleportUponReturn(playerActor->getPosition());
      getController().pop<segue<PixelateBlackWashFade>>();
    };

    command.onFinish.Slot(teleportHome);
    return;
  }

  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };
  static int read = 0;

  try {
    Poco::Net::SocketAddress sender;
    read += client.receiveFrom(rawBuffer, 4048u, sender);

    if (sender == remoteAddress && read > 0) {
      rawBuffer[read] = '\0';

      ServerEvents sig = *(ServerEvents*)rawBuffer;
      size_t sigLen = sizeof(ServerEvents);
      Poco::Buffer<char> data{ 0 };
      data.append(rawBuffer + sigLen, size_t(read) - sigLen);

      switch (sig) {
      case ServerEvents::avatar_change:
        recieveNaviChangeSignal(data);
        break;
      case ServerEvents::hologram_xyz:
        recieveXYZSignal(data);
        break;
      case ServerEvents::login:
        recieveLoginSignal(data);
        break;
      case ServerEvents::logout:
        recieveLogoutSignal(data);
        break;
      case ServerEvents::map:
        recieveMapSignal(data);
        break;
      case ServerEvents::emote:
        recieveEmoteSignal(data);
        break;
      case ServerEvents::avatar_join:
        recieveAvatarJoinSignal(data);
        break;
      }
    }

    errorCount = 0;
  }
  catch (Poco::Net::NetException& e) {
    Logger::Logf("OnlineArea Network exception: %s", e.message());

    // Assume we've connected fine, what other errors continue to occur?
    if (isConnected && mapBuffer.size()) {
      errorCount++;
    }
  }

  read = 0;
  std::memset(rawBuffer, 0, NetPlayConfig::MAX_BUFFER_LEN);
}

void Overworld::OnlineArea::RefreshOnlinePlayerSprite(OnlinePlayer& player, SelectedNavi navi)
{
  if (player.currNavi == navi) return;

  auto & meta = NAVIS.At(navi);
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
  sf::Vector2f world = target.mapPixelToCoords(sf::Mouse::getPosition(*ENGINE.GetWindow()), GetCamera().GetView());

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
