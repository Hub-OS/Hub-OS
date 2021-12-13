#include <Swoosh/ActivityController.h>
#include <Segues/WhiteWashFade.h>
#include <Segues/PixelateBlackWashFade.h>
#include <chrono>

#include "bnNetworkBattleScene.h"
#include "../../bnFadeInState.h"
#include "../../bnElementalDamage.h"
#include "../../bnBlockPackageManager.h"

// states 
#include "states/bnNetworkSyncBattleState.h"
#include "../../battlescene/States/bnRewardBattleState.h"
#include "../../battlescene/States/bnTimeFreezeBattleState.h"
#include "../../battlescene/States/bnBattleStartBattleState.h"
#include "../../battlescene/States/bnBattleOverBattleState.h"
#include "../../battlescene/States/bnFadeOutBattleState.h"
#include "../../battlescene/States/bnCombatBattleState.h"
#include "../../battlescene/States/bnCardSelectBattleState.h"
#include "../../battlescene/States/bnCardComboBattleState.h"

// Android only headers
#include "../../Android/bnTouchArea.h"

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

NetworkBattleScene::NetworkBattleScene(ActivityController& controller, NetworkBattleSceneProps& props, BattleResultsFunc onEnd) :
  BattleSceneBase(controller, props.base, onEnd),
  ping(Font::Style::wide),
  frameNumText(Font::Style::wide)
{
  ping.setPosition(480 - (2.f * 16) - 4, 320 - 2.f); // screen upscaled w - (16px*upscale scale) - (2px*upscale)
  ping.SetColor(sf::Color::Red);

  pingIndicator.setTexture(Textures().LoadFromFile("resources/ui/ping.png"));
  pingIndicator.getSprite().setOrigin(sf::Vector2f(16.f, 16.f));
  pingIndicator.setPosition(480, 320);
  pingIndicator.setScale(2.f, 2.f);
  UpdatePingIndicator(frames(0));

  std::shared_ptr<Player> clientPlayer = props.base.player;

  selectedNaviId = props.netconfig.myNaviId;

  packetProcessor = props.packetProcessor;
  packetProcessor->SetKickCallback([this] {
    this->Quit(FadeOut::black);
  });

  packetProcessor->SetPacketBodyCallback([this](NetPlaySignals header, const Poco::Buffer<char>& buffer) {
    this->ProcessPacketBody(header, buffer);
  });

  GetCardSelectWidget().PreventRetreat();

  // If playing co-op, add more players to track here
  players = { clientPlayer };

  // ptr to player, form select index (-1 none), if should transform
  // TODO: just make this a struct to use across all states that need it...
  trackedForms = {
    std::make_shared<TrackedFormData>(clientPlayer.get(), -1, false)
  };

  // trackedForms[0]->SetReadyState<PlayerControlledState>();

  // in seconds
  double battleDuration = 10.0;

  mob = new Mob(props.base.field);

  // First, we create all of our scene states
  auto syncState = AddState<NetworkSyncBattleState>(remotePlayer, this);
  auto cardSelect = AddState<CardSelectBattleState>(players, trackedForms);
  auto combat = AddState<CombatBattleState>(mob, players, battleDuration);
  auto combo = AddState<CardComboBattleState>(this->GetSelectedCardsUI(), props.base.programAdvance);
  auto forms = AddState<CharacterTransformBattleState>(trackedForms);
  auto battlestart = AddState<BattleStartBattleState>(players);
  auto battleover = AddState<BattleOverBattleState>(players);
  auto timeFreeze = AddState<TimeFreezeBattleState>();
  auto fadeout = AddState<FadeOutBattleState>(FadeOut::black, players); // this state requires arguments

  // We need to respond to new events later, create a resuable pointer to these states
  timeFreezePtr = &timeFreeze.Unwrap();
  combatPtr = &combat.Unwrap();
  syncStatePtr = &syncState.Unwrap();
  cardComboStatePtr = &combo.Unwrap();
  startStatePtr = &battlestart.Unwrap();
  cardStatePtr = &cardSelect.Unwrap();

  // Important! State transitions are added in order of priority!
  syncState.ChangeOnEvent(cardSelect, &NetworkSyncBattleState::IsRemoteConnected);

  // Goto the combo check state if new cards are selected...
  syncState.ChangeOnEvent(combo, &NetworkSyncBattleState::SelectedNewChips);

  // ... else if forms were selected, go directly to forms ....
  syncState.ChangeOnEvent(forms, &NetworkSyncBattleState::HasForm);

  // ... Finally if none of the above, just start the battle
  syncState.ChangeOnEvent(battlestart, &NetworkSyncBattleState::NoConditions);

  // Wait for handshake to complete by going back to the sync state..
  cardSelect.ChangeOnEvent(syncState, &CardSelectBattleState::OKIsPressed);

  // If we reached the combo state, we must also check if form transformation was next
  // or to just start the battle after
  combo.ChangeOnEvent(forms, [cardSelect, combo, this]() mutable {return combo->IsDone() && (cardSelect->HasForm() || remoteState.remoteChangeForm); });
  combo.ChangeOnEvent(battlestart, &CardComboBattleState::IsDone);
  forms.ChangeOnEvent(battlestart, &CharacterTransformBattleState::IsFinished);
  battlestart.ChangeOnEvent(combat, &BattleStartBattleState::IsFinished);
  timeFreeze.ChangeOnEvent(combat, &TimeFreezeBattleState::IsOver);

  // special condition: if lost in combat and had a form, trigger the character transform states
  auto playerLosesInForm = [this] {
    const bool changeState = this->trackedForms[0]->player->GetHealth() == 0 && (this->trackedForms[0]->selectedForm != -1);

    if (changeState) {
      this->trackedForms[0]->selectedForm = -1;
      this->trackedForms[0]->animationComplete = false;
    }

    return changeState;
  };

  // Lambda event callback that captures and handles network card select screen opening
  auto onCardSelectEvent = [this]() mutable {
    bool remoteRequestedChipSelect = this->remotePlayer->InputState().Has(InputEvents::pressed_cust_menu);
    return combatPtr->PlayerRequestCardSelect() || (remoteRequestedChipSelect && this->IsCustGaugeFull());
  };

  // combat has multiple state interruptions based on events
  // so we can chain them together
  combat
    .ChangeOnEvent(battleover, &CombatBattleState::PlayerWon)
    //.ChangeOnEvent(forms, playerLosesInForm)
    .ChangeOnEvent(fadeout, &CombatBattleState::PlayerLost)
    .ChangeOnEvent(cardSelect, onCardSelectEvent)
    .ChangeOnEvent(timeFreeze, &CombatBattleState::HasTimeFreeze);

  battleover.ChangeOnEvent(fadeout, &BattleOverBattleState::IsFinished);

  // share some values between states
  combo->ShareCardList(cardSelect->GetCardPtrList());

  // Some states need to know about card uses
  auto& ui = this->GetSelectedCardsUI();
  ui.Reveal();
  
  GetCardSelectWidget().SetSpeaker(props.mug, props.anim);
  GetEmotionWindow().SetTexture(props.emotion);

  combat->Subscribe(ui);
  timeFreeze->Subscribe(ui);

  // pvp cannot pause
  combat->EnablePausing(false);

  // Some states are part of the combat routine and need to respect
  // the combat state's timers
  combat->subcombatStates.push_back(&timeFreeze.Unwrap());

  // this kicks-off the state graph beginning with the intro state
  this->StartStateGraph(syncState);
  LoadMob(*mob);

  // Subscribe to player's events
  combatPtr->Subscribe(*GetLocalPlayer());
  timeFreezePtr->Subscribe(*GetLocalPlayer());

  // Spawn and subscribe to remote player's events
  SpawnRemotePlayer(props.remotePlayer, props.remoteBlocks);

  // Tell everything to begin battle
  BroadcastBattleStart();
}

