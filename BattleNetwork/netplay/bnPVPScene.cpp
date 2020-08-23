#include "bnPVPScene.h"
#include "bnNetworkBattleScene.h"
#include "../bnMainMenuScene.h"
#include "../bnGridBackground.h"
#include "../bnAudioResourceManager.h"

#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>
#include <SFML/Window/Clipboard.hpp>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

using namespace Poco;
using namespace Net;
using namespace swoosh::types;

std::string PVPScene::myIP = "";

PVPScene::PVPScene(swoosh::ActivityController& controller, size_t selected, CardFolder& folder, PA& pa)
  : textbox(sf::Vector2f(4, 250)), selectedNavi(selected), folder(folder), pa(pa),
  uiAnim("resources/pvp_widget.animation"),
  swoosh::Activity(&controller)
{
  // Sprites
  ui = sf::Sprite(*LOAD_TEXTURE_FILE("resources/ui/pvp_widget.png"));
  vs = sf::Sprite(*LOAD_TEXTURE_FILE("resources/ui/vs_text.png"));
  greenBg = sf::Sprite(*LOAD_TEXTURE(FOLDER_INFO_BG));

  navigator = sf::Sprite(*LOAD_TEXTURE(MUG_NAVIGATOR));
  navigator.setScale(2.f, 2.f);

  ui.setScale(2.f, 2.f);

  float w = controller.getInitialWindowSize().x;
  float h = controller.getInitialWindowSize().y;
  vs.setPosition(w / 2.f, h / 2.f);
  swoosh::game::setOrigin(vs.getSprite(), 0.5, 0.5);
  vs.setScale(2.f, 2.f);

  greenBg.setScale(2.f, 2.f);

  this->gridBG = new GridBackground();

  clientPreview.setTexture(NAVIS.At(selectedNavi).GetPreviewTexture());
  clientPreview.setScale(2.f, 2.f);
  swoosh::game::setOrigin(clientPreview.getSprite(), 1.0, 1.0);
  remotePreview.setScale(2.f, 2.f);

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
  std::string url = "http://api.ipify.org"; // send back public IP in plain text

  HTTPClientSession session(url, 80);
  HTTPRequest request(HTTPRequest::HTTP_GET, "/", HTTPMessage::HTTP_1_1);
  HTTPResponse response;

  session.sendRequest(request);
  std::istream& rs = session.receiveResponse(response);

  if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
  {
    return std::string(std::istreambuf_iterator<char>(rs), {});
  }
  else
  {
    // failed
    return "";
  }
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

void PVPScene::ProcessIncomingPackets()
{
  if (!client.poll(Poco::Timespan{ 0 }, Poco::Net::Socket::SELECT_READ)) return;
  static char rawBuffer[NetPlayConfig::MAX_BUFFER_LEN] = { 0 };
  static int read = 0;

  try {
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
    Logger::Logf("Network exception: %s", e.what());
  }

  read = 0;
  std::memset(rawBuffer, 0, NetPlayConfig::MAX_BUFFER_LEN);
}

void PVPScene::SendConnectSignal(const size_t navi)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::connect };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&navi, sizeof(size_t));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void PVPScene::SendHandshakeSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::handshake };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), (int)buffer.size());
}

void PVPScene::RecieveConnectSignal(const Poco::Buffer<char>& buffer)
{
  if (remoteIsReady) return; // prevent multiple connection requests...

  remoteIsReady = true;

  size_t navi = size_t{ 0 }; std::memcpy(&navi, buffer.begin(), buffer.size());
  auto meta = NAVIS.At(navi);
  this->remotePreview.setTexture(meta.GetPreviewTexture());
  swoosh::game::setOrigin(remotePreview.getSprite(), 0.0, 1.0);
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

  // always on info mode first
  HandleInfoMode();
}

void PVPScene::onResume() {

}

