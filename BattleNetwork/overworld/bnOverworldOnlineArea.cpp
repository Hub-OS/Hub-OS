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
  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), myPort);
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

    for (auto player : onlinePlayers) {
      player.second->actor.Update(elapsed);
    }

    loadMapTime.reset();
    loadMapTime.pause();
  }
  else {
    loadMapTime.update(elapsed);
  }

  this->processIncomingPackets();

  if (loadMapTime.getElapsed() > sf::seconds(5)) {
    using effect = segue<PixelateBlackWashFade>;
    getController().pop<effect>();
  }
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
  OnlineSignals type{ OnlineSignals::xyz };
  buffer.append((char*)&type, sizeof(OnlineSignals));
  buffer.append((char*)&x, sizeof(double));
  buffer.append((char*)&y, sizeof(double));
  buffer.append((char*)&z, sizeof(double));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void Overworld::OnlineArea::sendNaviChangeSignal(const SelectedNavi& navi)
{
  Poco::Buffer<char> buffer{ 0 };
  OnlineSignals type{ OnlineSignals::change };
  buffer.append((char*)&type, sizeof(OnlineSignals));
  buffer.append((char*)&navi, sizeof(SelectedNavi));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void Overworld::OnlineArea::sendLoginSignal()
{
  std::string username = WEBCLIENT.GetUserName() + "\0";
  std::string password = "\0"; // No servers need passwords at this time

  Poco::Buffer<char> buffer{ 0 };
  OnlineSignals type{ OnlineSignals::login };
  buffer.append((char*)&type, sizeof(OnlineSignals));
  buffer.append((char*)username.data(), username.length());
  buffer.append((char*)password.data(), password.length());
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void Overworld::OnlineArea::sendLogoutSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  OnlineSignals type{ OnlineSignals::logout };
  buffer.append((char*)&type, sizeof(OnlineSignals));
  client.sendBytes(buffer.begin(), (int)buffer.size());
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
    isConnected = true;
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
}

void Overworld::OnlineArea::processIncomingPackets()
{
  if (!client.poll(Poco::Timespan{ 0 }, Poco::Net::Socket::SELECT_READ)) return;

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
    read += client.receiveBytes(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN - 1);
    if (read > 0) {
      rawBuffer[read] = '\0';

      OnlineSignals sig = *(OnlineSignals*)rawBuffer;
      size_t sigLen = sizeof(OnlineSignals);
      Poco::Buffer<char> data{ 0 };
      data.append(rawBuffer + sigLen, size_t(read) - sigLen);

      switch (sig) {
      case OnlineSignals::change:
        recieveXYZSignal(data);
        break;
      case OnlineSignals::xyz:
        recieveXYZSignal(data);
        break;
      case OnlineSignals::login:
        recieveLoginSignal(data);
        break;
      case OnlineSignals::logout:
        recieveLogoutSignal(data);
        break;
      case OnlineSignals::map:
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