NetworkBattleScene::~NetworkBattleScene()
{
}

void NetworkBattleScene::OnHit(Entity& victim, const Hit::Properties& props)
{
  auto player = GetLocalPlayer();
  if (player.get() == &victim && props.damage > 0) {
    if (props.damage >= 300) {
      player->SetEmotion(Emotion::angry);
      GetSelectedCardsUI().SetMultiplier(2);
    }

    if (player->IsSuperEffective(props.element)) {
      // deform
      trackedForms[0]->animationComplete = false;
      trackedForms[0]->selectedForm = -1;
    }
  }

  if (victim.IsSuperEffective(props.element) && props.damage > 0) {
    auto seSymbol = std::make_shared<ElementalDamage>();
    seSymbol->SetLayer(-100);
    seSymbol->SetHeight(victim.GetHeight() + (victim.getLocalBounds().height * 0.5f)); // place it at sprite height
    GetField()->AddEntity(seSymbol, victim.GetTile()->GetX(), victim.GetTile()->GetY());
  }
}

void NetworkBattleScene::onUpdate(double elapsed) {
  if (!IsSceneInFocus()) return;

  SendPingSignal();

  bool skipFrame = IsRemoteBehind() && this->remotePlayer && !this->remotePlayer->IsDeleted();

  // std::cout << "remoteInputQueue size is " << remoteInputQueue.size() << std::endl;

  if (skipFrame && FrameNumber()-resyncFrameNumber >= 5) {
    combatPtr->SkipFrame();
    timeFreezePtr->SkipFrame();
  }
  else {
    if (combatPtr->IsStateCombat(GetCurrentState())) {
      frame_time_t currLag = frames(5); // frame_time_t::max(frames(5), from_milliseconds(packetProcessor->GetAvgLatency()));
      auto events = ProcessLocalPlayerInputQueue(currLag);
      SendFrameData(events);
    }
  }
  
  if (!remoteInputQueue.empty() && combatPtr->IsStateCombat(GetCurrentState())) {
    auto frame = remoteInputQueue.begin();

    // only log desyncs if we are beyond the initial delay frames and the numbers are not in sync yet
    if (FrameNumber() - resyncFrameNumber >= 5 && FrameNumber() != frame->frameNumber) {
      // for debugging, this should never appear if the code is working properly
      Logger::Logf(LogLevel::debug, "DESYNC: frames #s were %i - %i", remoteFrameNumber, FrameNumber());
    }

    if(FrameNumber() >= frame->frameNumber) {
      auto& events = frame->events;
      remoteFrameNumber = frame->frameNumber;

      // Logger::Logf("next remote frame # is %i", remoteFrameNumber);

      for (auto& e : events) {
        remotePlayer->InputState().VirtualKeyEvent(e);
      }

      frame = remoteInputQueue.erase(frame);
    }
  }

  BattleSceneBase::onUpdate(elapsed);
  
  //if (combatPtr->IsStateCombat(GetCurrentState()) && outEvents.has_value()) {
  //}

  frame_time_t elapsed_frames = from_seconds(elapsed);

  if (!syncStatePtr->IsSynchronized()) {
    if (packetProcessor->IsHandshakeAck() && remoteState.remoteHandshake) {
      syncStatePtr->Synchronize();
    }
  }
  else {
    packetTime += elapsed_frames;
  }

  if (skipFrame) return;

  if (remotePlayer && remotePlayer->WillEraseEOF()) {
    auto iter = std::find(players.begin(), players.end(), remotePlayer);
    if (iter != players.end()) {
      players.erase(iter);
    }
    remoteState.remoteConnected = false;
    remotePlayer = nullptr;
  }

  if (auto player = GetLocalPlayer()) {
    BattleResultsObj().finalEmotion = player->GetEmotion();
  }
}

