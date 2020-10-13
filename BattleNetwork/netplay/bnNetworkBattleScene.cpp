#include <Swoosh/ActivityController.h>
#include <Segues/WhiteWashFade.h>
#include <Segues/PixelateBlackWashFade.h>
#include <chrono>

#include "bnNetworkBattleScene.h"
#include "../bnMainMenuScene.h"
#include "bnPlayerInputReplicator.h"
#include "bnPlayerNetworkState.h"
#include "bnPlayerNetworkProxy.h"

#include "../bnCardLibrary.h"
#include "../bnGameOverScene.h"
#include "../bnUndernetBackground.h"
#include "../bnWeatherBackground.h"
#include "../bnRobotBackground.h"
#include "../bnMedicalBackground.h"
#include "../bnACDCBackground.h"
#include "../bnMiscBackground.h"
#include "../bnSecretBackground.h"
#include "../bnJudgeTreeBackground.h"
#include "../bnPlayerHealthUI.h"
#include "../bnPaletteSwap.h"
#include "../bnWebClientMananger.h"
#include "../bnFadeInState.h"

// Android only headers
#include "../Android/bnTouchArea.h"

// modals like card cust and battle reward slide in 12px per frame for 10 frames. 60 frames = 1 sec
// modal slide moves 120px in 1/6th of a second
// Per 1 second that is 6*120px in 6*1/6 of a sec = 720px in 1 sec
#define MODAL_SLIDE_PX_PER_SEC 720.0f

// Combos are counted if more than one enemy is hit within x frames
// The game is clocked to display 60 frames per second
// If x = 20 frames, then we want a combo hit threshold of 20/60 = 0.3 seconds
#define COMBO_HIT_THRESHOLD_SECONDS 20.0f/60.0f

using namespace swoosh::types;
using swoosh::ActivityController;

NetworkBattleScene::NetworkBattleScene(ActivityController& controller, const NetworkBattleSceneProps& props) : BattleSceneBase(controller, props.base) {
  networkCardUseListener = new NetworkCardUseListener(*this, props.base.player);
  networkCardUseListener->Subscribe(this->GetSelectedCardsUI());

  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), props.netconfig.myPort); 
  client = Poco::Net::DatagramSocket(sa);
  //client.bind(sa, true);
  client.setBlocking(false);

  remoteAddress = Poco::Net::SocketAddress(props.netconfig.remoteIP, props.netconfig.remotePort);
  client.connect(remoteAddress);

  selectedNavi = props.netconfig.myNavi;

  props.base.player.CreateComponent<PlayerInputReplicator>(&props.base.player);

  remoteShineAnimation = Animation("resources/mobs/boss_shine.animation");
  remoteShineAnimation.Load();

  remoteShine = sf::Sprite(*LOAD_TEXTURE(MOB_BOSS_SHINE));
  remoteShine.setScale(2.f, 2.f);
}

NetworkBattleScene::~NetworkBattleScene()
{ 
  delete remotePlayer;
  delete remoteCardUsePublisher;
  delete networkCardUseListener;
  client.close();
}

void NetworkBattleScene::onUpdate(double elapsed) {
  
}

void NetworkBattleScene::onDraw(sf::RenderTexture& surface) {
}

void NetworkBattleScene::onExit()
{
}

void NetworkBattleScene::onEnter()
{
}

void NetworkBattleScene::onResume()
{
}

void NetworkBattleScene::onEnd()
{
}

void NetworkBattleScene::Inject(PlayerInputReplicator& pub)
{
}

void NetworkBattleScene::sendHandshakeSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::handshake };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendShootSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::shoot };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendUseSpecialSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::special };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendChargeSignal(const bool state)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::charge };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&state, sizeof(bool));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendConnectSignal(const SelectedNavi navi)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::connect };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&navi, sizeof(SelectedNavi));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendReadySignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::ready };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), (int)buffer.size());

  /*if (remoteState.isRemoteReady) {
    this->isPreBattle = true;
    this->battleStartTimer.reset();
  }*/

  this->isClientReady = true;
}

