#include <cmath>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>
#include <SFML/Window/Clipboard.hpp>
#include <Segues/PushIn.h>
#include <Segues/WhiteWashFade.h>

#include "bnMatchMakingScene.h"
#include "bnDownloadScene.h"
#include "../bnGridBackground.h"
#include "../bnAudioResourceManager.h"
#include "../bnSecretBackground.h"
#include "../bnMessage.h"
#include "battlescene/bnNetworkBattleScene.h"

using namespace swoosh::types;

MatchMakingScene::MatchMakingScene(swoosh::ActivityController& controller, int selected, CardFolder& folder, PA& pa) : 
  textbox(sf::Vector2f(4, 250)), 
  selectedNavi(selected), 
  folder(folder), pa(pa),
  uiAnim("resources/ui/pvp_widget.animation"),
  text(Font::Style::thick),
  id(Font::Style::thick),
  Scene(controller)
{
  // Sprites
  ui.setTexture(Textures().LoadTextureFromFile("resources/ui/pvp_widget.png"));
  vs.setTexture(Textures().LoadTextureFromFile("resources/ui/vs_text.png"));
  vsFaded.setTexture(Textures().LoadTextureFromFile("resources/ui/vs_text.png"));
  greenBg.setTexture(Textures().GetTexture(TextureType::FOLDER_VIEW_BG));
  navigator.setTexture(Textures().GetTexture(TextureType::MUG_NAVIGATOR));

  float w = static_cast<float>(controller.getVirtualWindowSize().x);
  float h = static_cast<float>(controller.getVirtualWindowSize().y);
  vs.setPosition(w / 2.f, h / 2.f);
  swoosh::game::setOrigin(vs.getSprite(), 0.5, 0.5);
  
  // hide this until it is ready
  vs.setScale(0.f, 0.f);

  // this is the fadeout VS effect
  swoosh::game::setOrigin(vsFaded.getSprite(), 0.5, 0.5);
  vsFaded.setPosition(w / 2.f, h / 2.f);
  vsFaded.setScale(0.f, 0.f);

  greenBg.setScale(2.f, 2.f);

  this->gridBG = new GridBackground();
  gridBG->setColor(sf::Color(0)); // hide until it is ready

  clientPreview.setTexture(NAVIS.At(selectedNavi).GetPreviewTexture());
  clientPreview.setScale(2.f, 2.f);
  clientPreview.setOrigin(clientPreview.getLocalBounds().width, clientPreview.getLocalBounds().height);
  
  // flip the remote player
  remotePreview.setScale(-2.f, 2.f);

  // text / font
  text.setPosition(45.f, 4.f); 

  // upscale text
  text.setScale(2.f, 2.f);
  id.setScale(2.f, 2.f);

  // load animation files
  uiAnim.Load();

  using namespace std::placeholders;
  packetProcessor = std::make_shared<MatchMaking::PacketProcessor>();
  packetProcessor->SetPacketBodyCallback(std::bind(&MatchMakingScene::ProcessPacketBody, this, _1, _2));

  setView(sf::Vector2u(480, 320));
}

MatchMakingScene::~MatchMakingScene() {
  delete gridBG;
}

void MatchMakingScene::HandleInfoMode()
{
  textbox.ClearAllMessages();
  Message* help = new Message("CTRL + C to copy your IP address");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
}

void MatchMakingScene::HandleJoinMode()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Paste your opponent's IP address");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
}

void MatchMakingScene::HandleReady()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Waiting for " + theirIP);
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
}

void MatchMakingScene::HandleCancel()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Cancelled...");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
  Audio().Play(AudioType::CHIP_ERROR);
}

void MatchMakingScene::HandleGetIPFailure()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Error obtaining your IP");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open([=]() {
    Audio().Play(AudioType::CHIP_ERROR);
  });
  textbox.CompleteCurrentBlock();
}

void MatchMakingScene::HandleCopyEvent()
{
  std::string value = Input().GetClipboard();

  if (value != myIP) {
    Message* help = nullptr;
    if (myIP.empty()) {
      Audio().Play(AudioType::CHIP_ERROR);
      help = new Message("IP addr unavailable");
    }
    else {
      Input().SetClipboard(myIP);
      Audio().Play(AudioType::NEW_GAME);
      help = new Message("Copied!");
    }

    textbox.ClearAllMessages();
    textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
    textbox.CompleteCurrentBlock();
  }
}

