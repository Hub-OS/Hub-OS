#pragma once
#include "bnSpriteProxyNode.h"
#include "bnResourceHandle.h"
#include "bnFont.h"
#include "bnText.h"
#include "bnCallback.h"

class AppButton : public SceneNode, public ResourceHandle {
private:
  Callback<void()> callback;
  static Font font;
  mutable Text text;
  mutable SpriteProxyNode edge;
  mutable SpriteProxyNode mid;

  const float CalculateWidth() const;

public:
  AppButton(const std::string& name);
  ~AppButton();

  void SetColor(const sf::Color& color);
  void Slot(const Callback<void()> callback);
  void draw(sf::RenderTarget & target, sf::RenderStates states) const;
};