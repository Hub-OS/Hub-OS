#pragma once
#include "bnActivity.h"

class MainMenuScene : public Activity {
public:
  virtual void OnStart();
  virtual void OnUpdate(double _elapsed);
  virtual void OnLeave();
  virtual void OnResume();
  virtual void OnDraw(sf::RenderTexture& surface);
  virtual void OnEnd();
  virtual ~MainMenuScene() { ; }
};