void NetworkBattleScene::onDraw(sf::RenderTexture& surface) {
  BattleSceneBase::onDraw(surface);

  auto lag = GetAvgLatency();

  // draw network ping
  ping.SetString(std::to_string(lag).substr(0, 5));

  //the bounds has been changed after changing the text
  auto bounds = ping.GetLocalBounds();
  ping.setOrigin(bounds.width, bounds.height);

  // draw ping
  surface.draw(ping);

  frameNumText.SetString("F" + std::to_string(FrameNumber()));

  bounds = frameNumText.GetLocalBounds();
  frameNumText.setOrigin(bounds.width, bounds.height);

  frameNumText.SetColor(sf::Color::Cyan);
  frameNumText.setPosition(480 - (2.f * 128) - 4, 320 - 2.f);
  surface.draw(frameNumText);

  frameNumText.SetString("F" + std::to_string(remoteFrameNumber));

  bounds = frameNumText.GetLocalBounds();
  frameNumText.setOrigin(bounds.width, bounds.height);

  frameNumText.SetColor(sf::Color::Magenta);
  frameNumText.setPosition(480 - (2.f * 64) - 4, 320 - 2.f);
  surface.draw(frameNumText);

  // convert from ms to seconds to discrete frame count...
  frame_time_t lagTime = frame_time_t{ (long long)(lag) };

  // use ack to determine connectivity here
  UpdatePingIndicator(lagTime);

  //draw
  surface.draw(pingIndicator);
}

