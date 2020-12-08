#pragma once
#include <memory>
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
  unsigned totalObjects;
  std::atomic<int> progress;
  std::atomic<int> navisLoaded;
  std::atomic<int> mobsLoaded;

  // Status flags
  bool inLoadState = true;
  bool ready = false;
  bool loadMobs = false;
  bool pressedStart = false;
  bool loginSelected = true;

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