#pragma once
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <map>

#include "bnOverworldSceneBase.h"

namespace Overworld {
  // server expects uint16_t codes
  enum class OnlineSignals : uint16_t {
    login = 0,
    logout,
    xyz,
    map,
    change
  };

  struct OnlinePlayer {
    Overworld::Actor actor{"?"};
    SelectedNavi currNavi{};
  };

  class OnlineArea final : public SceneBase {
  private:
    Poco::Net::DatagramSocket client; //!< us
    Poco::Net::SocketAddress remoteAddress; //!< server
    bool isConnected{ false };
    SelectedNavi lastFrameNavi{};
    std::map<std::string, OnlinePlayer*> onlinePlayers;
    std::string mapBuffer;
    Timer loadMapTime;

    void RefreshOnlinePlayerSprite(OnlinePlayer& player, SelectedNavi navi);

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
    void recieveXYZSignal(const Poco::Buffer<char>&);
    void recieveNaviChangeSignal(const Poco::Buffer<char>&);
    void recieveLoginSignal(const Poco::Buffer<char>&);
    void recieveLogoutSignal(const Poco::Buffer<char>&);
    void recieveMapSignal(const Poco::Buffer<char>&);

    void processIncomingPackets();

  };
}