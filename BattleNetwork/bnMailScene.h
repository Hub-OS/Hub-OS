#pragma once
#include "bnScene.h"
#include "bnText.h"
#include "bnTextBox.h"
#include "bnMail.h"

#include <vector>

class MailScene : public Scene {
public:

private:
  Text label;
  sf::Sprite moreText;
  sf::Sprite scroll;
  sf::Sprite cursor;
  sf::Sprite bg;
  TextBox textbox;
  float totalElapsed{};
  signed row{}, rowOffset{};
  float heldCooldown{};
  Mail& inbox;

  const signed maxRows = 4;
public:
  MailScene(swoosh::ActivityController& controller, Mail& inbox);
  ~MailScene();

  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onStart() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void onEnd() override;
};