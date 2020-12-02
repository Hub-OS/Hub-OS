#pragma once
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <map>

#include "bnOverworldSceneBase.h"

namespace Overworld {
  // server expects uint16_t codes
  enum class ClientEvents : uint16_t {
    login = 0,    // 0 login request
    user_xyz,     // 1 reporting avatar world location
    logout,       // 2 logout notification
    loaded_map,   // 3 avatar loaded map
    avatar_change,// 4 avatar was switched
    size,
    unknown = size
  };

  enum class ServerEvents : uint16_t {
    login = 0,     // 0
    hologram_xyz,  // 1
    hologram_name, // 3
    time_of_day,   // 4
    map,           // 5
    logout,        // 6
    avatar_change, // 7
    size,
    unknown = size
  };

  struct OnlinePlayer {
    Overworld::Actor actor{"?"};
    TeleportController teleportController{};
    SelectedNavi currNavi{std::numeric_limits<SelectedNavi>::max()};
    sf::Vector2f startBroadcastPos{};
    sf::Vector2f endBroadcastPos{};
    long long timestamp{};
    double avgLagTime{};
    size_t packets{};
  };

  class OnlineArea final : public SceneBase {
  private:
    Poco::Net::DatagramSocket client; //!< us
    Poco::Net::SocketAddress remoteAddress; //!< server
    bool isConnected{ false };
    SelectedNavi lastFrameNavi{};
    std::map<std::string, OnlinePlayer*> onlinePlayers;
    std::list<std::string> removePlayers;
    std::string mapBuffer;
    Timer loadMapTime, movementTimer;
    std::shared_ptr<sf::Font> font;
    sf::Text name;
    size_t errorCount{};

    void RefreshOnlinePlayerSprite(OnlinePlayer& player, SelectedNavi navi);
    const bool IsMouseHovering(const sf::RenderTarget& target, const SpriteProxyNode& src);
  public:
    /**
     * @brief Loads the player's library data and loads graphics
     */
    OnlineArea(swoosh::ActivityController&, bool guestAccount);

    /**
    * @brief deconstructor
    */
    ~OnlineArea();

    void onUpdate(double elapsed) override;
    void onDraw(sf::RenderTexture& surface) override;
    void onStart() override;

    const std::pair<bool, Map::Tile**> FetchMapData() override;
    void OnTileCollision(const Map::Tile& tile) override;

    void sendXYZSignal();
    void sendNaviChangeSignal(const SelectedNavi& navi);
    void sendLoginSignal();
    void sendLogoutSignal();
    void sendMapRefreshSignal();
    void recieveXYZSignal(const Poco::Buffer<char>&);
    void recieveNameSignal(const Poco::Buffer<char>&);
    void recieveNaviChangeSignal(const Poco::Buffer<char>&);
    void recieveLoginSignal(const Poco::Buffer<char>&);
    void recieveLogoutSignal(const Poco::Buffer<char>&);
    void recieveMapSignal(const Poco::Buffer<char>&);

    void processIncomingPackets();

  };
}