#pragma once

#include <Swoosh/Activity.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>
#include <time.h>
#include <typeinfo>
#include <SFML/Graphics.hpp>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>

#include "../../bnMobHealthUI.h"
#include "../../bnCounterHitListener.h"
#include "../../bnCharacterDeleteListener.h"
#include "../../bnBackground.h"
#include "../../bnLanBackground.h"
#include "../../bnGraveyardBackground.h"
#include "../../bnVirusBackground.h"
#include "../../bnCamera.h"
#include "../../bnInputManager.h"
#include "../../bnCardSelectionCust.h"
#include "../../bnCardFolder.h"
#include "../../bnShaderResourceManager.h"
#include "../../bnPA.h"
#include "../../bnDrawWindow.h"
#include "../../bnSceneNode.h"
#include "../../bnBattleResults.h"
#include "../../battlescene/bnBattleSceneBase.h"
#include "../../bnMob.h"
#include "../../bnField.h"
#include "../../bnPlayer.h"
#include "../../bnSelectedCardsUI.h"
#include "../../bnCounterHitListener.h"
#include "../../bnCharacterDeleteListener.h"
#include "../../bnPlayerPackageManager.h"
#include "../../battlescene/States/bnCharacterTransformBattleState.h"

#include "../bnNetPlayFlags.h"
#include "../bnNetPlayConfig.h"
#include "../bnNetPlaySignals.h"
#include "../bnNetPlayPacketProcessor.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;

struct CombatBattleState;
struct TimeFreezeBattleState;
struct NetworkSyncBattleState;
struct CardComboBattleState;
struct BattleStartBattleState;
class CardSelectBattleState;

class Mob;
class Player;
class PlayerHealthUI;
class NetworkCardUseListener; 

struct NetworkBattleSceneProps {
  BattleSceneBaseProps base;
  sf::Sprite mug; // speaker mugshot
  Animation anim; // mugshot animation
  std::shared_ptr<sf::Texture> emotion; // emotion atlas image
  NetPlayConfig& netconfig;
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;
};

struct FrameInputData {
  unsigned int frameNumber{};
  std::vector<InputEvent> events;
};

static bool operator<(const FrameInputData& lhs, const FrameInputData& rhs) {
  return lhs.frameNumber < rhs.frameNumber;
}

class NetworkBattleScene final : public BattleSceneBase {
private:
  friend struct NetworkSyncBattleState;
  friend class NetworkCardUseListener;
  friend class PlayerInputReplicator;
  
  bool waitingForCardSelectScreen{}; //!< Flag to count down before changing states
  frame_time_t roundStartDelay{}; //!< How long to wait on opponent's animations before starting the next round
  frame_time_t cardStateDelay{}; //!< Timer that counts down until the card select state opens up
  frame_time_t packetTime{}; //!< When a packet was sent. Compare the time sent vs the recent ACK for accurate connectivity
  unsigned int remoteFrameNumber{};

  Text ping, frameNumText;
  std::string selectedNaviId; //!< the type of navi we selected
  NetPlayFlags remoteState; //!< remote state flags to ensure stability
  std::vector<std::shared_ptr<Player>> players; //!< Track all players
  std::vector<std::shared_ptr<TrackedFormData>> trackedForms;
  SpriteProxyNode pingIndicator;
  std::shared_ptr<SelectedCardsUI> remoteCardActionUsePublisher{ nullptr };
  std::vector<Battle::Card> remoteHand; // TODO: THIS IS HACKING TO GET IT TO WORK PLS REMOVE LATER
  std::vector<FrameInputData> remoteInputQueue;
  std::shared_ptr<Player> remotePlayer{ nullptr }; //!< their player pawn
  Mob* mob{ nullptr }; //!< Our managed mob structure for PVP
  CombatBattleState* combatPtr{ nullptr };
  TimeFreezeBattleState* timeFreezePtr{ nullptr };
  NetworkSyncBattleState* syncStatePtr{ nullptr };
  CardComboBattleState* cardComboStatePtr{ nullptr };
  CardSelectBattleState* cardStatePtr{ nullptr };
  BattleStartBattleState* startStatePtr{ nullptr };
  std::shared_ptr<Netplay::PacketProcessor> packetProcessor;

  // netcode send funcs
  void SendHandshakeSignal(); // send player data to start the next round
  void SendInputEvents(std::vector<InputEvent>& events); // send our key or gamepad events
  void SendConnectSignal(const std::string& naviId);
  void SendChangedFormSignal(const int form);
  void SendRequestedCardSelectSignal(); 
  void SendPingSignal();

  // netcode recieve funcs
  void RecieveHandshakeSignal(const Poco::Buffer<char>& buffer);
  void RecieveInputEvent(const Poco::Buffer<char>& buffer); 
  void RecieveConnectSignal(const Poco::Buffer<char>&);
  void RecieveChangedFormSignal(const Poco::Buffer<char>&);
  void RecieveRequestedCardSelectSignal(); // if the remote opens card select, we should be too

  void ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>&);
  bool IsRemoteBehind();
  void UpdatePingIndicator(frame_time_t frames);
public:
  using BattleSceneBase::ProcessNewestComponents;

  void OnHit(Entity& victim, const Hit::Properties& props) override final;
  void onUpdate(double elapsed) override final;
  void onDraw(sf::RenderTexture& surface) override final;
  void onExit() override;
  void onEnter() override;
  void onStart() override;
  void onResume() override;
  void onEnd() override;

  void Inject(PlayerInputReplicator& pub);

  const NetPlayFlags& GetRemoteStateFlags();
  const double GetAvgLatency() const;

  /**
   * @brief Construct scene with selected player, generated mob data, and the folder to use
   */
  NetworkBattleScene(swoosh::ActivityController&, NetworkBattleSceneProps& props, BattleResultsFunc onEnd=nullptr);
  
  /**
   * @brief Clears all nodes and components
   */
  ~NetworkBattleScene();

};