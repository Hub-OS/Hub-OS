#pragma once
#include "bnEngine.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

#include "Swoosh/Activity.h"

/*
This scene takes in a texture input. The texture is the last scene's screen snapshot.
This serves as a way to turn the "scene" drawn during loading, into a swoosh-compatible 
activity.
*/
class FakeScene : public swoosh::Activity {
private:
  sf::Sprite snapshot;
  bool triggered;

public:
  FakeScene(swoosh::ActivityController&, sf::Texture& snapshot);
  ~FakeScene();

  void onStart();
  void onUpdate(double elapsed);
  void onLeave() { ; }
  void onExit() { ; }
  void onEnter() { ; }
  void onResume();
  void onDraw(sf::RenderTexture& surface);
  void onEnd() { ; }
};