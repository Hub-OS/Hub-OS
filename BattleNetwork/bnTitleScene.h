#pragma once
#include <memory>
#include <future>
#include "bnLoaderScene.h"
#include "bnSpriteProxyNode.h"
#include "bnFont.h"
#include "bnText.h"

class TitleScene final : public LoaderScene {
private:
  SpriteProxyNode logoSprite, alertSprite;
  SpriteProxyNode bgSprite;
  SpriteProxyNode progSprite;
  SpriteProxyNode cursorSprite;
  Font font, startFont;
  Text logLabel, startLabel;
  std::vector<std::string> logs;
  unsigned progress{}, total {};
  unsigned ellipsis{};
  std::future<bool> loginResult;
  std::string taskStr, incomingTaskStr;

  // Status flags
  bool inLoadState = true;
  bool ready = false;
  bool loadMobs = false;
  bool pressedStart = false;
  bool loginSelected = true;

  void CenterLabel();
public:
  TitleScene(swoosh::ActivityController& controller, TaskGroup&& tasks);
  void onStart() override;
  void onUpdate(double elapsed) override;
  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onDraw(sf::RenderTexture& surface) override;
  void onEnd() override;
  void onTaskBegin(const std::string& taskName, float progress) override;
  void onTaskComplete(const std::string& taskName, float progress) override;
};