#pragma once
#include "bnEngine.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

#include "Swoosh/Activity.h"

class GameOverScene : public swoosh::Activity {
private:
  float fadeInCooldown;

public:
  GameOverScene(swoosh::ActivityController&);
  ~GameOverScene();

  void onStart();
  void onUpdate(double elapsed);
  void onLeave() { ; }
  void onExit() { ; }
  void onEnter() { ; }
  void onResume() { ; }
  void onDraw(sf::RenderTexture& surface);
  void onEnd() { ; }
};