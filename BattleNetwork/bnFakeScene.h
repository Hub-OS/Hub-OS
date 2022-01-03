#pragma once
#include "bnGame.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

#include <Swoosh/Activity.h>

/**
 * @class FakeScene
 * @author mav
 * @date 04/05/19
 * @brief Take a texture input and make into a swoosh-compatible activity for segue effects
 * 
 * This scene takes in a texture input. The texture is the last scene's screen snapshot.
 * This serves as a way to turn the screen during loading, into a swoosh-compatible 
 * activity.
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
