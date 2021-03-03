#include <Poco/Net/NetException.h>
#include <Segues/BlackWashFade.h>
#include "bnOverworldHomepage.h"
#include "bnOverworldOnlineArea.h"
#include "../netplay/bnNetPlayConfig.h"

using namespace swoosh::types;

constexpr sf::Int32 PING_SERVER_MILI = 5;

Overworld::Homepage::Homepage(swoosh::ActivityController& controller, bool guestAccount) :
  guest(guestAccount),
  SceneBase(controller, guestAccount)
{
  pingServerTimer.reverse(true);
  pingServerTimer.set(sf::milliseconds(PING_SERVER_MILI));
  pingServerTimer.start();


  LoadMap(FileUtil::Read("resources/ow/maps/homepage.tmx"));

  auto& map = GetMap();
  sf::Vector2f spawnPos;

  for (auto& tileObject : map.GetLayer(0).GetTileObjects()) {
    if (tileObject.name == "Home Warp") {
      spawnPos = tileObject.position + map.OrthoToIsometric(sf::Vector2f(0, tileObject.size.y / 2.0f));
    }
    else if (tileObject.name == "Net Warp") {
      auto centerPos = tileObject.position + map.OrthoToIsometric(sf::Vector2f(0, tileObject.size.y / 2.0f));
      auto tileSize = map.GetTileSize();

      netWarpTilePos = sf::Vector2f(
        std::floor(centerPos.x / (tileSize.x / 2)),
        std::floor(centerPos.y / tileSize.y)
      );
      netWarpObjectId = tileObject.id;
    }
  }

  auto& command = GetTeleportController().TeleportIn(GetPlayer(), spawnPos, Direction::up);
  command.onFinish.Slot([=] {
    GetPlayerController().ControlActor(GetPlayer());
  });
}

void Overworld::Homepage::PingRemoteAreaServer()
{
  if (pingServerTimer.getElapsed().asMilliseconds() == 0) {
    auto doSendThunk = [=] {
      Poco::Buffer<char> buffer{ 0 };
      buffer.append((char)Reliability::Unreliable);

      auto clientEvent = ClientEvents::ping;
      buffer.append((char*)&clientEvent, sizeof(uint16_t));

      try {
        client.sendTo(buffer.begin(), (int)buffer.size(), remoteAddress);

        if (!client.available()) return;

        char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };

        Poco::Net::SocketAddress sender;
        int read = client.receiveFrom(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN, sender);

        if (sender != remoteAddress || read == 0) return;

        Poco::Buffer<char> packet{ 0 };
        packet.append(rawBuffer, size_t(read));

        BufferReader reader;
        reader.Skip(1);
        auto sig = reader.Read<ServerEvents>(packet);
        auto version = reader.ReadString(packet);
        auto iteration = reader.Read<uint64_t>(packet);
        maxPayloadSize = reader.Read<uint16_t>(packet);

        if (sig == ServerEvents::pong && version == VERSION_ID && iteration == VERSION_ITERATION) {
          EnableNetWarps(true);
          isConnected = true;
        }
      }
      catch (Poco::Net::NetException& e) {
        Logger::Logf("Homepage warp could not request ping: %s", e.what());
        isConnected = false;
        reconnecting = false;
        client.close();
        EnableNetWarps(false);
      }
    };

    if (!reconnecting) {
      int myPort = getController().CommandLineValue<int>("port");
      Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), myPort);
      client = Poco::Net::DatagramSocket(sa);
      client.setBlocking(false);

      int remotePort = getController().CommandLineValue<int>("remotePort");
      std::string cyberworld = getController().CommandLineValue<std::string>("cyberworld");

      try {
        remoteAddress = Poco::Net::SocketAddress(cyberworld, remotePort);
        client.connect(remoteAddress);
        reconnecting = true;
        doSendThunk();
      }
      catch (Poco::Net::NetException& e) {
        reconnecting = false;
        Logger::Logf("Error trying to connect to remote address: %s", e.what());
      }
    }
    else {
      doSendThunk();
    }

    pingServerTimer.set(sf::milliseconds(PING_SERVER_MILI));
  }
}

