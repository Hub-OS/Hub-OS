#pragma once

#include <Swoosh/Activity.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "../bnMobHealthUI.h"
#include "../bnCounterHitListener.h"
#include "../bnCharacterDeleteListener.h"
#include "../bnBackground.h"
#include "../bnLanBackground.h"
#include "../bnGraveyardBackground.h"
#include "../bnVirusBackground.h"
#include "../bnCamera.h"
#include "../bnInputManager.h"
#include "../bnCardSelectionCust.h"
#include "../bnCardFolder.h"
#include "../bnShaderResourceManager.h"
#include "../bnPA.h"
#include "../bnEngine.h"
#include "../bnSceneNode.h"
#include "../bnBattleResults.h"
#include "../bnBattleScene.h"
#include "../bnMob.h"
#include "../bnField.h"
#include "../bnPlayer.h"
#include "../bnSelectedCardsUI.h"
#include "../bnPlayerCardUseListener.h"
#include "../bnEnemyCardUseListener.h"
#include "../bnCounterHitListener.h"
#include "../bnCharacterDeleteListener.h"
#include "../bnCardSummonHandler.h"
#include "../bnNaviRegistration.h"


#include "bnNetPlayFlags.h"

#include <time.h>
#include <typeinfo>
#include <SFML/Graphics.hpp>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

class Mob;
class Player;
class PlayerHealthUI;

constexpr const std::size_t MAX_BUFFER_LEN = 1024;
constexpr const uint16_t OBN_PORT = 8765;

/*******************************
Prototype network stuff
********************************/
struct NetPlayConfig {
  uint16_t myPort{ OBN_PORT };
  uint16_t remotePort{ OBN_PORT };
  std::string remoteIP;
  SelectedNavi myNavi{ 0 };
};

enum class NetPlaySignals : unsigned int {
  none = 0,
  connect,
  handshake,
  ready,
  form,
  move,
  hp,
  tile,
  chip,
  loser,
  shoot,
  special,
  charge
};

class NetworkCardUseListener; // declared bottom of file

class NetworkBattleScene final : public BattleScene {
private:
  friend class NetworkCardUseListener;
  friend class PlayerInputReplicator;

  NetworkCardUseListener* networkCardUseListener;
  Player* remotePlayer{ nullptr };
  CardUsePublisher* remoteCardUsePublisher{ nullptr };
  PlayerCardUseListener* remoteCardUseListener{ nullptr };
  NetPlayFlags remoteState;
  Animation remoteShineAnimation;
  sf::Sprite remoteShine;
  bool isClientReady{ false };
  Poco::Net::DatagramSocket client; //!< us
  SelectedNavi selectedNavi; //!< the type of navi we selected
  Poco::Net::SocketAddress remoteAddress;
  int clientPrevHP{ 1 }; //!< we will send HP signals only when it changes
  bool handshakeComplete{ false };

  void sendHandshakeSignal(); // sent until we recieve a handshake
  void sendShootSignal();
  void sendUseSpecialSignal();
  void sendChargeSignal(const bool);
  void sendConnectSignal(const SelectedNavi navi);
  void sendReadySignal();
  void sendChangedFormSignal(const int form);
  void sendMoveSignal(const Direction dir);
  void sendHPSignal(const int hp);
  void sendTileCoordSignal(const int x, const int y);
  void sendChipUseSignal(const std::string& used);
  void sendLoserSignal(); // if we die, let them know

  void recieveHandshakeSignal();
  void recieveShootSignal();
  void recieveUseSpecialSignal();
  void recieveChargeSignal(const Poco::Buffer<char>& buffer);
  void recieveConnectSignal(const Poco::Buffer<char>&);
  void recieveReadySignal();
  void recieveChangedFormSignal(const Poco::Buffer<char>&);
  void recieveMoveSignal(const Poco::Buffer<char>&);
  void recieveHPSignal(const Poco::Buffer<char>&);
  void recieveTileCoordSignal(const Poco::Buffer<char>&);
  void recieveChipUseSignal(const Poco::Buffer<char>&);
  void recieveLoserSignal(); // if they die, update our state flags

  void processIncomingPackets();

public:
  using BattleScene::ProcessNewestComponents;

  void onUpdate(double elapsed) override final;
  void onDraw(sf::RenderTexture& surface) override final;

  void Inject(PlayerInputReplicator& pub);

  /**
   * @brief Construct scene with selected player, generated mob data, and the folder to use
   */
  NetworkBattleScene(swoosh::ActivityController&, Player*, CardFolder* folder, const NetPlayConfig&);
  
  /**
   * @brief Clears all nodes and components
   */
  ~NetworkBattleScene();

};

class NetworkCardUseListener final : public CardUseListener {
  NetworkBattleScene& bs;
  Player& client;

public:
  NetworkCardUseListener(NetworkBattleScene& bs, Player& client)
    : bs(bs), client(client), CardUseListener() {

  }

  void OnCardUse(Battle::Card& card, Character& user) override {
    bs.sendChipUseSignal(card.GetUUID());
  }
};