void NetworkBattleScene::sendChangedFormSignal(const int form)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::form };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&form, sizeof(int));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendMoveSignal(const Direction dir)
{
  Logger::Logf("sending dir of %i", dir);

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::move };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&dir, sizeof(char));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendHPSignal(const int hp)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::hp };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&hp, sizeof(int));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendTileCoordSignal(const int x, const int y)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::tile };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&x, sizeof(int));
  buffer.append((char*)&y, sizeof(int));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendChipUseSignal(const std::string& used)
{
  Logger::Logf("sending chip data over network for %s", used.data());

  uint64_t timestamp = (uint64_t)CurrentTime::AsMilli();

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::chip };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&timestamp, sizeof(uint64_t));
  buffer.append((char*)used.data(),used.length());
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::sendLoserSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::loser };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void NetworkBattleScene::recieveHandshakeSignal()
{
  this->handshakeComplete = true;
  this->sendHandshakeSignal();
}

void NetworkBattleScene::recieveShootSignal()
{
  if (!remoteState.isRemoteConnected) return;

  //remotePlayer->Attack();
  remoteState.remoteShoot = true;
  Logger::Logf("recieved shoot signal from remote");
}

void NetworkBattleScene::recieveUseSpecialSignal()
{
  if (!remoteState.isRemoteConnected) return;

  // remotePlayer->UseSpecial();
  remoteState.remoteUseSpecial = true;

  Logger::Logf("recieved use special signal from remote");

}

void NetworkBattleScene::recieveChargeSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  bool state = remoteState.remoteCharge; std::memcpy(&state, buffer.begin(), buffer.size());
  remoteState.remoteCharge = state;

  Logger::Logf("recieved charge signal from remote: %i", state);

}

void NetworkBattleScene::recieveConnectSignal(const Poco::Buffer<char>& buffer)
{
  if (remoteState.isRemoteConnected) return; // prevent multiple connection requests...

  remoteState.isRemoteConnected = true;

  SelectedNavi navi = SelectedNavi{ 0 }; std::memcpy(&navi, buffer.begin(), buffer.size());
  remoteState.remoteNavi = navi;

  Logger::Logf("Recieved connect signal! Remote navi: %i", remoteState.remoteNavi);

  assert(remotePlayer == nullptr && "remote player was already set!");
  remotePlayer = NAVIS.At(navi).GetNavi();
  remotePlayer->SetTeam(Team::blue);
  remotePlayer->setScale(remotePlayer->getScale().x * -1.0f, remotePlayer->getScale().y);
  remotePlayer->ChangeState<PlayerIdleState>();
  remotePlayer->Character::Update(0);

  FinishNotifier onFinish = [this]() {
    //customProgress = customDuration; // open the cust
  };

  remotePlayer->ChangeState<FadeInState<Player>>(onFinish);
  GetField()->AddEntity(*remotePlayer, remoteState.remoteTileX, remoteState.remoteTileY);
  remotePlayer->ToggleTimeFreeze(true);

  remoteState.remoteHP = remotePlayer->GetHealth();

  remoteCardUsePublisher = remotePlayer->CreateComponent<SelectedCardsUI>(remotePlayer);
  remoteCardUseListener = new PlayerCardUseListener(*remotePlayer);
  remoteCardUseListener->Subscribe(*remoteCardUsePublisher);
  //this->summons.Subscribe(*remoteCardUsePublisher);

  remotePlayer->CreateComponent<MobHealthUI>(remotePlayer);
  remotePlayer->CreateComponent<PlayerNetworkProxy>(remotePlayer, remoteState);
}

void NetworkBattleScene::recieveReadySignal()
{
  if (!remoteState.isRemoteConnected) return;

  if (this->isClientReady && !remoteState.isRemoteReady) {
    //this->isPreBattle = true;
    //this->battleStartTimer.reset();
  }

  remoteState.isRemoteReady = true;
}

void NetworkBattleScene::recieveChangedFormSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  int form = remoteState.remoteFormSelect;
  int prevForm = remoteState.remoteFormSelect;
  std::memcpy(&form, buffer.begin(), sizeof(int));

  if (remotePlayer && form != prevForm) {
    // If we were in a form, replay the animation
    // going back to our base this time
    remoteState.remoteLastFormSelect = remoteState.remoteFormSelect;
    remoteState.remoteFormSelect = form;
    remoteState.remoteIsFormChanging = true; // begins the routine
    remoteState.remoteIsAnimatingFormChange = false; // state moment flags must be false to trigger
    remoteState.remoteIsLeavingFormChange = false;   // ... after the backdrop turns dark
  }
}

