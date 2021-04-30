#include <cmath>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>
#include <SFML/Window/Clipboard.hpp>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/IPAddress.h>
#include <Segues/PushIn.h>

#include "bnPVPScene.h"
#include "../bnGridBackground.h"
#include "../bnAudioResourceManager.h"
#include "../bnSecretBackground.h"
#include "../bnMessage.h"
#include "battlescene/bnNetworkBattleScene.h"

using namespace Poco;
using namespace Net;
using namespace swoosh::types;

std::string PVPScene::myIP = "";

PVPScene::PVPScene(swoosh::ActivityController& controller, int selected, CardFolder& folder, PA& pa) : 
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
  text.setPosition(45.f, 0.f); 

  // upscale text
  text.setScale(2.f, 2.f);
  id.setScale(2.f, 2.f);

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
  Poco::Net::SocketAddress sa(theirIP);
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
  Audio().Play(AudioType::CHIP_ERROR);
}

void PVPScene::HandleGetIPFailure()
{
  textbox.ClearAllMessages();
  Message* help = new Message("Error obtaining your IP");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.Open([=]() {
    Audio().Play(AudioType::CHIP_ERROR);
  });
  textbox.CompleteCurrentBlock();
}

void PVPScene::HandleCopyEvent()
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

void PVPScene::HandlePasteEvent()
{
  std::string value = Input().GetClipboard();

  if (value != theirIP) {
    Message* help = nullptr;

    if (IsValidIPv4(value)) {
      theirIP = value;
      Audio().Play(AudioType::COUNTER_BONUS);
      help = new Message("Pasted! Press start to connect.");
    }
    else {
      Audio().Play(AudioType::CHIP_ERROR);
      help = new Message("Bad IP");
    }

    textbox.ClearAllMessages();
    textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
    textbox.CompleteCurrentBlock();
  }
}

void PVPScene::ProcessIncomingPackets()
{
  auto& client = Net().GetSocket();

  if (!client.poll(Poco::Timespan{ 0 }, Poco::Net::Socket::SELECT_READ)) return;
  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };
  static int read = 0;

  try {
    // discover their IP
    /*if (theirIP.empty()) {
      Poco::Net::SocketAddress sender;
      read = client.receiveFrom(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN - 1, sender);
      rawBuffer[read] = '\0';

      theirIP = std::string(rawBuffer, read);
    }*/

    Poco::Net::SocketAddress sa(theirIP);
    read += client.receiveFrom(rawBuffer, NetPlayConfig::MAX_BUFFER_LEN - 1, sa);
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
  auto& client = Net().GetSocket();
  Poco::Net::SocketAddress sa(theirIP);

  try {
    Poco::Buffer<char> buffer{ 0 };
    NetPlaySignals type{ NetPlaySignals::connect };
    buffer.append((char*)&type, sizeof(NetPlaySignals));
    buffer.append((char*)&navi, sizeof(size_t));
    client.sendTo(buffer.begin(), (int)buffer.size(), sa);
  }
  catch (Poco::IOException& e) {
    Logger::Logf("IOException when trying to connect to opponent: %s", e.what());
  }
}

void PVPScene::SendHandshakeSignal()
{
  auto& client = Net().GetSocket();
  Poco::Net::SocketAddress sa(theirIP);

  try {
    Poco::Buffer<char> buffer{ 0 };
    NetPlaySignals type{ NetPlaySignals::handshake };
    buffer.append((char*)&type, sizeof(NetPlaySignals));
    client.sendTo(buffer.begin(), (int)buffer.size(), sa);
  }
  catch (Poco::IOException& e) {
    Logger::Logf("IOException when trying to connect to opponent: %s", e.what());
  }
}

void PVPScene::RecieveConnectSignal(const Poco::Buffer<char>& buffer)
{
  if (remoteIsReady) return; // prevent multiple connection requests...

  remoteIsReady = true;

  size_t navi = size_t{ 0 }; std::memcpy(&navi, buffer.begin(), buffer.size());
  auto& meta = NAVIS.At(static_cast<int>(navi));
  this->remotePreview.setTexture(meta.GetPreviewTexture());
  auto height = remotePreview.getSprite().getLocalBounds().height;
  remotePreview.setOrigin(sf::Vector2f(0, height));
}

