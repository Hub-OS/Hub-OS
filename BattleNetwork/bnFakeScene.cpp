#include "bnFakeScene.h"
#include <Swoosh/ActivityController.h>

#include "Segues/BlackWashFade.h"

FakeScene::FakeScene(swoosh::ActivityController& controller, sf::Texture& snapshot) : swoosh::Activity(&controller) {
  FakeScene::snapshot = sf::Sprite(snapshot);
  triggered = false;
}

FakeScene::~FakeScene() {

}

void FakeScene::onStart() {
}

void FakeScene::onResume() {

}

/**
 * @brief Immediately pop the activity with a pattern effect
 * @param elapsed in seconds
 */
void FakeScene::onUpdate(double elapsed) {
  if (!triggered) {
    triggered = true;

    using pattern = BlackWashFade;
    using segue = swoosh::intent::segue<pattern, swoosh::intent::milli<500>>;
    getController().queuePop<segue>();
  }
}

void FakeScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(snapshot);
}
