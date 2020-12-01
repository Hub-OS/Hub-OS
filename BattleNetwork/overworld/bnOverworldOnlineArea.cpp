#include <Swoosh/Segue.h>
#include <Swoosh/ActivityController.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/BlackWashFade.h>

#include "bnOverworldOnlineArea.h"
#include "../bnXmasBackground.h"
#include "../bnNaviRegistration.h"
#include "../netplay/bnNetPlayConfig.h"

using namespace swoosh::types;

Overworld::OnlineArea::OnlineArea(swoosh::ActivityController& controller, bool guestAccount) :
  SceneBase(controller, guestAccount)
{
  lastFrameNavi = this->GetCurrentNavi();

  int myPort = ENGINE.CommandLineValue<int>("port");
  Poco::Net::SocketAddress sa("127.0.0.1", myPort);
  client = Poco::Net::DatagramSocket(sa);
  client.setBlocking(false);

  int remotePort = ENGINE.CommandLineValue<int>("remotePort");
  std::string cyberworld = ENGINE.CommandLineValue<std::string>("cyberworld");
  remoteAddress = Poco::Net::SocketAddress(cyberworld, remotePort);
  client.connect(remoteAddress);

  SetBackground(new XmasBackground);
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
  if (mapBuffer.empty() == false) {
    SceneBase::onUpdate(elapsed);
    sendXYZSignal();

    for (auto player : onlinePlayers) {
      player.second->actor.Update(elapsed);
    }

    loadMapTime.reset();
    loadMapTime.pause();
  }
  else {
    loadMapTime.update(elapsed);

    if (loadMapTime.getElapsed() > sf::seconds(5)) {
      using effect = segue<PixelateBlackWashFade>;
      getController().pop<effect>();
    }
  }

  this->processIncomingPackets();
}

void Overworld::OnlineArea::onDraw(sf::RenderTexture& surface)
{
  if (mapBuffer.empty() == false) {
    SceneBase::onDraw(surface);
  }
}

void Overworld::OnlineArea::onStart()
{
  SceneBase::onStart();
  loadMapTime.start();
  sendLoginSignal();
}

const std::pair<bool, Overworld::Map::Tile**> Overworld::OnlineArea::FetchMapData()
{
  std::istringstream iss{ mapBuffer };
  return Map::LoadFromStream(GetMap(), iss);
}

void Overworld::OnlineArea::OnTileCollision(const Overworld::Map::Tile& tile)
{
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

void Overworld::OnlineArea::sendXYZSignal()
{
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
  /*Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::change };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&navi, sizeof(SelectedNavi));
  client.sendBytes(buffer.begin(), (int)buffer.size());*/
}

void Overworld::OnlineArea::sendLoginSignal()
{
  std::string username("Maverick\0", 9);
  std::string password("Nunya\0", 6); // No servers need passwords at this time

  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::login };
  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)username.data(), username.length());
  buffer.append((char*)password.data(), password.length());
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::sendLogoutSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::logout };
  buffer.append((char*)&type, sizeof(ClientEvents));
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::sendMapRefreshSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  ClientEvents type{ ClientEvents::loaded_map };
  size_t mapID{};

  buffer.append((char*)&type, sizeof(ClientEvents));
  buffer.append((char*)&mapID, sizeof(size_t));
  client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);
}

void Overworld::OnlineArea::recieveXYZSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;

  double x{}, y{}, z{};
  std::string user = std::string(buffer.begin(), buffer.size() - 16ll);
  std::memcpy(&x, buffer.begin(), sizeof(double));
  std::memcpy(&y, buffer.begin()+ sizeof(double),    sizeof(double));
  std::memcpy(&z, buffer.begin()+(sizeof(double)*2), sizeof(double));

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    auto* onlinePlayer = userIter->second;
    onlinePlayer->actor.setPosition(sf::Vector2f(x, y));
  }
}

void Overworld::OnlineArea::recieveNaviChangeSignal(const Poco::Buffer<char>& buffer) 
{
  if (!isConnected) return;

  SelectedNavi form{};
  std::memcpy(&form, buffer.begin(), sizeof(SelectedNavi));

  std::string user = std::string(buffer.begin() + sizeof(SelectedNavi), buffer.size() - sizeof(SelectedNavi));

  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    RefreshOnlinePlayerSprite(*userIter->second, form);
  }
}

void Overworld::OnlineArea::recieveLoginSignal(const Poco::Buffer<char>& buffer)
{
  // Ignore error codes for login signals
  std::string user = std::string(buffer.begin()+sizeof(uint16_t), buffer.size()-sizeof(uint16_t));

  if (user == this->client.address().toString()) {
    if (!isConnected) {
      sendMapRefreshSignal();
    }

    isConnected = true;
    return;
  }

  if (!isConnected) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter == onlinePlayers.end()) {
    auto [pair, success] = onlinePlayers.emplace(user, new Overworld::OnlinePlayer{ user });

    if (success) {
      GetMap().AddSprite(&(pair->second->actor), 0);
    }
  }
}

void Overworld::OnlineArea::recieveLogoutSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;

  std::string user = std::string(buffer.begin(), buffer.size());
  auto userIter = onlinePlayers.find(user);

  if (userIter != onlinePlayers.end()) {
    GetMap().RemoveSprite(&(userIter->second->actor));
    onlinePlayers.erase(userIter);
  }
}

void Overworld::OnlineArea::recieveMapSignal(const Poco::Buffer<char>& buffer)
{
  mapBuffer = std::string(buffer.begin(), buffer.size());

  // If we are still invalid after this, there's a problem
  if (mapBuffer.empty()) {
    Logger::Logf("Server sent empty map data");
  }
}

void Overworld::OnlineArea::processIncomingPackets()
{
  if (!client.available()) return;

  static int errorCount = 0;

  if (errorCount > 10) {
    using effect = segue<PixelateBlackWashFade>;
    getController().pop<effect>();
    errorCount = 0; // reset for next instance of scene
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
      /*case ServerEvents::change:
        recieveNaviChangeSignal(data);
        break;*/
      case ServerEvents::hologram_xyz:
        recieveXYZSignal(data);
        break;
      case ServerEvents::login:
        recieveLoginSignal(data);
        break;
      /*case ServerEvents::logout
        recieveLogoutSignal(data);
        break;*/
      case ServerEvents::map:
        recieveMapSignal(data);
        break;
      }
    }

    errorCount = 0;
  }
  catch (std::exception& e) {
    Logger::Logf("OnlineArea Network exception: %s", e.what());

    if (isConnected) {
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
  }
}