void MatchMakingScene::HandlePasteEvent()
{
  std::string value = Input().GetClipboard();

  // assume true to avoid a series if-else scenarios
  bool hasError = true;

  if (value != theirIP) {
    Message* help = nullptr;

    if (IsValidIPv4(value)) {
      packetProcessor->SetNewRemote(value);

      if (packetProcessor->RemoteAddrIsValid()) {
        theirIP = value;
        Audio().Play(AudioType::COUNTER_BONUS);
        help = new Message("Pasted! Press start to connect.");

        // If everything goes well, we have no error with the address...
        hasError = false;
      } 
    }
    
    if(hasError) {
      Audio().Play(AudioType::CHIP_ERROR);
      help = new Message("Bad IP");
    }

    textbox.ClearAllMessages();
    textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
    textbox.CompleteCurrentBlock();
  }
}

void MatchMakingScene::ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>& body)
{
  try {
    switch (header) {
    case NetPlaySignals::matchmaking_handshake:
      RecieveHandshakeSignal();
      break;
    case NetPlaySignals::matchmaking_request:
      RecieveConnectSignal(body);
      break;
    }
  }
  catch (std::exception& e) {
    Logger::Logf("Match Making exception: %s", e.what());
  }
}

void MatchMakingScene::SendConnectSignal(size_t navi)
{
  try {
    Poco::Buffer<char> buffer{ 0 };


    NetPlaySignals type{ NetPlaySignals::matchmaking_request };
    buffer.append((char*)&type, sizeof(NetPlaySignals));
    buffer.append((char*)&navi, sizeof(size_t));
    packetProcessor->SendPacket(Reliability::Unreliable, buffer);
  }
  catch (Poco::IOException& e) {
    Logger::Logf("IOException when trying to connect to opponent: %s", e.what());
  }
}

void MatchMakingScene::SendHandshakeSignal()
{
  try {
    Poco::Buffer<char> buffer{ 0 };

    // mark unreliable, in case we leak into the next scene
    NetPlaySignals type{ NetPlaySignals::matchmaking_handshake };
    buffer.append((char*)&type, sizeof(NetPlaySignals));
    packetProcessor->SendPacket(Reliability::Unreliable, buffer);
  }
  catch (Poco::IOException& e) {
    Logger::Logf("IOException when trying to connect to opponent: %s", e.what());
  }
}

void MatchMakingScene::RecieveConnectSignal(const Poco::Buffer<char>& buffer)
{
  if (remoteIsReady || !clientIsReady) return; // prevent multiple connection requests...

  remoteIsReady = true;

  size_t navi = size_t{ 0 }; 
  std::memcpy(&navi, buffer.begin(), sizeof(size_t));
  auto& meta = NAVIS.At(static_cast<int>(navi));
  this->remotePreview.setTexture(meta.GetPreviewTexture());
  auto height = remotePreview.getSprite().getLocalBounds().height;
  remotePreview.setOrigin(sf::Vector2f(0, height));
}

void MatchMakingScene::RecieveHandshakeSignal()
{
  // only acknowledge handshakes if you have recieved the opponent's character data
  if (!remoteIsReady || !clientIsReady) return;

  this->handshakeComplete = true;
  this->SendHandshakeSignal();
}

void MatchMakingScene::DrawIDInputWidget(sf::RenderTexture& surface)
{
  uiAnim.SetAnimation("ID_START");
  uiAnim.SetFrame(0, ui.getSprite());
  ui.setPosition(100, 40);
  surface.draw(ui);

  if (infoMode) {
    id.SetString(sf::String(myIP));
  }
  else {
    id.SetString(sf::String(theirIP));
  }
  id.setPosition(145, 48);

  // restrict the widget from collapsing at text length 0
  float widgetWidth = std::fmax(id.GetLocalBounds().width*2.f +10.f, 50.f);

  uiAnim.SetAnimation("ID_MID");
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(widgetWidth, 1.0f);
  ui.setPosition(140, 40);
  surface.draw(ui);
  surface.draw(id);

  uiAnim.SetAnimation("ID_END");
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(1.f, 1.0f);
  ui.setPosition(140 + widgetWidth, 40);
  surface.draw(ui);
}

