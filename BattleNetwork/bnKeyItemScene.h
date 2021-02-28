#pragma once
#include "bnScene.h"
#include "bnText.h"
#include "bnTextBox.h"

#include <vector>

class KeyItemScene : public Scene {
  struct Item {
    std::string name;
    std::string desc;
  };

  Text label;
  sf::Sprite moreText;
  sf::Sprite scroll;
  sf::Sprite cursor;
  sf::Sprite bg;
  TextBox textbox;
  float totalElapsed{};
  signed col{}, row{}, rowOffset{};
  float heldCooldown{};
  std::vector<Item> items;

  const signed maxRows = 4;
  const signed maxCols = 2;
public:
  KeyItemScene(swoosh::ActivityController& controller);
  ~KeyItemScene();

  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onStart() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void onEnd() override;
};