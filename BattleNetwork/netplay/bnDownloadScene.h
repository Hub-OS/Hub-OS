#pragma once

#include <Swoosh/Activity.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>
#include <Swoosh/EmbedGLSL.h>
#include <Swoosh/Shaders.h>

#include <time.h>
#include <queue>
#include <typeinfo>
#include <SFML/Graphics.hpp>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>

#include "bnNetPlayPacketProcessor.h"
#include "../bnScene.h"
#include "../bnText.h"
#include "../../bnInputManager.h"
#include "../../bnDrawWindow.h"
#include "../../bnSceneNode.h"
#include "../../bnWebClientMananger.h"

struct DownloadSceneProps {
  bool& downloadSuccess;
  std::vector<std::string> webCardsUUIDs, cardPackageHashes;
  std::string playerHash;
  std::string& remotePlayerHash;
  Poco::Net::SocketAddress remoteAddress;
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;
  sf::Texture lastScreen;
};

class DownloadScene final : public Scene {
private:
  enum class TaskTypes : char {
    none = 0,
    trade_web_cards,
    trade_card_packages,
    trade_player_package,
    wait
  } currTask{}, nextTask{ TaskTypes::trade_web_cards };

  bool& downloadSuccess;
  bool aborting{}, remoteSuccess{};
  frame_time_t abortingCountdown{frames(150)};
  size_t tries{}; //!< After so many attempts, quit the download...
  size_t packetAckId{};
  std::string playerHash;
  std::string& remotePlayerHash;
  std::vector<std::string> playerWebCardList, playerCardPackageList;
  std::map<std::string, std::string> contentToDownload;
  Text label;
  sf::Sprite bg; // background
  sf::RenderTexture surface;
  sf::Texture lastScreen;
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;
  swoosh::glsl::FastGaussianBlur blur{ 10 };

  void RemoveFromDownloadList(const std::string& id);

  void SendHandshakeAck();
  bool ProcessTaskQueue();
  bool AllTasksComplete();
  void SendPing(); //!< keep connections alive while clients download data

  // Notify remote of health
  void SendDownloadComplete(bool success);

  // Initiate trades
  void TradeWebCardList(const std::vector<std::string>& uuids);
  void TradePlayerPackageData(const std::string& hash);
  void TradeCardPackageData(const std::vector<std::string>& hashes);

  // Initiate requests
  void RequestWebCardList(const std::vector<std::string>& uuids);
  void RequestPlayerPackageData(const std::string& hash);
  void RequestCardPackageList(const std::vector<std::string>& hash);

  // Handle recieve 
  void RecieveTradeWebCardList(const Poco::Buffer<char>& buffer);
  void RecieveTradePlayerPackageData(const Poco::Buffer<char>& buffer);
  void RecieveTradeCardPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestWebCardList(const Poco::Buffer<char>& buffer);
  void RecieveRequestPlayerPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestCardPackageData(const Poco::Buffer<char>& buffer);
  void RecieveDownloadComplete(const Poco::Buffer<char>& buffer);

  // Downloads
  void DownloadCardList(const Poco::Buffer<char>& buffer);
  void DownloadPlayerData(const Poco::Buffer<char>& buffer);

  template<typename PackageManagerType, typename ScriptedDataType>
  void DownloadPackageData(const Poco::Buffer<char>& buffer, PackageManagerType& pm);

  // Serializers
  std::vector<std::string> DeserializeListOfStrings(const Poco::Buffer<char>& buffer);
  Poco::Buffer<char> SerializeListOfStrings(NetPlaySignals header, const std::vector<std::string>& list);
  Poco::Buffer<char> SerializeCards(const std::vector<std::string>& cardList);

  template<typename PackageManagerType>
  Poco::Buffer<char> SerializePackageData(const std::string& hash, NetPlaySignals header, PackageManagerType& pm);

  // Aux
  void Abort();
  void ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>& body);

public:
  void onUpdate(double elapsed) override final;
  void onDraw(sf::RenderTexture& surface) override final;
  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onStart() override;
  void onResume() override;
  void onEnd() override;

  /**
   * @brief Construct scene from previous screen's contents
   */
  DownloadScene(swoosh::ActivityController&, const DownloadSceneProps& props);
  ~DownloadScene();

};