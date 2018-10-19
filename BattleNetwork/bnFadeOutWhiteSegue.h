#pragma once
#include "bnActivity.h"
#include "bnShaderResourceManager.h"
#include "bnSmartShader.h"

template<class NextActivity>
class Segue : public Activity {
private:
  Activity* next;
  Activity* top;
  double progress;

public:
  Segue() {
   
  }

  virtual void OnStart() {
  }

  virtual void OnUpdate(double _elapsed) {

  }

  virtual void OnLeave() {

  }

  virtual void OnResume() {

  }

  virtual void OnDraw(sf::RenderTexture& surface) {

  }

  virtual void OnEnd() {

  }
};