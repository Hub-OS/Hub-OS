#pragma once

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include <map>

#include "bnOverworldSceneBase.h"
#include "bnPacketShipper.h"
#include "bnPacketSorter.h"
#include "bnBufferReader.h"

namespace Overworld {
  constexpr size_t LAG_WINDOW_LEN = 300;

  struct OnlinePlayer {
    Overworld::Actor actor{"?"};
    Overworld::EmoteNode emoteNode;
    Overworld::TeleportController teleportController{};
    SelectedNavi currNavi{std::numeric_limits<SelectedNavi>::max()};
    sf::Vector2f startBroadcastPos{};
    sf::Vector2f endBroadcastPos{};
    long long timestamp{};
    std::array<double, LAG_WINDOW_LEN> lagWindow{ 0 };
    size_t packets{};
  };

  class OnlineArea final : public SceneBase {
  private:
    std::string ticket; //!< How we are represented on the server
    Poco::Net::DatagramSocket client; //!< us
    Poco::Net::SocketAddress remoteAddress; //!< server
    bool isConnected{ false };
    PacketShipper packetShipper;
    PacketSorter packetSorter;
    SelectedNavi lastFrameNavi{};
    std::map<std::string, OnlinePlayer*> onlinePlayers;
    std::list<std::string> removePlayers;
    std::string mapBuffer;
    Timer loadMapTime, movementTimer;
    Font font;
    Text name;
    size_t errorCount{};

    void Leave();
    void RefreshOnlinePlayerSprite(OnlinePlayer& player, SelectedNavi navi);
    const bool IsMouseHovering(const sf::RenderTarget& target, const SpriteProxyNode& src);
    const double CalculatePlayerLag(OnlinePlayer& player, double nextLag = 0);
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
    void onResume() override;

    const std::pair<bool, Map::Tile**> FetchMapData() override;
    void OnTileCollision(const Map::Tile& tile) override;
    void OnEmoteSelected(Overworld::Emotes emote) override;

    void sendLoginSignal();
    void sendLogoutSignal();
    void sendReadySignal();
    void sendPositionSignal();
    void sendAvatarChangeSignal(const SelectedNavi& navi);
    void sendEmoteSignal(const Overworld::Emotes emote);
    void receiveLoginSignal(BufferReader& reader, const Poco::Buffer<char>&);
    void receiveMapSignal(BufferReader& reader, const Poco::Buffer<char>&);
    void receiveNaviConnectedSignal(BufferReader& reader, const Poco::Buffer<char>&);
    void receiveNaviDisconnectedSignal(BufferReader& reader, const Poco::Buffer<char>&);
    void receiveNaviSetNameSignal(BufferReader& reader, const Poco::Buffer<char>&);
    void receiveNaviMoveSignal(BufferReader& reader, const Poco::Buffer<char>&);
    void receiveNaviSetAvatarSignal(BufferReader& reader, const Poco::Buffer<char>&);
    void receiveNaviEmoteSignal(BufferReader& reader, const Poco::Buffer<char>&);

    void processIncomingPackets();

  };
}