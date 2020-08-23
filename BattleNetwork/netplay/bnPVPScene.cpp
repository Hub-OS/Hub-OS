#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>
#include <SFML/Window/Clipboard.hpp>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

#include "bnPVPScene.h"
#include "bnNetworkBattleScene.h"

#include "Segues/PushIn.h"

#include "../bnMainMenuScene.h"
#include "../bnGridBackground.h"
#include "../bnAudioResourceManager.h"

using namespace Poco;
using namespace Net;
using namespace swoosh::types;

std::string PVPScene::myIP = "";

PVPScene::PVPScene(swoosh::ActivityController& controller, int selected, CardFolder& folder, PA& pa)
  : textbox(sf::Vector2f(4, 250)), selectedNavi(selected), folder(folder), pa(pa),
  uiAnim("resources/pvp_widget.animation"),
  swoosh::Activity(&controller)
{
  // network
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

  float w = controller.getInitialWindowSize().x;
  float h = controller.getInitialWindowSize().y;
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
  text.setPosition(20.f, 50.0f);

  // load animation files
  uiAnim.Load();
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
      return std::string(std::istreambuf_iterator<char>(rs), {});
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
  INPUTx.SetClipboard(myIP);
  AUDIO.Play(AudioType::NEW_GAME);

  textbox.ClearAllMessages();
  Message* help = new Message("Copied!");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.CompleteCurrentBlock();
}

void PVPScene::HandlePasteEvent()
{
  std::string value = INPUTx.GetClipboard();

  if (value != theirIP) {
    theirIP = value;
    AUDIO.Play(AudioType::COUNTER_BONUS);
  }

  textbox.ClearAllMessages();
  Message* help = new Message("Pasted! Press start to connect.");
  textbox.EnqueMessage(navigator.getSprite(), "resources/ui/navigator.animation", help);
  textbox.CompleteCurrentBlock();
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
  auto meta = NAVIS.At(navi);
  this->remotePreview.setTexture(meta.GetPreviewTexture());
  auto height = remotePreview.getSprite().getLocalBounds().height;
  remotePreview.setOrigin(sf::Vector2f(0, height));
}

void PVPScene::RecieveHandshakeSignal()
{
  this->handshakeComplete = true;
  this->SendHandshakeSignal();
}

void PVPScene::onStart() {
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
}

void PVPScene::onResume() {

}

void PVPScene::onUpdate(double elapsed) {
  gridBG->Update(elapsed);
  textbox.Update(elapsed);

  // DEBUG SCENE STUFF
  if (INPUTx.Has(EventTypes::RELEASED_SPECIAL)) {
    clientIsReady = remoteIsReady = true;
    remotePreview.setTexture(clientPreview.getTexture());
    remotePreview.setOrigin(0, remotePreview.getLocalBounds().height);
    theirIP = "127.0.0.1";
  }

  if (!infoMode) {
    text.setString(theirIP);
  }
  else {
    text.setString(myIP);
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
    double amt = 255 * (1 - delta);
    greenBg.setColor(sf::Color(amt,amt,amt,255));

    // Refresh mob graphic origin every frame as it may change
    auto size = getController().getVirtualWindowSize();

    if (delta == 1) {
      delta = swoosh::ease::linear(sequenceTimer - 0.5, 0.5, 1.0);
      float x = static_cast<float>(delta) * 160.0f;

      clientPreview.setPosition(delta * float(size.x) * 0.25f, float(size.y));
      clientPreview.setOrigin(float(clientPreview.getTextureRect().width) * 0.5f, float(clientPreview.getTextureRect().height));

      remotePreview.setPosition(size.x - (delta * float(size.x) * 0.25f), float(size.y));
      remotePreview.setOrigin(float(remotePreview.getTextureRect().width) * 0.5f, float(remotePreview.getTextureRect().height));

      delta = swoosh::ease::bezierPopIn(sequenceTimer - 0.5, 0.5);
      float scale = std::fabs(static_cast<float>(1.0 - delta) * 20.0f) + 1.f;
      vs.setScale(scale, scale);

      if (delta == 1) {
        // do once
        if (playVS) {
          playVS = false;
          AUDIO.Play(AudioType::CUSTOM_BAR_FULL, AudioPriority::highest);
        }

        flashState++;

        // make bg appear
        gridBG->setColor(sf::Color(255, 255, 255, 255));

        if (flashState > 20) {
          delta = swoosh::ease::linear(sequenceTimer - 1.0, 2.0, 1.0);
          float scale = static_cast<float>(delta) + 1.f;
          vsFaded.setScale(scale, scale);
          amt = 150 * (1 - delta);
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

    getController().push<effect::to<NetworkBattleScene>>(player, copy, pa, config);
    // getController().queuePop<effect>(); // for testing animations
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
      getController().queuePop<effect>();
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
      text.setString(sf::String("ERROR"));
    }

    ENGINE.Draw(text);

    if (infoMode) {
      text.setString("My Info");
    }
    else {
      text.setString("Remote");
    }

    // re-use text object to draw more
    auto pos = text.getPosition();
    text.setPosition(20, -5);

    ENGINE.Draw(text);
    text.setPosition(pos); // revert
  }
  else {
    ENGINE.Draw(clientPreview);
    ENGINE.Draw(remotePreview);
    ENGINE.Draw(vs);
    ENGINE.Draw(vsFaded);
  }

  if (flashState >= 1 && flashState <= 20) {
    sf::RectangleShape screen(ENGINE.GetCamera()->GetView().getSize());
    screen.setFillColor(sf::Color::White);
    ENGINE.Draw(screen);
  }
}