void PVPScene::RecieveHandshakeSignal()
{
  this->handshakeComplete = true;
  this->SendHandshakeSignal();
}

void PVPScene::DrawIDInputWidget(sf::RenderTexture& surface)
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
  id.setPosition(145, 40);

  // restrict the widget from collapsing at text length 0
  float widgetWidth = std::fmax(id.GetLocalBounds().width+10.f, 50.f);

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

void PVPScene::DrawCopyPasteWidget(sf::RenderTexture& surface)
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

  widgetText.setPosition(145, 83);

  float widgetWidth = widgetText.GetLocalBounds().width + 10.f;

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

const bool PVPScene::IsValidIPv4(const std::string& ip) const {
  /*Poco::Net::IPAddress temp;
  return Poco::Net::IPAddress::tryParse(ip, temp);*/
  return true; // for debugging now...
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
  gridBG->Update(double(elapsed));
  textbox.Update(double(elapsed));

  if (!isScreenReady || leave) return; // don't update our scene if not fully in view from segue

  if (clientIsReady && remoteIsReady && !isInFlashyVSIntro) {
    isInFlashyVSIntro = true;
    Audio().StopStream();
  }
  else if (clientIsReady && !remoteIsReady) {
    if (Input().Has(InputEvents::pressed_cancel)) {
      clientIsReady = false;
      HandleCancel();
    }
    else {
      SendConnectSignal(selectedNavi);
      ProcessIncomingPackets();
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
    Audio().Play(AudioType::PRE_BATTLE, AudioPriority::high);

    // Stop music and go to battle screen 
    Audio().StopStream();

    // Configure the session
    config.remote = theirIP;
    config.myNavi = selectedNavi;

    Player* player = NAVIS.At(selectedNavi).GetNavi();

    // client.close();

    NetworkBattleSceneProps props = {
      { *player, pa, copy, new Field(6, 3), std::make_shared<SecretBackground>() },
      config
    };

    getController().push<effect::to<NetworkBattleScene>>(props);
  } else {
    // TODO use states/switches this is getting out of hand...
    if (clientIsReady && theirIP.size()) {
      ProcessIncomingPackets();

      if (!handshakeComplete && !theirIP.empty()) {
        // Try reaching out to someone...
        this->SendConnectSignal(selectedNavi);
      }

      if (remoteIsReady && !clientIsReady) {
        this->SendHandshakeSignal();
      }
    }

    if (Input().Has(InputEvents::pressed_cancel)) {
      leave = true;
      Audio().Play(AudioType::CHIP_CANCEL);
      // client.close();
      using effect = segue<PushIn<direction::up>, milliseconds<500>>;
      getController().pop<effect>();
    }
    else if (Input().HasSystemCopyEvent() && infoMode) {
      HandleCopyEvent();
    }
    else if (Input().HasSystemPasteEvent() && !infoMode) {
      HandlePasteEvent();
    }
    else if (Input().Has(InputEvents::pressed_shoulder_left) && infoMode) {
      infoMode = false;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      HandleJoinMode();
    }
    else if (Input().Has(InputEvents::pressed_shoulder_right) && !infoMode) {
      infoMode = true;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      HandleInfoMode();
    } 
    else if (Input().Has(InputEvents::pressed_confirm) && !infoMode && !theirIP.empty()) {
      this->clientIsReady = true;
      HandleReady();
      Audio().Play(AudioType::CHIP_CHOOSE);
    }
  }
}

void PVPScene::onDraw(sf::RenderTexture& surface) {
  surface.draw(greenBg);
  surface.draw(*gridBG);

  // Draw the widget pieces
  //surface.draw(ui);
  //uiAnim.SetAnimation("...")

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
}