void MatchMakingScene::DrawCopyPasteWidget(sf::RenderTexture& surface)
{
  std::string state = "ENABLED";
  std::string clipboard = Input().GetClipboard();

  if (!infoMode && (clipboard.empty() || theirIP == clipboard)) {
    state = "DISABLED";
  }
  else if (!infoMode && theirIP == clipboard) {
    // also disable if already entered
    state = "DISABLED";
  }

  if (infoMode && (myIP.empty() || myIP == clipboard)) {
    state = "DISABLED";
  }

  std::string prefix = "BTN_" + state +"_";
  std::string start = prefix + "START";
  std::string mid = prefix + "MID";
  std::string end = prefix + "END";

  std::string icon = "ICO_" + state + "_";

  uiAnim.SetAnimation(start);
  uiAnim.SetFrame(0, ui.getSprite());
  ui.setPosition(100, 90);
  surface.draw(ui);

  Text widgetText(Font::Style::thick);
  widgetText.setScale(2.f, 2.f);

  if (infoMode) {
    widgetText.SetString("Copy");
    icon += "COPY";
  }
  else {
    widgetText.SetString("Paste");
    icon += "PASTE";
  }

  uiAnim.SetAnimation(icon);
  uiAnim.SetFrame(0, ui.getSprite());
  ui.setPosition(102, 92); // offset by 2 pixels to fit inside the frame
  surface.draw(ui);

  widgetText.setPosition(145, 94);

  float widgetWidth = widgetText.GetLocalBounds().width*2.f + 10.f;

  uiAnim.SetAnimation(mid);
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(widgetWidth, 1.0f);
  ui.setPosition(140, 90);
  surface.draw(ui);
  surface.draw(widgetText);

  uiAnim.SetAnimation(end);
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(1.f, 1.0f);
  ui.setPosition(140 + widgetWidth, 90);
  surface.draw(ui);
}

const bool MatchMakingScene::IsValidIPv4(const std::string& ip) const {
  std::string_view home = "127.0.0.1";
  std::string_view local = "localhost";
  std::string_view colon = ":";

  if (ip.find(home) != std::string::npos || ip.find(local) != std::string::npos) {
    size_t pos = ip.find(colon);
    std::string port = ip.substr(pos);
    
    // Do not connect to ourselves...
    // Note: bad things happen but really shouldn't here, why? 
    //       you can connect fine in overworld...
    if (atoi(port.c_str()) == Net().GetSocket().address().port()) {
      return false;
    }
  }

  /*Poco::Net::IPAddress temp;
  return Poco::Net::IPAddress::tryParse(ip, temp);*/
  return true; // for debugging now...
}

void MatchMakingScene::Reset()
{
  leave = false; /*!< Scene state coming/going flag */
  remoteIsReady = false;
  clientIsReady = false;
  isInFlashyVSIntro = false;
  isInBattleStartup = false;
  handshakeComplete = false;
  hasProcessedCards = false;
  canProceedToBattle = false;
  playVS = true;
  sequenceTimer = 0.0; // in seconds
  flashCooldown = 0;

  this->isScreenReady = true;
  
  // hide this until it is ready
  vs.setScale(0.f, 0.f);

  // minor optimzation
  if (myIP.empty()) {
    myIP = Net().GetPublicIP();
  }

  if (myIP.empty()) {
    // there was a problem
    HandleGetIPFailure();
  }
  else {
    Logger::Logf("My IP came back as %s", myIP.c_str());
    // start on info mode first
    HandleInfoMode();
  }

  greenBg.setColor(sf::Color::White);
  gridBG->setColor(sf::Color(0)); // hide until it is ready

  clientPreview.setPosition(0, 0);
  remotePreview.setPosition(0, 0);
}

void MatchMakingScene::onStart() {
  Reset();
}

void MatchMakingScene::onResume() {
  if (!canProceedToBattle) {
    // If this condition is false, we could not download assets we needed
    // Reset for next match attempt
    Reset();
  }
}

void MatchMakingScene::onExit()
{
  Net().DropProcessor(packetProcessor);
}

