#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>
#include <SFML/Window/Clipboard.hpp>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/IPAddress.h>

#include "bnPVPScene.h"
#include "battlescene/bnNetworkBattleScene.h"

#include "Segues/PushIn.h"

#include "../bnMainMenuScene.h"
#include "../bnGridBackground.h"
#include "../bnAudioResourceManager.h"
#include "../bnSecretBackground.h"

using namespace Poco;
using namespace Net;
using namespace swoosh::types;

std::string PVPScene::myIP = "";

PVPScene::PVPScene(swoosh::ActivityController& controller, int selected, CardFolder& folder, PA& pa)
  : textbox(sf::Vector2f(4, 250)), selectedNavi(selected), folder(folder), pa(pa),
  uiAnim("resources/ui/pvp_widget.animation"),
  swoosh::Activity(&controller)
{
  // network
  netplayconfig.myPort = ENGINE.CommandLineValue<int>("port");
  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), netplayconfig.myPort);
  client = Poco::Net::DatagramSocket(sa);
  client.setBlocking(false);

  // Sprites
  ui.setTexture(LOAD_TEXTURE_FILE("resources/ui/pvp_widget.png"));
  vs.setTexture(LOAD_TEXTURE_FILE("resources/ui/vs_text.png"));
  vsFaded.setTexture(LOAD_TEXTURE_FILE("resources/ui/vs_text.png"));
  greenBg.setTexture(LOAD_TEXTURE(FOLDER_VIEW_BG));

  navigator.setTexture(LOAD_TEXTURE(MUG_NAVIGATOR));
  //navigator.setScale(2.f, 2.f);

  // NOTE: ui sprites are already at 2x scale
  // ui.setScale(2.f, 2.f);

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
  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  text.setFont(*font);
  text.setPosition(45.f, -5.f); // y starts above the visible screen because sfml fonts are weird

  id.setFont(*font);

  // load animation files
  uiAnim.Load();

  setView(sf::Vector2u(480, 320));
}

PVPScene::~PVPScene() {
  delete gridBG;
}

const std::string PVPScene::GetPublicIP()
{
  std::string url = "checkip.amazonaws.com"; // send back public IP in plain text

  try {
    HTTPClientSession session(url);
    HTTPRequest request(HTTPRequest::HTTP_GET, "/", HTTPMessage::HTTP_1_1);
    HTTPResponse response;

    session.sendRequest(request);
    std::istream& rs = session.receiveResponse(response);

    if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
    {
      std::string temp = std::string(std::istreambuf_iterator<char>(rs), {});
      temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
      return temp;
    }
  }
  catch (std::exception& e) {
    Logger::Logf("PVP Network Exception while obtaining IP: %s", e.what());
  }

  // failed 
  return "";
}

void PVPScene::HandleInfoMode()
{
  textbox.ClearAllMessages();
  Message* help = new Message("CTRL + C to copy your IP address");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
}

void PVPScene::HandleJoinMode()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Paste your opponent's IP address");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
}

void PVPScene::HandleReady()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Waiting for " + theirIP);
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
}

void PVPScene::HandleCancel()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Cancelled...");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open();
  textbox.CompleteCurrentBlock();
  AUDIO.Play(AudioType::CHIP_ERROR);
}

void PVPScene::HandleGetIPFailure()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Error obtaining your IP");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open([]() {
    AUDIO.Play(AudioType::CHIP_ERROR);
  });
  textbox.CompleteCurrentBlock();
}

void PVPScene::HandleCopyEvent()
{
  std::string value = INPUTx.GetClipboard();

  if (value != myIP) {
    Message* help = nullptr;
    if (myIP.empty()) {
      AUDIO.Play(AudioType::CHIP_ERROR);
      help = new Message("IP addr unavailable");
    }
    else {
      INPUTx.SetClipboard(myIP);
      AUDIO.Play(AudioType::NEW_GAME);
      help = new Message("Copied!");
    }

    textbox.ClearAllMessages();
    textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
    textbox.CompleteCurrentBlock();
  }
}

void PVPScene::HandlePasteEvent()
{
  std::string value = INPUTx.GetClipboard();

  if (value != theirIP) {
    Message* help = nullptr;

    if (IsValidIPv4(value)) {
      theirIP = value;
      AUDIO.Play(AudioType::COUNTER_BONUS);
      help = new Message("Pasted! Press start to connect.");
    }
    else {
      AUDIO.Play(AudioType::CHIP_ERROR);
      help = new Message("Bad IP");
    }

    textbox.ClearAllMessages();
    textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
    textbox.CompleteCurrentBlock();
  }
}