void NetworkBattleScene::onExit()
{
}

void NetworkBattleScene::onEnter()
{
}

void NetworkBattleScene::onStart()
{
  BattleSceneBase::onStart();
  packetProcessor->EnableKickForSilence(true);
}

void NetworkBattleScene::onResume()
{
}

void NetworkBattleScene::onEnd()
{
  BattleSceneBase::onEnd();
}

const NetPlayFlags& NetworkBattleScene::GetRemoteStateFlags()
{
  return remoteState; 
}

const double NetworkBattleScene::GetAvgLatency() const
{
  return packetProcessor->GetAvgLatency();
}

bool NetworkBattleScene::IsRemoteBehind()
{
  return FrameNumber() > this->maxRemoteFrameNumber;
}

void NetworkBattleScene::SendHandshakeSignal()
{
  /**
  To begin the round, we need to supply the following information to our opponent:
    1. Our selected form
    2. Our card list size
    3. Our card list items

  We need to also measure latency to synchronize client animation with
  remote animations (see: combos and forms)
  */

  int form = trackedForms[0]->selectedForm;
  unsigned thisFrame = this->FrameNumber();
  auto& selectedCardsWidget = this->GetSelectedCardsUI();
  std::vector<std::string> uuids = selectedCardsWidget.GetUUIDList();
  size_t len = uuids.size();

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals signalType{ NetPlaySignals::handshake };
  buffer.append((char*)&signalType, sizeof(NetPlaySignals));
  buffer.append((char*)&thisFrame, sizeof(unsigned));
  buffer.append((char*)&form, sizeof(int));
  buffer.append((char*)&len, sizeof(size_t));

  for (auto& id : uuids) {
    size_t len = id.size();
    buffer.append((char*)&len, sizeof(size_t));
    buffer.append(id.c_str(), len);
  }

  auto [_, id] = packetProcessor->SendPacket(Reliability::Reliable, buffer);
  packetProcessor->UpdateHandshakeID(id);
}

void NetworkBattleScene::SendFrameData(std::vector<InputEvent>& events)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals signalType{ NetPlaySignals::frame_data };
  buffer.append((char*)&signalType, sizeof(NetPlaySignals));

  unsigned int frameNumber = FrameNumber() + 5u; // std::max(5u, from_milliseconds(packetProcessor->GetAvgLatency()).count());
  buffer.append((char*)&frameNumber, sizeof(unsigned int));

  // Send our hp
  int hp = 0;
  if (auto player = GetLocalPlayer()) {
    hp = player->GetHealth();
  }

  buffer.append((char*)&hp, sizeof(int));

  // send the input keys
  size_t list_len = events.size();
  buffer.append((char*)&list_len, sizeof(size_t));

  while (list_len > 0) {
    size_t len = events[list_len-1].name.size();
    buffer.append((char*)&len, sizeof(size_t));
    buffer.append(events[list_len-1].name.c_str(), len);
    buffer.append((char*)&events[list_len-1].state, sizeof(InputState));
    list_len--;
  }

  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
  packetTime = frames(0);
  events.clear();
}

