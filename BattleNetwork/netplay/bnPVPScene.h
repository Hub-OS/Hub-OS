#pragma once
#include <Swoosh/Activity.h>
#include <Poco/Buffer.h>

#include "../bnFont.h"
#include "../bnText.h"
#include "../bnDrawWindow.h"
#include "../bnAudioResourceManager.h"
#include "../bnTextureResourceManager.h"
#include "../bnBackground.h"
#include "../bnSpriteProxyNode.h"
#include "../bnPA.h"
#include "../bnAnimatedTextBox.h"
#include "../bnCardFolder.h" 
#include "../bnScene.h"

#include "bnNetPlayConfig.h"
/**
 * @class PVPScene
 * @author mav
 * @date 08/22/2020
 * @brief Connect with a remote and battle eachother
 */
class PVPScene : public Scene {
private:
  bool isScreenReady{ false };
  bool leave{ false }; /*!< Scene state coming/going flag */
  bool remoteIsReady{ false };
  bool clientIsReady{ false };
  bool isInFlashyVSIntro{ false };
  bool isInBattleStartup{ false };
  bool infoMode{ true }; // we start here and then allow the player to toggle
  bool handshakeComplete{ false };
  bool playVS{ true };
  int selectedNavi{};
  double sequenceTimer{ 0.0 }; // in seconds
  double flashCooldown{ 0 };
  size_t selectionIndex{ 0 }; // 0 = text input field widget
  CardFolder& folder;
  PA& pa;
  NetPlayConfig netplayconfig;
  Background* gridBG;
  SpriteProxyNode navigator;
  SpriteProxyNode ui;
  SpriteProxyNode vs, vsFaded;
  SpriteProxyNode greenBg;
  SpriteProxyNode clientPreview;
  SpriteProxyNode remotePreview;
  Animation uiAnim;
  AnimatedTextBox textbox;

  static std::string myIP;
  std::string theirIP;
  Text text, id;

  const std::string GetPublicIP(); // TODO: this should be a fetchable value in the web client manager

  // event responses
  void HandleInfoMode();
  void HandleJoinMode();
  void HandleReady();
  void HandleCancel();
  void HandleGetIPFailure();
  void HandleCopyEvent();
  void HandlePasteEvent();

  // netplay comm.
  void ProcessIncomingPackets();
  void SendConnectSignal(const int navi);
  void SendHandshakeSignal(); // sent until we recieve a handshake
  void RecieveConnectSignal(const Poco::Buffer<char>&);
  void RecieveHandshakeSignal();

  // custom drawing
  void DrawIDInputWidget(sf::RenderTexture& surface);
  void DrawCopyPasteWidget(sf::RenderTexture& surface);

  const bool IsValidIPv4(const std::string& ip) const;

  void Reset();

public:
  PVPScene(swoosh::ActivityController&, int, CardFolder&, PA&);
  ~PVPScene();

  /**
 * @brief
 */
  void onStart();
  
  /**
   * @brief
   * @param elapsed in seconds
   */
  void onUpdate(double elapsed);
  
  void onLeave();
  void onEnter();
  void onResume();
  void onExit();

  /**
   * @brief Draws graphic
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface);
  
  void onEnd() { ; }
};