void PVPScene::ProcessIncomingPackets()
{
  if (!client.poll(Poco::Timespan{ 0 }, Poco::Net::Socket::SELECT_READ)) return;
  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };
  static int read = 0;

  try {
    // discover their IP
    if (theirIP.empty()) {
      Poco::Net::SocketAddress sender;
      read = client.receiveFrom(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN - 1, sender);
      rawBuffer[read] = '\0';

      theirIP = std::string(rawBuffer, read);
    }

    read += client.receiveBytes(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN - 1);
    if (read > 0) {
      rawBuffer[read] = '\0';

      NetPlaySignals sig = *(NetPlaySignals*)rawBuffer;
      size_t sigLen = sizeof(NetPlaySignals);
      Poco::Buffer<char> data{ 0 };
      data.append(rawBuffer + sigLen, size_t(read) - sigLen);

      switch (sig) {
      case NetPlaySignals::handshake:
        RecieveHandshakeSignal();
        break;
      case NetPlaySignals::connect:
        RecieveConnectSignal(data);
        break;
      }
    }
  }
  catch (std::exception& e) {
    Logger::Logf("PVP Network exception: %s", e.what());
  }

  read = 0;
  std::memset(rawBuffer, 0, NetPlayConfig::MAX_BUFFER_LEN);
}

void PVPScene::SendConnectSignal(const int navi)
{
  try {
    Poco::Buffer<char> buffer{ 0 };
    NetPlaySignals type{ NetPlaySignals::connect };
    buffer.append((char*)&type, sizeof(NetPlaySignals));
    buffer.append((char*)&navi, sizeof(size_t));
    client.sendBytes(buffer.begin(), (int)buffer.size());
  }
  catch (...) {
    // bad IP?
  }
}

void PVPScene::SendHandshakeSignal()
{
  try {
    Poco::Buffer<char> buffer{ 0 };
    NetPlaySignals type{ NetPlaySignals::handshake };
    buffer.append((char*)&type, sizeof(NetPlaySignals));
    client.sendBytes(buffer.begin(), (int)buffer.size());
  }
  catch (...) {
    // bad IP?
  }
}

void PVPScene::RecieveConnectSignal(const Poco::Buffer<char>& buffer)
{
  if (remoteIsReady) return; // prevent multiple connection requests...

  remoteIsReady = true;

  size_t navi = size_t{ 0 }; std::memcpy(&navi, buffer.begin(), buffer.size());
  auto meta = NAVIS.At(static_cast<int>(navi));
  this->remotePreview.setTexture(meta.GetPreviewTexture());
  auto height = remotePreview.getSprite().getLocalBounds().height;
  remotePreview.setOrigin(sf::Vector2f(0, height));
}

void PVPScene::RecieveHandshakeSignal()
{
  this->handshakeComplete = true;
  this->SendHandshakeSignal();
}

void PVPScene::DrawIDInputWidget()
{
  uiAnim.SetAnimation("ID_START");
  uiAnim.SetFrame(0, ui.getSprite());
  ui.setPosition(100, 40);
  ENGINE.Draw(ui);

  if (infoMode) {
    id.setString(sf::String(myIP));
  }
  else {
    id.setString(sf::String(theirIP));
  }
  id.setPosition(145, 40);

  // restrict the widget from collapsing at text length 0
  float widgetWidth = std::fmax(id.getLocalBounds().width+10.f, 50.f);

  uiAnim.SetAnimation("ID_MID");
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(widgetWidth, 1.0f);
  ui.setPosition(140, 40);
  ENGINE.Draw(ui);
  ENGINE.Draw(id);

  uiAnim.SetAnimation("ID_END");
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(1.f, 1.0f);
  ui.setPosition(140 + widgetWidth, 40);
  ENGINE.Draw(ui);
}

void PVPScene::DrawCopyPasteWidget()
{
  std::string state = "ENABLED";
  std::string clipboard = INPUTx.GetClipboard();

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
  ENGINE.Draw(ui);

  sf::Text widgetText;
  widgetText.setFont(*font);

  if (infoMode) {
    widgetText.setString("Copy");
    icon += "COPY";
  }
  else {
    widgetText.setString("Paste");
    icon += "PASTE";
  }

  uiAnim.SetAnimation(icon);
  uiAnim.SetFrame(0, ui.getSprite());
  ui.setPosition(102, 92); // offset by 2 pixels to fit inside the frame
  ENGINE.Draw(ui);

  widgetText.setPosition(145, 83);

  float widgetWidth = widgetText.getLocalBounds().width + 10.f;

  uiAnim.SetAnimation(mid);
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(widgetWidth, 1.0f);
  ui.setPosition(140, 90);
  ENGINE.Draw(ui);
  ENGINE.Draw(widgetText);

  uiAnim.SetAnimation(end);
  uiAnim.Update(0, ui.getSprite());
  ui.setScale(1.f, 1.0f);
  ui.setPosition(140 + widgetWidth, 90);
  ENGINE.Draw(ui);
}

const bool PVPScene::IsValidIPv4(const std::string& ip) const {
  Poco::Net::IPAddress temp;
  return Poco::Net::IPAddress::tryParse(ip, temp);
}

