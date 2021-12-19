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

class CardPackageManager;
class PlayerPackageManager;
class BlockPackageManager;

struct DownloadSceneProps;

class DownloadScene final : public Scene {
public:
  struct Hash {
    std::string packageId;
    std::string md5;
  };

private:
  bool& downloadSuccess;
  bool downloadFlagSet{}, aborting{}, remoteSuccess{}, remoteHandshake{}, hasTradedData{};
  bool playerPackageRequested{}, cardPackageRequested{}, blockPackageRequested{};
  unsigned& coinFlip;
  unsigned mySeed{};
  frame_time_t abortingCountdown{frames(150)};
  size_t tries{}; //!< After so many attempts, quit the download...
  size_t packetAckId{};
  Hash playerHash;
  PackageAddress& remotePlayer;
  std::vector<PackageAddress>& remoteBlocks;
  std::vector<DownloadScene::Hash> playerCardPackageList, playerBlockPackageList;
  std::map<std::string, std::string> contentToDownload;
  Text label;
  sf::Sprite bg; // background
  sf::RenderTexture surface;
  sf::Texture lastScreen;
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;
  swoosh::glsl::FastGaussianBlur blur{ 10 };

  void ResetRemotePartitions(); // prepare remote namespace for incoming mods
  CardPackageManager& RemoteCardPartition();
  CardPackageManager& LocalCardPartition();
  BlockPackageManager& RemoteBlockPartition();
  BlockPackageManager& LocalBlockPartition();
  PlayerPackageManager& RemotePlayerPartition();
  PlayerPackageManager& LocalPlayerPartition();

  template<template<typename> class PackageManagerType, class MetaType>
  bool DifferentHash(PackageManagerType<MetaType>& packageManager, const std::string& packageId, const std::string& desiredFingerprint);

  void RemoveFromDownloadList(const std::string& id);

  void SendHandshakeAck();
  bool AllTasksComplete();
  void SendPing(); //!< keep connections alive while clients download data

  // Notify remote of health
  void SendDownloadComplete(bool success);

  // Initiate trades
  void TradePlayerPackageData(const Hash& hash);
  void TradeCardPackageData(const std::vector<Hash>& hashes);
  void TradeBlockPackageData(const std::vector<Hash>& hashes);

  // Initiate requests
  void RequestPlayerPackageData(const std::string& hash);
  void RequestCardPackageList(const std::vector<std::string>& hashes);
  void RequestBlockPackageList(const std::vector<std::string>& hashes);

  // Handle recieve 
  void RecieveHandshake(const Poco::Buffer<char>& buffer);
  void RecieveTradePlayerPackageData(const Poco::Buffer<char>& buffer);
  void RecieveTradeCardPackageData(const Poco::Buffer<char>& buffer);
  void RecieveTradeBlockPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestPlayerPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestCardPackageData(const Poco::Buffer<char>& buffer);
  void RecieveRequestBlockPackageData(const Poco::Buffer<char>& buffer);
  void RecieveDownloadComplete(const Poco::Buffer<char>& buffer);

  // Downloads
  void DownloadPlayerData(const Poco::Buffer<char>& buffer);

  template<typename PackageManagerType, typename ScriptedDataType>
  void DownloadPackageData(const Poco::Buffer<char>& buffer, PackageManagerType& pm);

  // Serializers
  std::vector<Hash> DeserializeListOfHashes(const Poco::Buffer<char>& buffer);
  Poco::Buffer<char> SerializeListOfHashes(NetPlaySignals header, const std::vector<Hash>& list);

  template<typename PackageManagerType>
  Poco::Buffer<char> SerializePackageData(const std::string& packageId, NetPlaySignals header, PackageManagerType& pm);

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

struct DownloadSceneProps {
  std::vector<DownloadScene::Hash> cardPackageHashes;
  std::vector<DownloadScene::Hash> blockPackageHashes;
  DownloadScene::Hash playerHash;
  Poco::Net::SocketAddress remoteAddress;
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;
  sf::Texture lastScreen;
  bool& downloadSuccess;
  unsigned& coinFlip;
  PackageAddress& remotePlayer;
  std::vector<PackageAddress>& remotePlayerBlocks;
};