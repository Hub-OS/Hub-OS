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
  SceneBase(controller, guestAccount)
{
  lastFrameNavi = this->GetCurrentNavi();

  int myPort = getController().CommandLineValue<int>("port");
  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), myPort);
  client = Poco::Net::DatagramSocket(sa);
  client.setBlocking(false);

  int remotePort = getController().CommandLineValue<int>("remotePort");
  std::string cyberworld = getController().CommandLineValue<std::string>("cyberworld");
  remoteAddress = Poco::Net::SocketAddress(cyberworld, remotePort);

  try {
    client.connect(remoteAddress);
  }
  catch (Poco::Net::NetException& e) {
    Logger::Log(e.message());
    errorCount = MAX_ERROR_COUNT+1;
  }

  SetBackground(new XmasBackground);

  loadMapTime.reverse(true);
  loadMapTime.set(sf::seconds(MAX_TIMEOUT_SECONDS));
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
      sendNaviChangeSignal(lastFrameNavi);
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

    loadMapTime.reset();
    loadMapTime.pause();

    movementTimer.update(sf::seconds(static_cast<float>(elapsed)));

    if (movementTimer.getElapsed().asSeconds() > SECONDS_PER_MOVEMENT) {
      movementTimer.reset();
      sendXYZSignal();
    }
  }
  else {
    loadMapTime.update(sf::seconds(static_cast<float>(elapsed)));

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
    name.SetString("Connecting " + trimmed + "s...");
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
  loadMapTime.start();
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
  if (errorCount > MAX_ERROR_COUNT) return;

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

  std::string username("James\0", 5);
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
  if (mapBuffer.empty()) return;

  std::string user = std::string(buffer.begin());

  // ignore our ip update
  if (user == ticket) {
    return;
  }

  double xd{}, yd{}, zd{};
  std::memcpy(&xd, buffer.begin()+user.size(), sizeof(double));
  std::memcpy(&yd, buffer.begin()+user.size()+sizeof(double),    sizeof(double));
  std::memcpy(&zd, buffer.begin()+user.size()+(sizeof(double)*2), sizeof(double));

  float x = static_cast<float>(xd);
  float y = static_cast<float>(yd);
  float z = static_cast<float>(zd);

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
    if(onlinePlayer->teleportController.IsComplete() && onlinePlayer->packets > 1) {
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
    onlinePlayer->lagWindow[onlinePlayer->packets%Overworld::LAG_WINDOW_LEN] = incomingLag;
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
      GetMap().AddSprite(&pair->second->teleportController.GetBeam(), 0);
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

  if (user == ticket) return;

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

    // Ignore error codes for login signals
    this->ticket = std::string(buffer.begin() + sizeof(uint16_t), buffer.size() - sizeof(uint16_t));
  }

  return;
}

void Overworld::OnlineArea::recieveAvatarJoinSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;
  if (mapBuffer.empty()) return;

  std::string user = std::string(buffer.begin(), buffer.size());

  if (user == ticket) return;

  auto userIter = onlinePlayers.find(user);

  if (userIter == onlinePlayers.end()) {

    auto [pair, success] = onlinePlayers.emplace(user, new Overworld::OnlinePlayer{ user });

    if (success) {
      auto token = GetMap().FindToken("H")[0];
      auto* onlinePlayer = pair->second;

      auto& actor = onlinePlayer->actor;
      auto& teleport = onlinePlayer->teleportController;
      onlinePlayer->actor.AddNode(&onlinePlayer->emoteNode);
      onlinePlayer->timestamp = CurrentTime::AsMilli();
      onlinePlayer->startBroadcastPos = sf::Vector2f(token.x, token.y); // TODO emit (x,y) from server itself
      onlinePlayer->endBroadcastPos = sf::Vector2f(token.x, token.y); // TODO emit (x,y) from server itself
      RefreshOnlinePlayerSprite(*onlinePlayer, SelectedNavi{ 0 });
      actor.setPosition(onlinePlayer->endBroadcastPos);
      actor.Hide();
      GetMap().AddSprite(&actor, 0);
      GetMap().AddSprite(&teleport.GetBeam(), 0);

      onlinePlayer->actor.setPosition(onlinePlayer->startBroadcastPos);
      onlinePlayer->teleportController.TeleportIn(onlinePlayer->actor, onlinePlayer->endBroadcastPos, Direction::none);
      onlinePlayer->teleportController.EnableSound(false);
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
  if (errorCount > MAX_ERROR_COUNT) return;

  mapBuffer = std::string(buffer.begin(), buffer.size());

  // If we are still invalid after this, there's a problem
  if (mapBuffer.empty()) {
    Logger::Logf("Server sent empty map data");
  }
  else {
    sendLoginSignal();
  }
}

void Overworld::OnlineArea::recieveEmoteSignal(const Poco::Buffer<char>& buffer)
{
  if (!isConnected) return;
  if (errorCount > MAX_ERROR_COUNT) return;

  uint8_t emote{};
  std::memcpy(&emote, buffer.begin(), sizeof(uint8_t));
  std::string user = std::string(buffer.begin() + sizeof(uint8_t), buffer.size() - sizeof(uint8_t));

  if (user == ticket) return;

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
    read += client.receiveFrom(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN, sender);

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
    Logger::Logf("OnlineArea Network exception: %s", e.message().c_str());

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