void PVPScene::Reset()
{
  leave = false; /*!< Scene state coming/going flag */
  remoteIsReady = false;
  clientIsReady = false;
  isInFlashyVSIntro = false;
  isInBattleStartup = false;
  handshakeComplete = false;
  playVS = true;
  sequenceTimer = 0.0; // in seconds
  flashCooldown = 0;

  this->isScreenReady = true;

  // minor optimzation
  if (myIP.empty()) {
    myIP = GetPublicIP();
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
}

void PVPScene::onStart() {
  Reset();
}

void PVPScene::onResume() {
  Reset();
}

void PVPScene::onExit()
{

}

void PVPScene::onUpdate(double elapsed) {
  gridBG->Update(float(elapsed));
  textbox.Update(float(elapsed));

  // DEBUG SCENE STUFF
  if (INPUTx.Has(EventTypes::RELEASED_SPECIAL)) {
    clientIsReady = remoteIsReady = true;
    remotePreview.setTexture(clientPreview.getTexture());
    remotePreview.setOrigin(0, remotePreview.getLocalBounds().height);
    theirIP = "127.0.0.1";
  }

  if (!isScreenReady || leave) return; // don't update our scene if not fully in view from segue

  if (clientIsReady && remoteIsReady && !isInFlashyVSIntro) {
    isInFlashyVSIntro = true;
    AUDIO.StopStream();
  }
  else if (clientIsReady && !remoteIsReady) {
    if (INPUTx.Has(EventTypes::PRESSED_CANCEL)) {
      clientIsReady = false;
      HandleCancel();
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
          AUDIO.Play(AudioType::CUSTOM_BAR_FULL, AudioPriority::highest);
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
      isInBattleStartup = true;
    }
  }
  else if (isInBattleStartup) {
    leave = true;

    // Shuffle our folder
    CardFolder* copy = folder.Clone();
    copy->Shuffle();

    // Queue screen transition to Battle Scene with a white fade effect
    // just like the game
    using effect = segue<WhiteWashFade>;
    NetPlayConfig config;

    // Play the pre battle sound
    AUDIO.Play(AudioType::PRE_BATTLE, AudioPriority::high);

    // Stop music and go to battle screen 
    AUDIO.StopStream();

    // Configure the session
    config.remoteIP = theirIP;

    Player* player = NAVIS.At(selectedNavi).GetNavi();

    // TODO: reuse this connection
    client.close();

    NetworkBattleSceneProps props = {
      { *player, pa, copy, new Field(6, 3), new SecretBackground() },
      config
    };

    getController().push<effect::to<NetworkBattleScene>>(props);
  } else {
    ProcessIncomingPackets();

    if (!handshakeComplete && !theirIP.empty()) {
      // Try reaching out to someone...
      this->SendConnectSignal(selectedNavi);
    }

    if (remoteIsReady && !clientIsReady) {
      this->SendHandshakeSignal();
    }

    if (INPUTx.Has(EventTypes::PRESSED_CANCEL)) {
      leave = true;
      AUDIO.Play(AudioType::CHIP_CANCEL);
      client.close();
      using effect = segue<PushIn<direction::up>, milliseconds<500>>;
      getController().pop<effect>();
    }
    else if (INPUTx.HasSystemCopyEvent() && infoMode) {
      HandleCopyEvent();
    }
    else if (INPUTx.HasSystemPasteEvent() && !infoMode) {
      HandlePasteEvent();
    }
    else if (INPUTx.Has(EventTypes::PRESSED_SCAN_LEFT) && infoMode) {
      infoMode = false;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      HandleJoinMode();
    }
    else if (INPUTx.Has(EventTypes::PRESSED_SCAN_RIGHT) && !infoMode) {
      infoMode = true;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      HandleInfoMode();
    } 
    else if (INPUTx.Has(EventTypes::PRESSED_CONFIRM) && !infoMode && !theirIP.empty()) {
      this->clientIsReady = true;
      HandleReady();
      AUDIO.Play(AudioType::CHIP_CHOOSE);
    }
  }
}

void PVPScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(greenBg);
  ENGINE.Draw(gridBG);

  // Draw the widget pieces
  //ENGINE.Draw(ui);
  //uiAnim.SetAnimation("...")

  if (!isInFlashyVSIntro) {
    ENGINE.Draw(textbox);

    if (infoMode && myIP.empty()) {
      id.setString(sf::String("ERROR"));
    }

    if (infoMode) {
      text.setString("My Info");

      uiAnim.SetAnimation("L");
      uiAnim.SetFrame(0, ui.getSprite());
      ui.setPosition(5, 4);
    }
    else {
      text.setString("Remote");

      uiAnim.SetAnimation("R");
      uiAnim.SetFrame(0, ui.getSprite());
      ui.setPosition(130, 4);
    }

    ENGINE.Draw(text);

    // L/R icons
    ENGINE.Draw(ui);

    this->DrawIDInputWidget();
    this->DrawCopyPasteWidget();
  }
  else {
    ENGINE.Draw(clientPreview);
    ENGINE.Draw(remotePreview);
    ENGINE.Draw(vs);
    ENGINE.Draw(vsFaded);
  }

  if (flashCooldown > 0) {
    sf::RectangleShape screen(ENGINE.GetCamera()->GetView().getSize());
    screen.setFillColor(sf::Color::White);
    ENGINE.Draw(screen);
  }
}