void NetworkBattleScene::SendChangedFormSignal(const int form)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::form };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&form, sizeof(int));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void NetworkBattleScene::SendPingSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::ping };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  packetProcessor->SendPacket(Reliability::ReliableOrdered, buffer);
}

void NetworkBattleScene::RecieveHandshakeSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.remoteConnected) return;

  remoteInputQueue.clear();
  FlushLocalPlayerInputQueue();

  std::vector<std::string> remoteUUIDs;
  int remoteForm{ -1 };
  size_t cardLen{};
  size_t read{};

  std::memcpy(&remoteFrameNumber, buffer.begin(), sizeof(unsigned));
  maxRemoteFrameNumber = remoteFrameNumber;
  read += sizeof(unsigned);

  std::memcpy(&remoteForm, buffer.begin() + read, sizeof(int));
  read += sizeof(int);

  std::memcpy(&cardLen, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  while (cardLen > 0) {
    std::string uuid;
    size_t len{};
    std::memcpy(&len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);
    uuid.resize(len);
    std::memcpy(uuid.data(), buffer.begin() + read, len);
    read += len;
    remoteUUIDs.push_back(uuid);
    cardLen--;
  }

  // Now that we have the remote's form and cards,
  // populate the net play remote state with this information
  // and kick off the battle sequence

  //remoteState.remoteFormSelect = remoteForm;
  remoteState.remoteChangeForm = remoteState.remoteFormSelect != remoteForm;
  remoteState.remoteFormSelect = remoteForm;
  trackedForms[1]->selectedForm = remoteForm;
  trackedForms[1]->animationComplete = !remoteState.remoteChangeForm; // a value of false forces animation to play

  remoteHand.clear();

  size_t handSize = remoteUUIDs.size();
  int len = (int)handSize; // TODO: use size_t for card lengths...

  if (handSize) {
    auto& packageManager = getController().CardPackageManager();

    for (size_t i = 0; i < handSize; i++) {
      Battle::Card card;
      std::string id = remoteUUIDs[i];

      if (packageManager.HasPackage(id)) {
        card = packageManager.FindPackageByID(id).GetCardProperties();
      }

      remoteHand.push_back(card);
    }
  }

  // Prepare for simulation
  cardComboStatePtr->Reset();
  double duration{};

  // simulate PA to calculate time required to animate
  while (!cardComboStatePtr->IsDone()) {
    constexpr double step = 1.0 / frame_time_t::frames_per_second;
    cardComboStatePtr->Simulate(step, remoteHand, false);
    duration += step;
  }

  // Prepare for next PA
  cardComboStatePtr->Reset();

  // Filter support cards
  FilterSupportCards(remoteHand);

  // Supply the final hand info
  remoteCardActionUsePublisher->LoadCards(remoteHand);
  
  // Convert to microseconds and use this as the round start delay
  roundStartDelay = from_milliseconds((long long)((duration*1000.0) + packetProcessor->GetAvgLatency()));

  // startStatePtr->SetStartupDelay(roundStartDelay);
  startStatePtr->SetStartupDelay(frames(5));

  remoteState.remoteHandshake = true;
}