void PVPScene::onUpdate(double elapsed) {
  gridBG->Update(elapsed);
  textbox.Update(elapsed);

  if (!infoMode) {
    text.setString(theirIP);
  }
  else {
    text.setString(myIP);
  }

  if (!isScreenReady || leave) return; // don't update our scene if not fully in view from segue

  if (clientIsReady && remoteIsReady && !isInFlashyVSIntro) {
    isInFlashyVSIntro = true;
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
    greenBg.setColor(sf::Color(255 * (1 - delta)));

    if (delta > 1) {
      delta = swoosh::ease::linear(sequenceTimer - 0.5, 0.5, 1.0);
      float x = static_cast<float>(delta) * 100.0f;
      float y = 320;
      sf::Vector2f pos = sf::Vector2f(x, y);
      clientPreview.setPosition(pos);

      pos = sf::Vector2f(640 - x, y);
      remotePreview.setPosition(pos);

      gridBG->setColor(sf::Color(255 * (delta)));

      if (delta > 1) {
        delta = swoosh::ease::linear(sequenceTimer - 1, 0.5, 1.0);
        float scale = min(2.0f, static_cast<float>(1.0 - delta) * 100.0f);
        vs.setScale(scale, scale);

        if (delta >= 1 && playVS) {
          playVS = false;
          AUDIO.Play(AudioType::DARK_CARD);
        }
      }
    }

    if (this->sequenceTimer >= 3) {
      isInBattleStartup = true;
    }
  }
  else if (isInBattleStartup) {
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

    Player* player = NAVIS.At(selectedNavi).GetNavi();
    getController().push<effect::to<NetworkBattleScene>>(player, copy, pa, config);
  } else if(!leave) {
    ProcessIncomingPackets();

    if (!handshakeComplete) {
      // Try reaching out to someone...
      this->SendConnectSignal(selectedNavi);
    }

    if (INPUTx.Has(EventTypes::PRESSED_CANCEL)) {
      leave = true;
      AUDIO.Play(AudioType::CHIP_CANCEL);
      textbox.ClearAllMessages();
      textbox.Close();
    }else if (textbox.IsOpen() && INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
      textbox.DequeMessage();
      textbox.CompleteCurrentBlock();
    }
    else if (INPUTx.HasSystemCopyEvent() && infoMode) {
      INPUTx.SetClipboard(myIP);
      AUDIO.Play(AudioType::NEW_GAME);
    }
    else if (INPUTx.HasSystemPasteEvent() && !infoMode) {
      std::string value = INPUTx.GetClipboard();

      if (value != theirIP) {
        theirIP = value;
        AUDIO.Play(AudioType::COUNTER_BONUS);
      }
    }
    else if (INPUTx.Has(EventTypes::PRESSED_SCAN_LEFT) && infoMode) {
      infoMode = false;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      HandleJoinMode();
    }
    else if (INPUTx.Has(EventTypes::PRESSED_SCAN_RIGHT) && !infoMode) {
      infoMode = false;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      HandleInfoMode();
    } 
    else if (INPUTx.Has(EventTypes::PRESSED_CONFIRM) && !infoMode && !theirIP.empty()) {
      this->clientIsReady = true;
      HandleReady();
    }
  } else {
      using effect = segue<WhiteWashFade, milliseconds<500>>;
      getController().push<effect::to<MainMenuScene>>();
  }
}

void PVPScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(gridBG);
  ENGINE.Draw(greenBg);
  ENGINE.Draw(textbox);

  // Draw the widget pieces
  ENGINE.Draw(ui);
  //uiAnim.SetAnimation("...")

  if (!isInFlashyVSIntro) {
    ENGINE.Draw(text);

    if (infoMode) {
      text.setString("My Info");
    }
    else {
      text.setString("Remote");
    }

    // re-use text object to draw more
    auto pos = text.getPosition();
    text.setPosition(20, 5);

    ENGINE.Draw(text);
    text.setPosition(pos); // revert
  }
  else {
    ENGINE.Draw(clientPreview);
    ENGINE.Draw(remotePreview);
    ENGINE.Draw(vs);
  }
}
