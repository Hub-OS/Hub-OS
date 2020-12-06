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
    emote,        // 5 player emoted
    size,
    unknown = size
  };

  enum class ServerEvents : uint16_t {
    login = 0,     // 0
    hologram_xyz,  // 1
    hologram_name, // 2
    time_of_day,   // 3
    map,           // 4
    logout,        // 5
    avatar_change, // 6
    emote,         // 7
    avatar_join,   // 8
    size,
    unknown = size
  };

  struct OnlinePlayer {
    Overworld::Actor actor{"?"};
    Overworld::EmoteNode emoteNode;
    Overworld::TeleportController teleportController{};
    SelectedNavi currNavi{std::numeric_limits<SelectedNavi>::max()};
    sf::Vector2f startBroadcastPos{};
    sf::Vector2f endBroadcastPos{};
    long long timestamp{};
    double avgLagTime{};
    size_t packets{};
  };

  class OnlineArea final : public SceneBase {
  private:
    std::string ticket; //!< How we are represented on the server
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
    void OnEmoteSelected(Overworld::Emotes emote) override;

    void sendXYZSignal();
    void sendNaviChangeSignal(const SelectedNavi& navi);
    void sendLoginSignal();
    void sendLogoutSignal();
    void sendMapRefreshSignal();
    void sendEmoteSignal(const Overworld::Emotes emote);
    void recieveXYZSignal(const Poco::Buffer<char>&);
    void recieveNameSignal(const Poco::Buffer<char>&);
    void recieveNaviChangeSignal(const Poco::Buffer<char>&);
    void recieveLoginSignal(const Poco::Buffer<char>&);
    void recieveAvatarJoinSignal(const Poco::Buffer<char>&);
    void recieveLogoutSignal(const Poco::Buffer<char>&);
    void recieveMapSignal(const Poco::Buffer<char>&);
    void recieveEmoteSignal(const Poco::Buffer<char>&);

    void processIncomingPackets();

  };
}