void NetworkBattleScene::RecieveFrameData(const Poco::Buffer<char>& buffer)
{
  if (!this->remotePlayer) return;

  std::string name;
  size_t len{}, list_len{};
  size_t read{};

  unsigned int frameNumber{};
  std::memcpy(&frameNumber, buffer.begin(), sizeof(unsigned int));
  read += sizeof(unsigned int);

  maxRemoteFrameNumber = frameNumber;

  int hp{};
  std::memcpy(&hp, buffer.begin() + read, sizeof(int));
  read += sizeof(int);

  std::memcpy(&list_len, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  std::vector<InputEvent> events;
  while (list_len-- > 0) {
    std::memcpy(&len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);

    name.clear();
    name.resize(len);
    std::memcpy(name.data(), buffer.begin() + read, len);
    read += len;

    InputState state{};
    std::memcpy(&state, buffer.begin() + read, sizeof(InputState));
    read += sizeof(InputState);

    InputEvent event{};
    event.name = name;
    event.state = state;
    events.push_back(event);
  }

  remoteInputQueue.push_back({ frameNumber, events });
  
  if (remotePlayer) {
    remotePlayer->SetHealth(hp);
  }
}

void NetworkBattleScene::SpawnRemotePlayer(std::shared_ptr<Player> newRemotePlayer, const std::vector<std::string>& remoteBlocks)
{
  if (remotePlayer) return;

  if (!newRemotePlayer) {
    Logger::Logf(LogLevel::critical, "PVP tried to spawn a nullptr remote player!");
    return;
  }

  remotePlayer = newRemotePlayer;
  remoteState.remoteConnected = true;

  Logger::Logf(LogLevel::debug, "Spawning remote navi: %s", remotePlayer->GetName().c_str());

  remotePlayer->SetTeam(Team::blue);
  remotePlayer->ChangeState<FadeInState<Player>>([]{});

  GetField()->AddEntity(remotePlayer, remoteState.remoteTileX, remoteState.remoteTileY);
  mob->Track(remotePlayer);

  remoteCardActionUsePublisher = remotePlayer->CreateComponent<PlayerSelectedCardsUI>(remotePlayer, &getController().CardPackageManager());
  remoteCardActionUsePublisher->Hide(); // do not reveal opponent's cards

  combatPtr->Subscribe(*remotePlayer);
  combatPtr->Subscribe(*remoteCardActionUsePublisher);
  timeFreezePtr->Subscribe(*remotePlayer);
  timeFreezePtr->Subscribe(*remoteCardActionUsePublisher);

  this->SubscribeToCardActions(*remoteCardActionUsePublisher);

  remotePlayer->CreateComponent<MobHealthUI>(remotePlayer);

  players.push_back(remotePlayer); // TODO: remove this now that we have TrackOtherPlayer()
  this->TrackOtherPlayer(remotePlayer);
  trackedForms.push_back(std::make_shared<TrackedFormData>(remotePlayer.get(), -1, false ));

  // Run block programs on the remote player now that they are spawned
  BlockPackageManager& blockPackages = getController().BlockPackageManager();
  for (const std::string& blockID : remoteBlocks) {
    if (!blockPackages.HasPackage(blockID)) continue;

    auto& blockMeta = blockPackages.FindPackageByID(blockID);
    blockMeta.mutator(*remotePlayer);
  }
}

void NetworkBattleScene::RecieveChangedFormSignal(const Poco::Buffer<char>& buffer)
{
  if (buffer.empty()) return;
  int form = remoteState.remoteFormSelect;
  int prevForm = remoteState.remoteFormSelect;
  std::memcpy(&form, buffer.begin(), sizeof(int));

  if (remotePlayer && form != prevForm) {
    remoteState.remoteFormSelect = form;

    // TODO: hacky and ugly
    this->trackedForms[1]->selectedForm = remoteState.remoteFormSelect;
    this->trackedForms[1]->animationComplete = false; // kick-off animation
  }
}

void NetworkBattleScene::ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>& body)
{
  try {
    switch (header) {
      case NetPlaySignals::handshake:
        RecieveHandshakeSignal(body);
        break;
      case NetPlaySignals::frame_data:
        RecieveFrameData(body);
        break;
      case NetPlaySignals::form:
        RecieveChangedFormSignal(body);
        break;
    }
  }
  catch (std::exception& e) {
    Logger::Logf(LogLevel::critical, "PVP Network exception: %s", e.what());
    packetProcessor->HandleError();
  }
}

void NetworkBattleScene::UpdatePingIndicator(frame_time_t frames)
{
  unsigned int idx{};
  const unsigned int count = frames.count();

  if (count >= 20) {
    // 1st frame is red - no connection for a significant fraction of the frame
    // innevitable desync here
    idx = 1;
  }
  else if (count >= 5) {
    // 2nd frame is weak - not strong and prone to desync
    idx = 2;
  }
  else if (count >= 3) {
    // 3rd frame is average - ok but not best
    idx = 3; 
  }
  else  {
    // 4th frame is excellent or zero latency 
    idx = 4;
  }

  pingIndicator.setTextureRect(sf::IntRect((idx-1u)*16, 0, 16, 16));
}