void NetworkBattleScene::recieveMoveSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  Direction dir = remoteState.remoteDirection; std::memcpy(&dir, buffer.begin(), buffer.size());

  if (!GetPlayer()->Teammate(remotePlayer->GetTeam())) {
    if (dir == Direction::left || dir == Direction::right) {
      dir = Reverse(dir);
    }
  }

  Logger::Logf("recieved move signal from remote %i", dir);

  remoteState.remoteDirection = dir;
}

void NetworkBattleScene::recieveHPSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  int hp = remoteState.remoteHP; std::memcpy(&hp, buffer.begin(), buffer.size());
  remoteState.remoteHP = hp;
  remotePlayer->SetHealth(hp);
}

void NetworkBattleScene::recieveTileCoordSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  int x = remoteState.remoteTileX; std::memcpy(&x, buffer.begin(), sizeof(int));
  int y = remoteState.remoteTileX; std::memcpy(&y, (buffer.begin()+sizeof(int)), sizeof(int));

  // mirror the x value for remote
  x = (GetField()->GetWidth() - x)+1;

  remoteState.remoteTileX = x;
  remoteState.remoteTileY = y;

  Battle::Tile* t = GetField()->GetAt(x, y);

  if (remotePlayer->GetTile() != t && !remotePlayer->IsSliding()) {
    remotePlayer->GetTile()->RemoveEntityByID(remotePlayer->GetID());
    remotePlayer->AdoptTile(t);
    remotePlayer->FinishMove();
  }
}

void NetworkBattleScene::recieveChipUseSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  uint64_t timestamp = 0; std::memcpy(&timestamp, buffer.begin(), sizeof(uint64_t));
  std::string used = std::string(buffer.begin()+sizeof(uint64_t), buffer.size()-sizeof(uint64_t));
  remoteState.remoteChipUse = used;
  Battle::Card card = WEBCLIENT.MakeBattleCardFromWebCardData(WebAccounts::Card{ used });
  remoteCardUsePublisher->Broadcast(card, *remotePlayer, timestamp);
  Logger::Logf("remote used chip %s", used.c_str());
}

void NetworkBattleScene::recieveLoserSignal()
{
  remoteState.isRemotePlayerLoser = true;
}

void NetworkBattleScene::processIncomingPackets()
{
  if (!client.poll(Poco::Timespan{ 0 }, Poco::Net::Socket::SELECT_READ)) return;

  static int errorCount = 0;

  if (errorCount > 10) {
    AUDIO.StopStream();
    using effect = segue<PixelateBlackWashFade>;
    getController().queuePop<effect>();
    errorCount = 0; // reset for next match
    return;
  }

  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };
  static int read = 0;

  try {
    read+= client.receiveBytes(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN-1);
    if (read > 0) {
      rawBuffer[read] = '\0';

      NetPlaySignals sig = *(NetPlaySignals*)rawBuffer;
      size_t sigLen = sizeof(NetPlaySignals);
      Poco::Buffer<char> data{ 0 };
      data.append(rawBuffer + sigLen, size_t(read)-sigLen);

      switch (sig) {
      case NetPlaySignals::handshake:
        recieveHandshakeSignal();
        break;
      case NetPlaySignals::connect:
        recieveConnectSignal(data);
        break;
      case NetPlaySignals::chip:
        recieveChipUseSignal(data);
        break;
      case NetPlaySignals::form:
        recieveChangedFormSignal(data);
        break;
      case NetPlaySignals::hp:
        recieveHPSignal(data);
        break;
      case NetPlaySignals::loser:
        recieveLoserSignal();
        break;
      case NetPlaySignals::move:
        recieveMoveSignal(data);
        break;
      case NetPlaySignals::ready:
        recieveReadySignal();
        break;
      case NetPlaySignals::tile:
        recieveTileCoordSignal(data);
        break;
      case NetPlaySignals::shoot:
        recieveShootSignal();
        break;
      case NetPlaySignals::special:
        recieveUseSpecialSignal();
        break;
      case NetPlaySignals::charge:
        recieveChargeSignal(data);
        break;
      }
    }

    errorCount = 0;
  }
  catch (std::exception& e) {
    Logger::Logf("PVP Network exception: %s", e.what());
    
    if (remoteState.isRemoteConnected) {
      errorCount++;
    }
  }

  read = 0;
  std::memset(rawBuffer, 0, NetPlayConfig::MAX_BUFFER_LEN);
}