void Overworld::Homepage::EnableNetWarps(bool enabled) {
  auto& map = GetMap();

  auto netWarpOptional = map.GetLayer(0).GetTileObject(netWarpObjectId);

  if(!netWarpOptional) {
    return;
  }

  auto& netWarp = netWarpOptional->get();
  auto tileset = map.GetTileset("warp");

  netWarp.tile.gid = tileset->firstGid + (enabled ? 1 : 0);
}

void Overworld::Homepage::onUpdate(double elapsed)
{
  if (infocus)
  {
    pingServerTimer.update(sf::seconds(static_cast<float>(elapsed)));
    PingRemoteAreaServer();
  }

  // Update our logic
  auto& map = GetMap();
  auto& window = getController().getWindow();
  auto mousei = sf::Mouse::getPosition(window);
  const auto& [row, col] = PixelToRowCol(mousei, window);
  sf::Vector2f click = { (float)col * map.GetTileSize().x, (float)row * map.GetTileSize().y };

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Home)) {
    map.setScale(2.f, 2.f);
  }
  else if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) && !scaledmap) {
    map.setScale(map.getScale() * 1.25f);
    scaledmap = true;
  }
  else if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) && !scaledmap) {
    map.setScale(map.getScale() * 0.75f);
    scaledmap = true;
  }

  if (scaledmap
    && !sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp)
    && !sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown)) {
    scaledmap = false;
  }

  /*if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !clicked) {
    const size_t tilesetCount = map.GetTilesetItemCount();
    if (tilesetCount > 0) {
      auto tile = map.GetTileAt(click);
      tile.solid = false;

      tile.ID = ((++tile.ID) % tilesetCount) + 1ull;

      map.SetTileAt(click, tile);

      clicked = true;
    }
  }
  else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) && !clicked) {
    auto tile = map.GetTileAt(click);
    tile.solid = true;
    tile.ID = 0;

    map.SetTileAt(click, tile);

    clicked = true;
  }
  else if (clicked
    && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)
    && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
  {
    clicked = false;
  }*/

  // do default logic
  SceneBase::onUpdate(elapsed);
}

void Overworld::Homepage::onDraw(sf::RenderTexture& surface)
{
  SceneBase::onDraw(surface);
}

void Overworld::Homepage::onStart()
{
  SceneBase::onStart();

  Audio().Stream("resources/loops/undernet.ogg", false);
  infocus = true;
}

void Overworld::Homepage::onResume()
{
  SceneBase::onResume();
  Audio().Stream("resources/loops/undernet.ogg", false);
  infocus = true;
}

void Overworld::Homepage::onLeave()
{
  infocus = false;

  // repeat reconnection in case there was a fail that
  // forced us to return
  client.close();
  isConnected = false;
  reconnecting = false;
}

void Overworld::Homepage::OnTileCollision()
{
  auto playerActor = GetPlayer();
  auto playerPos = playerActor->getPosition();

  auto& map = GetMap();
  auto tileSize = sf::Vector2f(map.GetTileSize());

  auto tilePos = sf::Vector2f(
    std::floor(playerPos.x / (tileSize.x / 2)),
    std::floor(playerPos.y / tileSize.y)
  );

  auto& teleportController = GetTeleportController();

  if (netWarpTilePos == tilePos && teleportController.IsComplete() && isConnected) {
    auto& playerController = GetPlayerController();

    // Calculate the origin by grabbing this tile's grid Y/X values
    // return at the center origin of this tile
    sf::Vector2f returnPoint = sf::Vector2f(
      tilePos.x * tileSize.x / 2 + tileSize.x / 4.0f,
      tilePos.y * tileSize.y + tileSize.y / 2.0f
    );

    auto teleportToCyberworld = [=] {
      this->TeleportUponReturn(returnPoint);
      client.close();
      getController().push<segue<BlackWashFade>::to<Overworld::OnlineArea>>(maxPayloadSize, guest);
    };

    playerController.ReleaseActor();
    auto& command = teleportController.TeleportOut(playerActor);
    command.onFinish.Slot(teleportToCyberworld);
  }
}

void Overworld::Homepage::OnInteract() {}