void MatchMakingScene::onUpdate(double elapsed) {
  gridBG->Update(double(elapsed));
  textbox.Update(double(elapsed));

  if (!isScreenReady || leave) return; // don't update our scene if not fully in view from segue

  bool systemEvent = Input().HasSystemCopyEvent() || Input().HasSystemPasteEvent();

  if (handshakeComplete && !isInFlashyVSIntro) {
    isInFlashyVSIntro = true;
    Audio().StopStream();
  }
  else if (clientIsReady && !remoteIsReady) {
    if (Input().Has(InputEvents::pressed_cancel) && !systemEvent) {
      clientIsReady = false;
      Net().DropProcessor(packetProcessor);
      HandleCancel();
    }
    else {
      SendConnectSignal(selectedNavi);
    }
  }
  else if (isInFlashyVSIntro && !isInBattleStartup) {
    // use swoosh deltas on the timer to create a sequence
    this->sequenceTimer += elapsed;

    double delta = swoosh::ease::linear(sequenceTimer, 0.5, 1.0);
    sf::Uint8 amt = sf::Uint8(255 * (1 - delta));
    greenBg.setColor(sf::Color(amt,amt,amt,255));

    // Refresh mob graphic origin every frame as it may change
    auto size = getController().getVirtualWindowSize();

    if (delta == 1) {
      delta = swoosh::ease::linear(sequenceTimer - 0.5, 0.5, 1.0);
      float x = float(delta) * 160.0f;

      clientPreview.setPosition(float(delta * size.x) * 0.25f, float(size.y));
      clientPreview.setOrigin(float(clientPreview.getTextureRect().width) * 0.5f, float(clientPreview.getTextureRect().height));

      remotePreview.setPosition(size.x - float(delta * size.x * 0.25f), float(size.y));
      remotePreview.setOrigin(float(remotePreview.getTextureRect().width) * 0.5f, float(remotePreview.getTextureRect().height));

      delta = swoosh::ease::bezierPopIn(sequenceTimer - 0.5, 0.5);
      float scale = std::fabs(float(1.0 - delta) * 20.0f) + 1.f;
      vs.setScale(scale, scale);

      if (delta == 1) {
        // do once
        if (playVS) {
          playVS = false;
          Audio().Play(AudioType::CUSTOM_BAR_FULL, AudioPriority::highest);
          flashCooldown = 20.0 / 60.0;
        }

        flashCooldown -= elapsed;

        // make bg appear
        gridBG->setColor(sf::Color(255, 255, 255, 255));

        if (flashCooldown > 20) {
          delta = swoosh::ease::linear(sequenceTimer - 1.0, 2.0, 1.0);
          float scale = static_cast<float>(delta) + 1.f;
          vsFaded.setScale(scale, scale);
          amt = sf::Uint8(150 * (1 - delta));
          vsFaded.setColor(sf::Color(255, 255, 255, amt));
        }
      }
    }

    if (this->sequenceTimer >= 3) {
      copyScreen = true; // copy this cool preview for the D/L screen...
      isInBattleStartup = true;
    }
  }
  else if (isInBattleStartup) {
    if (!hasProcessedCards) {
      hasProcessedCards = true;

      std::vector<std::string> cardUUIDs;

      CardFolder* copy = folder.Clone();
      auto next = copy->Next();

      while(next) {
        cardUUIDs.push_back(next->GetUUID());
        next = copy->Next();
      }

      delete copy;

      DownloadSceneProps props = {
        canProceedToBattle,
        cardUUIDs,
        packetProcessor->GetRemoteAddr(),
        screen
      };

      getController().push<DownloadScene>(props);
    }
    else if (canProceedToBattle) {
      leave = true;

      // Shuffle our folder
      CardFolder* copy = folder.Clone();
      copy->Shuffle();

      // Queue screen transition to Battle Scene with a white fade effect
      // just like the game
      using effect = segue<WhiteWashFade>;
      NetPlayConfig config;

      // Play the pre battle sound
      Audio().Play(AudioType::PRE_BATTLE, AudioPriority::high);

      // Stop music and go to battle screen 
      Audio().StopStream();

      // Configure the session
      SelectedNavi compatibleNavi = 0;

      // TODO: 
      // For Demo's prevent sending scripted navis over the network
      if (selectedNavi < 6) {
        compatibleNavi = selectedNavi;
      }

      config.remote = theirIP;
      config.myNavi = compatibleNavi;

      auto& meta = NAVIS.At(compatibleNavi);
      const std::string& image = meta.GetMugshotTexturePath();
      const std::string& mugshotAnim = meta.GetMugshotAnimationPath();
      const std::string& emotionsTexture = meta.GetEmotionsTexturePath();
      auto mugshot = Textures().LoadTextureFromFile(image);
      auto emotions = Textures().LoadTextureFromFile(emotionsTexture);
      Player* player = meta.GetNavi();


      NetworkBattleSceneProps props = {
        { *player, pa, copy, new Field(6, 3), std::make_shared<SecretBackground>() },
        sf::Sprite(*mugshot),
        mugshotAnim,
        emotions,
        config
      };

      getController().push<effect::to<NetworkBattleScene>>(props);
    }
  } else {
    // TODO use states/switches this is getting out of hand...
    if (clientIsReady && theirIP.size()) {

      if (!handshakeComplete && !theirIP.empty()) {
        // Try reaching out to someone...
        this->SendConnectSignal(selectedNavi);
        this->SendHandshakeSignal();
      }
    }

    if (Input().Has(InputEvents::pressed_cancel) && !systemEvent) {
      leave = true;
      Audio().Play(AudioType::CHIP_CANCEL);
      using effect = segue<PushIn<direction::up>, milliseconds<500>>;
      getController().pop<effect>();
    }
    else if (Input().HasSystemCopyEvent() && infoMode) {
      HandleCopyEvent();
    }
    else if (Input().HasSystemPasteEvent() && !infoMode) {
      HandlePasteEvent();
    }
    else if (Input().Has(InputEvents::pressed_shoulder_left) && infoMode && !systemEvent) {
      infoMode = false;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      HandleJoinMode();
    }
    else if (Input().Has(InputEvents::pressed_shoulder_right) && !infoMode && !systemEvent) {
      infoMode = true;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      HandleInfoMode();
    } 
    else if (Input().Has(InputEvents::pressed_confirm) && !infoMode && packetProcessor->RemoteAddrIsValid() && !systemEvent) {
      Net().AddHandler(packetProcessor->GetRemoteAddr(), packetProcessor);

      this->clientIsReady = true;
      HandleReady();
      Audio().Play(AudioType::CHIP_CHOOSE);
    }
  }
}

void MatchMakingScene::onLeave()
{
  if (leave) {
    Net().DropProcessor(packetProcessor);
  }
}

void MatchMakingScene::onEnter()
{
  if (leave || !canProceedToBattle) {
    Reset();
  }
}

void MatchMakingScene::onDraw(sf::RenderTexture& surface) {
  surface.draw(greenBg);
  surface.draw(*gridBG);

  if (!isInFlashyVSIntro) {
    surface.draw(textbox);

    if (infoMode && myIP.empty()) {
      id.SetString("ERROR");
    }

    if (infoMode) {
      text.SetString("My Info");

      uiAnim.SetAnimation("L");
      uiAnim.SetFrame(0, ui.getSprite());
      ui.setPosition(5, 4);
    }
    else {
      text.SetString("Remote");

      uiAnim.SetAnimation("R");
      uiAnim.SetFrame(0, ui.getSprite());
      ui.setPosition(130, 4);
    }

    surface.draw(text);

    // L/R icons
    surface.draw(ui);

    this->DrawIDInputWidget(surface);
    this->DrawCopyPasteWidget(surface);
  }
  else {
    surface.draw(clientPreview);
    surface.draw(remotePreview);
    surface.draw(vs);
    surface.draw(vsFaded);
  }

  if (flashCooldown > 0) {
    const sf::Vector2u winSize = getController().getVirtualWindowSize();
    sf::Vector2f size = sf::Vector2f(static_cast<float>(winSize.x), static_cast<float>(winSize.y));
    sf::RectangleShape screen(size);
    screen.setFillColor(sf::Color::White);
    surface.draw(screen);
  }

  if (copyScreen) {
    surface.display();
    screen = surface.getTexture();
    copyScreen = false;
  }
}
